/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * 
 * Reference:
 *   Json load using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>

#include "ctl-binding.h"

typedef void*(*DispatchPluginInstallCbT)(const char* label, const char*version, const char*info);


static afb_req NULL_AFBREQ = {};


typedef struct {
    const char* label;
    const char *info;
    DispatchActionT *actions;
} DispatchHandleT;

typedef struct {
    const char *label;
    const char *info;
    void *context;
    char *sharelib;
    void *dlHandle;
    luaL_Reg *l2cFunc;
    int l2cCount;
} DispatchPluginT;

typedef struct {
    const char* label;
    const char *info;
    const char *version;
    DispatchPluginT *plugin;
    DispatchHandleT **onloads;
    DispatchHandleT **events;
    DispatchHandleT **controls;
} DispatchConfigT;

// global config handle 
STATIC DispatchConfigT *configHandle = NULL;

STATIC int DispatchControlToIndex(DispatchHandleT **controls, const char* controlLabel) {

    for (int idx = 0; controls[idx]; idx++) {
        if (!strcasecmp(controlLabel, controls[idx]->label)) return idx;
    }
    return -1;
}

STATIC int DispatchOneControl(DispatchSourceT source, DispatchHandleT **controls, const char* controlLabel, json_object *queryJ, afb_req request) {
    int err;
    
    if (!configHandle) {
        AFB_ERROR("DISPATCH-CTL-API: (Hoops/Bug!!!) No Config Loaded");
        goto OnErrorExit;
    }

    if (!configHandle->controls) {
        AFB_ERROR("DISPATCH-CTL-API: No Control Action in Json config label=%s version=%s", configHandle->label, configHandle->version);
        goto OnErrorExit;
    }

    int index = DispatchControlToIndex(controls, controlLabel);
    if (index < 0 || !controls[index]->actions) {
        AFB_ERROR("DISPATCH-CTL-API:NotFound/Error label=%s in Json Control Config File", controlLabel);
        goto OnErrorExit;
    }
    
    // Fulup (Bug/Feature) in current version is unique to every onload profile
    if (configHandle->plugin && configHandle->plugin->l2cCount) {
        LuaL2cNewLib (configHandle->plugin->label, configHandle->plugin->l2cFunc, configHandle->plugin->l2cCount);      
    }
    
    // loop on action for this control
    DispatchActionT *actions = controls[index]->actions;
    for (int idx = 0; actions[idx].label; idx++) {

        switch (actions[idx].mode) {
            case CTL_MODE_API:
            {
                json_object *returnJ;

                // if query is empty increment usage count and pass args
                if (!queryJ || json_object_get_type(queryJ) != json_type_object) {
                    json_object_get(actions[idx].argsJ);                    
                    queryJ= actions[idx].argsJ;
                } else if (actions[idx].argsJ) {
                    
                    // Merge queryJ and argsJ before sending request
                    if (json_object_get_type(actions[idx].argsJ) ==  json_type_object) {
                        json_object_object_foreach(actions[idx].argsJ, key, val) {
                        json_object_object_add(queryJ, key, val);
                        }
                    } else {
                        json_object_object_add(queryJ, "args", actions[idx].argsJ);
                    }
                }
                
                int err = afb_service_call_sync(actions[idx].api, actions[idx].call, queryJ, &returnJ);
                if (err) {
                    static const char*format = "DispatchOneControl(Api) api=%s verb=%s args=%s";
                    if (afb_req_is_valid(request))afb_req_fail_f(request, "DISPATCH-CTL-MODE:API", format, actions[idx].label, actions[idx].api, actions[idx].call);
                    else AFB_ERROR(format, actions[idx].api, actions[idx].call, actions[idx].label);
                    goto OnErrorExit;
                }
                break;
            }

#ifdef CONTROL_SUPPORT_LUA
            case CTL_MODE_LUA:
                err = LuaCallFunc(source, &actions[idx], queryJ);
                if (err) {
                    static const char*format = "DispatchOneControl(Lua) label=%s func=%s args=%s";
                    if (afb_req_is_valid(request)) afb_req_fail_f(request, "DISPATCH-CTL-MODE:Lua", format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    else AFB_ERROR(format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    goto OnErrorExit;
                }
                break;
#endif

            case CTL_MODE_CB:
                err = (*actions[idx].actionCB) (source, actions[idx].label, actions[idx].argsJ, queryJ, configHandle->plugin->context);
                if (err) {
                    static const char*format = "DispatchOneControl(Callback) label%s func=%s args=%s";
                    if (afb_req_is_valid(request)) afb_req_fail_f(request, "DISPATCH-CTL-MODE:Cb", format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    else AFB_ERROR(format, actions[idx].label, actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    goto OnErrorExit;
                }
                break;

            default:
            {
                static const char*format = "DispatchOneControl(unknown) mode control=%s action=%s";
                AFB_ERROR(format, controls[index]->label);
                if (afb_req_is_valid(request))afb_req_fail_f(request, "DISPATCH-CTL-MODE:Unknown", format, controls[index]->label);
            }
        }
    }

    // everything when fine
    if (afb_req_is_valid(request))afb_req_success(request, NULL, controls[index]->label);
    return 0;

OnErrorExit:
    return -1;
}


// Event name is mapped on control label and executed as a standard control

PUBLIC void DispatchOneEvent(const char *evtLabel, json_object *eventJ) {
    DispatchHandleT **events = configHandle->events;

    (void) DispatchOneControl(CTL_SOURCE_EVENT, events, evtLabel, eventJ, NULL_AFBREQ);
}

// Event name is mapped on control label and executed as a standard control

PUBLIC int DispatchOnLoad(const char *onLoadLabel) {
    if (!configHandle) return 1;
    
    DispatchHandleT **onloads = configHandle->onloads;

    int err = DispatchOneControl(CTL_SOURCE_ONLOAD, onloads, onLoadLabel, NULL, NULL_AFBREQ);
    return err;
}

PUBLIC void ctlapi_dispatch(afb_req request) {
    DispatchHandleT **controls = configHandle->controls;
    json_object *queryJ, *argsJ=NULL;
    const char *target;
    DispatchSourceT source= CTL_SOURCE_UNKNOWN;
    
    queryJ = afb_req_json(request);
    int err = wrap_json_unpack(queryJ, "{s:s, s?i s?o !}", "target", &target, "source", &source, "args", &argsJ);
    if (err) {
        afb_req_fail_f(request, "CTL-DISPTACH-INVALID", "missing target or args not a valid json object query=%s", json_object_get_string(queryJ));
        goto OnErrorExit;
    }
    
    (void) DispatchOneControl(source, controls, target, argsJ, request);

OnErrorExit:
    return;
}

// Wrapper to Lua2c plugin command add context dans delegate to LuaWrapper
PUBLIC int DispatchOneL2c(lua_State* luaState, char *funcname, Lua2cFunctionT callback) {
#ifndef CONTROL_SUPPORT_LUA
    AFB_ERROR("DISPATCH-ONE-L2C: LUA support not selected (cf:CONTROL_SUPPORT_LUA) in config.cmake");
    return 1;
#else    
    int err=Lua2cWrapper(luaState, funcname, callback, configHandle->plugin->context);
    return err;
#endif
}


// List Avaliable Configuration Files

PUBLIC void ctlapi_config(struct afb_req request) {
    json_object*tmpJ;
    char *dirList;
    

    json_object* queryJ = afb_req_json(request);
    if (queryJ && json_object_object_get_ex(queryJ, "cfgpath", &tmpJ)) {
        dirList = strdup(json_object_get_string(tmpJ));
    } else {
        
        dirList = getenv("CONTROL_CONFIG_PATH");
        if (!dirList) dirList = strdup(CONTROL_CONFIG_PATH);
        AFB_NOTICE("CONFIG-MISSING: use default CONTROL_CONFIG_PATH=%s", CONTROL_CONFIG_PATH);
    }

    // get list of config file
    struct json_object *responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, "onload", "json");

    if (json_object_array_length(responseJ) == 0) {
        afb_req_fail(request, "CONFIGPATH:EMPTY", "No Config Found in CONTROL_CONFIG_PATH");
    } else {
        afb_req_success(request, responseJ, NULL);
    }

    return;
}

// unpack individual action object

STATIC int DispatchLoadOneAction(DispatchConfigT *controlConfig, json_object *actionJ, DispatchActionT *action) {
    char *api = NULL, *verb = NULL, *callback = NULL, *lua = NULL;
    int err, modeCount = 0;

    err = wrap_json_unpack(actionJ, "{ss,s?s,s?s,s?s,s?s,s?s,s?o !}"
            , "label", &action->label, "info", &action->info, "callback", &callback, "lua", &lua, "api", &api, "verb", &verb, "args", &action->argsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-ACTION Missing something label|info|callback|lua|(api+verb)|args in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (lua) {
        action->mode = CTL_MODE_LUA;
        action->call = lua;
        modeCount++;
    }

    if (api && verb) {
        action->mode = CTL_MODE_API;
        action->api = api;
        action->call = verb;
        modeCount++;
    }

    if (callback && controlConfig->plugin) {
        action->mode = CTL_MODE_CB;
        action->call = callback;
        modeCount++;

        action->actionCB = dlsym(controlConfig->plugin->dlHandle, callback);
        if (!action->actionCB) {
            AFB_ERROR("DISPATCH-LOAD-ACTION fail to find calbback=%s in %s", callback, controlConfig->plugin->sharelib);
            goto OnErrorExit;
        }
    }

    // make sure at least one mode is selected
    if (modeCount == 0) {
        AFB_ERROR("DISPATCH-LOAD-ACTION No Action Selected lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (modeCount > 1) {
        AFB_ERROR("DISPATCH-LOAD-ACTION:ToMany arguments lua|callback|(api+verb) in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }
    return 0;

OnErrorExit:
    return -1;
};

STATIC DispatchActionT *DispatchLoadActions(DispatchConfigT *controlConfig, json_object *actionsJ) {
    int err;
    DispatchActionT *actions;

    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
        actions = calloc(count + 1, sizeof (DispatchActionT));

        for (int idx = 0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);
            err = DispatchLoadOneAction(controlConfig, actionJ, &actions[idx]);
            if (err) goto OnErrorExit;
        }

    } else {
        actions = calloc(2, sizeof (DispatchActionT));
        err = DispatchLoadOneAction(controlConfig, actionsJ, &actions[0]);
        if (err) goto OnErrorExit;
    }

    return actions;

OnErrorExit:
    return NULL;

}

STATIC DispatchHandleT *DispatchLoadControl(DispatchConfigT *controlConfig, json_object *controlJ) {
    json_object *actionsJ, *permissionsJ;
    int err;

    DispatchHandleT *dispatchHandle = calloc(1, sizeof (DispatchHandleT));
    err = wrap_json_unpack(controlJ, "{ss,s?s,s?o, so !}", "label", &dispatchHandle->label, "info", &dispatchHandle->info
         , "permissions", &permissionsJ, "actions", &actionsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:CONTROL Missing something label|[info]|actions in %s", json_object_get_string(controlJ));
        goto OnErrorExit;
    }

    dispatchHandle->actions = DispatchLoadActions(controlConfig, actionsJ);
    if (!dispatchHandle->actions) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:CONTROL Error when parsing actions %s", dispatchHandle->label);
        goto OnErrorExit;
    }
    return dispatchHandle;

OnErrorExit:
    return NULL;
}

STATIC DispatchHandleT *DispatchLoadOnload(DispatchConfigT *controlConfig, json_object *onloadJ) {
    json_object *actionsJ = NULL, *requireJ = NULL, *pluginJ = NULL;
    int err;

    DispatchHandleT *dispatchHandle = calloc(1, sizeof (DispatchHandleT));
    err = wrap_json_unpack(onloadJ, "{ss,s?s, s?o,s?o,s?o !}",
            "label", &dispatchHandle->label, "info", &dispatchHandle->info, "plugin", &pluginJ, "require", &requireJ, "actions", &actionsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Missing something label|[info]|[plugin]|[actions] in %s", json_object_get_string(onloadJ));
        goto OnErrorExit;
    }

    // best effort to initialise everything before starting
    if (requireJ) {

        void DispatchRequireOneApi(json_object * bindindJ) {
            const char* requireBinding = json_object_get_string(bindindJ);
            err = afb_daemon_require_api(requireBinding, 1);
            if (err) {
                AFB_WARNING("DISPATCH-LOAD-CONFIG:REQUIRE Fail to get=%s", requireBinding);
            }
        }

        if (json_object_get_type(requireJ) == json_type_array) {
            for (int idx = 0; idx < json_object_array_length(requireJ); idx++) {
                DispatchRequireOneApi(json_object_array_get_idx(requireJ, idx));
            }
        } else {
            DispatchRequireOneApi(requireJ);
        }
    }

    if (pluginJ) {
        json_object *lua2csJ = NULL;
        DispatchPluginT *dPlugin= calloc(1, sizeof(DispatchPluginT));
        controlConfig->plugin = dPlugin;
        const char*ldSearchPath=NULL;

        err = wrap_json_unpack(pluginJ, "{ss,s?s,s?s,ss,s?o!}",
                "label", &dPlugin->label, "info", &dPlugin->info, "ldpath", &ldSearchPath, "sharelib", &dPlugin->sharelib, "lua2c", &lua2csJ);
        if (err) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Plugin missing label|[info]|sharelib|[lua2c] in %s", json_object_get_string(onloadJ));
            goto OnErrorExit;
        }

        // if search path not in Json config file, then try default
        if (!ldSearchPath) ldSearchPath=CONTROL_PLUGIN_PATH;
        
        // search for default policy config file
        json_object *pluginPathJ = ScanForConfig(ldSearchPath, CTL_SCAN_RECURSIVE, dPlugin->sharelib, NULL);
        if (!pluginPathJ || json_object_array_length(pluginPathJ) == 0) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN Missing plugin=%s in path=%s", dPlugin->sharelib, ldSearchPath);
            goto OnErrorExit;
        }

        char *filename;
        char*fullpath;
        err = wrap_json_unpack(json_object_array_get_idx(pluginPathJ, 0), "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN HOOPs invalid plugin file path = %s", json_object_get_string(pluginPathJ));
            goto OnErrorExit;
        }

        if (json_object_array_length(pluginPathJ) > 1) {
            AFB_WARNING("DISPATCH-LOAD-CONFIG:PLUGIN plugin multiple instances in searchpath will use %s/%s", fullpath, filename);
        }

        char pluginpath[CONTROL_MAXPATH_LEN];
        strncpy(pluginpath, fullpath, sizeof (pluginpath));
        strncat(pluginpath, "/", sizeof (pluginpath));
        strncat(pluginpath, filename, sizeof (pluginpath));
        dPlugin->dlHandle = dlopen(pluginpath, RTLD_NOW);
        if (!dPlugin->dlHandle) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:PLUGIN Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
            goto OnErrorExit;
        }

        CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*) dlsym(dPlugin->dlHandle, "CtlPluginMagic");
        if (!ctlPluginMagic || ctlPluginMagic->magic != CTL_PLUGIN_MAGIC) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin symbol'CtlPluginMagic' missing or !=  CTL_PLUGIN_MAGIC plugin=%s", pluginpath);
            goto OnErrorExit;
        } else {
            AFB_NOTICE("DISPATCH-LOAD-CONFIG:Plugin %s successfully registered", ctlPluginMagic->label);
        }

        // Jose hack to make verbosity visible from sharelib
        struct afb_binding_data_v2 *afbHidenData = dlsym(dPlugin->dlHandle, "afbBindingV2data");
        if (afbHidenData) *afbHidenData = afbBindingV2data;

        // Push lua2cWrapper @ into plugin 
        Lua2cWrapperT *lua2cInPlug = dlsym(dPlugin->dlHandle, "Lua2cWrap");
#ifndef CONTROL_SUPPORT_LUA      
        if (lua2cInPlug) *lua2cInPlug = NULL;
#else
        // Lua2cWrapper is part of binder and not expose to dynamic link
        if (lua2cInPlug) *lua2cInPlug = DispatchOneL2c;
        
        {
            int Lua2cAddOne(luaL_Reg *l2cFunc, const char* l2cName, int index) {
                char funcName[CONTROL_MAXPATH_LEN];
                strncpy(funcName, "lua2c_", sizeof(funcName));
                strncat(funcName, l2cName, sizeof(funcName));
                
                Lua2cFunctionT l2cFunction= (Lua2cFunctionT)dlsym(dPlugin->dlHandle, funcName);
                if (!l2cFunction) {
                    AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin symbol'%s' missing err=%s", funcName, dlerror());
                    return 1;
                }
                l2cFunc[index].func=(void*)l2cFunction;
                l2cFunc[index].name=strdup(l2cName);
                
                return 0;
            }

            int errCount = 0;
            luaL_Reg *l2cFunc=NULL;
            int count=0;
            
            // look on l2c command and push them to LUA
            if (json_object_get_type(lua2csJ) == json_type_array) {
                int length = json_object_array_length(lua2csJ);
                l2cFunc = calloc(length + 1, sizeof (luaL_Reg));
                for (count=0; count < length; count++) {
                    int err;
                    const char *l2cName = json_object_get_string(json_object_array_get_idx(lua2csJ, count));
                    err = Lua2cAddOne(l2cFunc, l2cName, count);
                    if (err) errCount++;
                }
            } else {
                l2cFunc = calloc(2, sizeof (luaL_Reg));
                const char *l2cName = json_object_get_string(lua2csJ);
                errCount = Lua2cAddOne(l2cFunc, l2cName, 0);
                count=1;
            }
            if (errCount) {
                AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin %d symbols not found in plugin='%s'", errCount, pluginpath);
                goto OnErrorExit;
            } else {
                dPlugin->l2cFunc= l2cFunc;
                dPlugin->l2cCount= count;
            }
        }
#endif
        DispatchPluginInstallCbT ctlPluginOnload = dlsym(dPlugin->dlHandle, "CtlPluginOnload");
        if (ctlPluginOnload) {
            dPlugin->context = (*ctlPluginOnload) (controlConfig->label, controlConfig->version, controlConfig->info);
        }
    }

    dispatchHandle->actions = DispatchLoadActions(controlConfig, actionsJ);
    if (!dispatchHandle->actions) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:ONLOAD Error when parsing actions %s", dispatchHandle->label);
        goto OnErrorExit;
    }
    return dispatchHandle;

OnErrorExit:
    return NULL;
}

STATIC DispatchConfigT *DispatchLoadConfig(const char* filepath) {
    json_object *controlConfigJ, *ignoreJ;
    int err;

    // Load JSON file
    controlConfigJ = json_object_from_file(filepath);
    if (!controlConfigJ) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG:JsonLoad invalid JSON %s ", filepath);
        goto OnErrorExit;
    }

    AFB_INFO("DISPATCH-LOAD-CONFIG: loading config filepath=%s", filepath);

    json_object *metadataJ = NULL, *onloadsJ = NULL, *controlsJ = NULL, *eventsJ = NULL;
    err = wrap_json_unpack(controlConfigJ, "{s?o,so,s?o,s?o,s?o !}", "$schema", &ignoreJ, "metadata", &metadataJ, "onload", &onloadsJ, "controls", &controlsJ, "events", &eventsJ);
    if (err) {
        AFB_ERROR("DISPATCH-LOAD-CONFIG Missing something metadata|[onload]|[controls]|[events] in %s", json_object_get_string(controlConfigJ));
        goto OnErrorExit;
    }

    DispatchConfigT *controlConfig = calloc(1, sizeof (DispatchConfigT));
    if (metadataJ) {
        err = wrap_json_unpack(metadataJ, "{ss,s?s,ss !}", "label", &controlConfig->label, "info", &controlConfig->info, "version", &controlConfig->version);
        if (err) {
            AFB_ERROR("DISPATCH-LOAD-CONFIG:METADATA Missing something label|version|[label] in %s", json_object_get_string(metadataJ));
            goto OnErrorExit;
        }
    }

    if (onloadsJ) {
        DispatchHandleT *dispatchHandle;

        if (json_object_get_type(onloadsJ) != json_type_array) {
            controlConfig->onloads = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadOnload(controlConfig, onloadsJ);
            controlConfig->onloads[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(onloadsJ);
            controlConfig->onloads = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *onloadJ = json_object_array_get_idx(onloadsJ, jdx);
                dispatchHandle = DispatchLoadOnload(controlConfig, onloadJ);
                controlConfig->onloads[jdx] = dispatchHandle;
            }
        }
    }

    if (controlsJ) {
        DispatchHandleT *dispatchHandle;

        if (json_object_get_type(controlsJ) != json_type_array) {
            controlConfig->controls = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadControl(controlConfig, controlsJ);
            controlConfig->controls[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(controlsJ);
            controlConfig->controls = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *controlJ = json_object_array_get_idx(controlsJ, jdx);
                dispatchHandle = DispatchLoadControl(controlConfig, controlJ);
                controlConfig->controls[jdx] = dispatchHandle;
            }
        }
    }

    if (eventsJ) {
        DispatchHandleT *dispatchHandle;

        if (json_object_get_type(eventsJ) != json_type_array) {
            controlConfig->events = (DispatchHandleT**) calloc(2, sizeof (void*));
            dispatchHandle = DispatchLoadControl(controlConfig, eventsJ);
            controlConfig->events[0] = dispatchHandle;
        } else {
            int length = json_object_array_length(eventsJ);
            controlConfig->events = (DispatchHandleT**) calloc(length + 1, sizeof (void*));

            for (int jdx = 0; jdx < length; jdx++) {
                json_object *eventJ = json_object_array_get_idx(eventsJ, jdx);
                dispatchHandle = DispatchLoadControl(controlConfig, eventJ);
                controlConfig->events[jdx] = dispatchHandle;
            }
        }
    }

    return controlConfig;

OnErrorExit:
    return NULL;
}


// Load default config file at init

PUBLIC int DispatchInit() {
    int index, err, luaLoaded = 0;
    char controlFile [CONTROL_MAXPATH_LEN];
    
    const char *dirList= getenv("CONTROL_CONFIG_PATH");
    if (!dirList) dirList=CONTROL_CONFIG_PATH;

    strncpy(controlFile, CONTROL_CONFIG_PRE "-", CONTROL_MAXPATH_LEN);
    strncat(controlFile, GetBinderName(), CONTROL_MAXPATH_LEN);

    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(dirList, CTL_SCAN_RECURSIVE, controlFile, "json");

    // We load 1st file others are just warnings   
    for (index = 0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ = json_object_array_get_idx(responseJ, index);

        char *filename;
        char*fullpath;
        err = wrap_json_unpack(entryJ, "{s:s, s:s !}", "fullpath", &fullpath, "filename", &filename);
        if (err) {
            AFB_ERROR("DISPATCH-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            goto OnErrorExit;
        }

        if (index == 0) {
            if (strcasestr(filename, controlFile)) {
                char filepath[CONTROL_MAXPATH_LEN];
                strncpy(filepath, fullpath, sizeof (filepath));
                strncat(filepath, "/", sizeof (filepath));
                strncat(filepath, filename, sizeof (filepath));
                configHandle = DispatchLoadConfig(filepath);
                if (!configHandle) {
                    AFB_ERROR("DISPATCH-INIT:ERROR Fail loading [%s]", filepath);
                    goto OnErrorExit;
                }
                luaLoaded = 1;
                break;
            }
        } else {
            AFB_WARNING("DISPATCH-INIT:WARNING Secondary Control Config Ignored %s/%s", fullpath, filename);
        }
    }

    // no dispatch config found remove control API from binder
    if (!luaLoaded) {
        AFB_WARNING("DISPATCH-INIT:WARNING (setenv CONTROL_CONFIG_PATH) No Config '%s-*.json' in '%s'", controlFile, dirList);
    }

    AFB_NOTICE("DISPATCH-INIT:SUCCES: Audio Control Dispatch Init");
    return 0;

OnErrorExit:
    AFB_NOTICE("DISPATCH-INIT:ERROR: Audio Control Dispatch Init");
    return 1;
}



