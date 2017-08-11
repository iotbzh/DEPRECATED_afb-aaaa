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

#include "wrap-json.h"
#include "ctl-binding.h"

typedef void*(*DispatchPluginInstallCbT)(const char* label, const char*version, const char*info);

typedef struct {
    const char* label;
    const char *info;
    DispatchActionT *actions;
} DispatchHandleT;

typedef struct {
    const char* label;
    const char *info;
    const char *version;
    void *context;
    char *plugin;
    void *dlHandle;
    DispatchHandleT *onload;
    DispatchHandleT *events;
    DispatchHandleT **controls;
} DispatchConfigT;

// global config handle 
STATIC DispatchConfigT *configHandle = NULL;

STATIC int DispatchControlToIndex(char* controlLabel) {
    DispatchHandleT **controls=configHandle->controls;
    for (int idx=0; controls[idx]; idx++) {
        if (!strcasecmp(controlLabel, controls[idx]->label)) return idx;
    }
    return 0;
}

PUBLIC void ctlapi_dispatch (char* controlLabel, afb_req request) {
    DispatchHandleT **controls=configHandle->controls;
    int err;
    
    if(!configHandle->controls) {
        AFB_ERROR ("DISPATCH-CTL-API: No Control Action in Json config label=%s version=%s", configHandle->label, configHandle->version);
        goto OnErrorExit;        
    }
    
    int index=DispatchControlToIndex(controlLabel);
    if (!index || !controls[index]->actions) {
        AFB_ERROR ("DISPATCH-CTL-API:NotFound label=%s index=%d", controlLabel, index);
        goto OnErrorExit;
    } else {
        AFB_NOTICE("DISPATCH-CTL-API:Found Control=%s", controls[index]->label);
    }

    // loop on action for this control
    DispatchActionT *actions=controls[index]->actions;
    for (int idx=0; actions[idx].label; idx++) {
        
        switch (actions[idx].mode) {
            case CTL_MODE_API: {
                json_object *returnJ;
                
                json_object_get( actions[idx].argsJ); // make sure afb_service_call does not free the argsJ
                int err = afb_service_call_sync(actions[idx].api, actions[idx].call, actions[idx].argsJ, &returnJ);
                if (err) {
                    AFB_NOTICE ("DISPATCH-CTL-MODE:Api api=%s verb=%s args=%s", actions[idx].api, actions[idx].call, actions[idx].label);
                    afb_req_fail_f(request, "DISPATCH-CTL-MODE:Api", "label=%s api=%s verb=%s", actions[idx].label, actions[idx].api, actions[idx].call);
                    goto OnErrorExit;
                }
                break;
            }
                
            case CTL_MODE_LUA:
                err= LuaCallFunc (request, &actions[idx]);
                if (err) {
                    afb_req_fail_f(request, "DISPATCH-CTL-MODE:Lua", "func=%s args=%s", actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    goto OnErrorExit;
                }
                break;
                
            case CTL_MODE_CB:
                err= (*actions[idx].actionCB) (request, &actions[idx], configHandle->context);
                if (err) {
                    afb_req_fail_f(request, "DISPATCH-CTL-MODE:Cb", "func=%s args=%s", actions[idx].call, json_object_get_string(actions[idx].argsJ));
                    goto OnErrorExit;
                }
                break;
                
            default:
                AFB_ERROR ("DISPATCH-CTL-API:control=%s action=%s", controls[index]->label, actions[idx].label);
                afb_req_fail_f(request, "DISPATCH-CTL-MODE:Unknown", "control=%s action=%s", controls[index]->label, actions[idx].label);
        }
    }
    
    // everything when fine
    afb_req_success(request,NULL,controls[index]->label);
    return;
    
    OnErrorExit:
        return;
}

// List Avaliable Configuration Files
PUBLIC void ctlapi_config (struct afb_req request) {
    json_object*tmpJ;
    char *dirList;
    
    json_object* argsJ = afb_req_json(request);
    if (argsJ && json_object_object_get_ex (argsJ, "cfgpath" , &tmpJ)) {
        dirList = strdup (json_object_get_string(tmpJ));
    } else {    
        dirList = strdup (CONTROL_CONFIG_PATH); 
        AFB_NOTICE ("CONFIG-MISSING: use default CONTROL_CONFIG_PATH=%s", CONTROL_CONFIG_PATH);
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
STATIC int DispatchLoadOneAction (DispatchConfigT *controlConfig, json_object *actionJ,  DispatchActionT *action) {
    char *api=NULL, *verb=NULL, *callback=NULL, *lua=NULL;
    int err, modeCount=0;

    err= wrap_json_unpack(actionJ, "{ss,s?s,s?s,s?s,s?s,s?s,s?o !}"
        , "label",&action->label, "info",&action->info, "callback",&callback, "lua", &lua, "api",&api, "verb", &verb, "args",&action->argsJ);
    if (err) {
        AFB_ERROR ("DISPATCH-LOAD-ACTION Missing something label|info|callback|lua|(api+verb)|args in %s", json_object_get_string(actionJ));
        goto OnErrorExit;
    }

    if (lua) {
        action->mode = CTL_MODE_LUA;
        action->call=lua;
        modeCount++;
    }

    if (api && verb) {
        action->mode = CTL_MODE_API;
        action->api=api;
        action->call=verb;
        modeCount++;
    }

    if (callback && controlConfig->dlHandle) {
        action->mode = CTL_MODE_CB;
        action->call=callback;
        modeCount++;

        action->actionCB = dlsym(controlConfig->dlHandle, callback);
        if (!action->actionCB) {
            AFB_ERROR ("DISPATCH-LOAD-ACTION fail to find calbback=%s in %s", callback, controlConfig->plugin);
            goto OnErrorExit;
        }
    }

    // make sure at least one mode is selected
    if (modeCount == 0) {
        AFB_ERROR ("DISPATCH-LOAD-ACTION No Action Selected lua|callback|(api+verb) in %s", json_object_get_string(actionJ));            
        goto OnErrorExit;
    } 

    if (modeCount > 1) {
        AFB_ERROR ("DISPATCH-LOAD-ACTION:ToMany arguments lua|callback|(api+verb) in %s", json_object_get_string(actionJ));            
        goto OnErrorExit;
   } 
   return 0;
   
OnErrorExit:
   return -1;   
};

STATIC DispatchActionT *DispatchLoadActions (DispatchConfigT *controlConfig, json_object *actionsJ) {
    int err;
    DispatchActionT *actions;
                
    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
        actions = calloc (count+1, sizeof(DispatchActionT));
        
        for (int idx=0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);
            err = DispatchLoadOneAction (controlConfig, actionJ, &actions[idx]);
            if (err) goto OnErrorExit;
        }
        
    } else {
        actions = calloc (2, sizeof(DispatchActionT));  
        err = DispatchLoadOneAction (controlConfig, actionsJ, &actions[0]);
        if (err) goto OnErrorExit;
    }
    
    return actions;
    
 OnErrorExit:
    return NULL;
    
}


STATIC DispatchConfigT *DispatchLoadConfig (const char* filepath) {
    json_object *controlConfigJ, *ignoreJ;
    int err;
    
    // Load JSON file
    controlConfigJ= json_object_from_file(filepath);
    if (!controlConfigJ) {
        AFB_ERROR ("DISPATCH-LOAD-CONFIG:JsonLoad invalid JSON %s ", filepath);
        goto OnErrorExit;
    }
    
    AFB_INFO ("DISPATCH-LOAD-CONFIG: loading config filepath=%s", filepath);
    
    json_object *metadataJ=NULL, *onloadJ=NULL, *controlsJ=NULL, *eventsJ=NULL;
    err= wrap_json_unpack(controlConfigJ, "{s?o,so,s?o,s?o,s?o !}", "$schema", &ignoreJ, "metadata",&metadataJ, "onload",&onloadJ, "controls",&controlsJ, "events",&eventsJ);
    if (err) {
        AFB_ERROR ("DISPATCH-LOAD-CONFIG Missing something metadata|[onload]|[controls]|[events] in %s", json_object_get_string(controlConfigJ));
        goto OnErrorExit;
    }
    
    DispatchConfigT *controlConfig = calloc (1, sizeof(DispatchConfigT));
    if (metadataJ) {
        err= wrap_json_unpack(metadataJ, "{ss,s?s,ss !}", "label", &controlConfig->label, "info",&controlConfig->info, "version",&controlConfig->version);
        if (err) {
            AFB_ERROR ("DISPATCH-LOAD-CONFIG:METADATA Missing something label|version|[label] in %s", json_object_get_string(metadataJ));
            goto OnErrorExit;
        }
    } 
    
    if (onloadJ) {
        json_object * actionsJ;
        DispatchHandleT *dispatchHandle = calloc (1, sizeof(DispatchHandleT));
        err= wrap_json_unpack(onloadJ, "{so,s?s,s?s,s?o !}", "label",&dispatchHandle->label, "info",&dispatchHandle->info, "plugin", &controlConfig->plugin, "actions",&actionsJ);
        if (err) {
            AFB_ERROR ("DISPATCH-LOAD-CONFIG:ONLOAD Missing something label|[info]|[plugin]|[actions] in %s", json_object_get_string(onloadJ));
            goto OnErrorExit;
        }
        
        if (controlConfig->plugin) {
            
            // search for default policy config file
            json_object *pluginPathJ = ScanForConfig(CONTROL_PLUGIN_PATH , CTL_SCAN_RECURSIVE, controlConfig->plugin, NULL);
            if (!pluginPathJ || json_object_array_length(pluginPathJ) == 0) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:PLUGIN Missing plugin=%s in path=%s", controlConfig->plugin, CONTROL_PLUGIN_PATH);
                goto OnErrorExit;                
            }
            
            char *filename; char*fullpath;
            err= wrap_json_unpack (json_object_array_get_idx(pluginPathJ,0), "{s:s, s:s !}", "fullpath",  &fullpath,"filename", &filename);
            if (err) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:PLUGIN HOOPs invalid plugin file path = %s", json_object_get_string(pluginPathJ));
                goto OnErrorExit;
            }
            
            if (json_object_array_length(pluginPathJ) > 1) {
                AFB_WARNING ("DISPATCH-LOAD-CONFIG:PLUGIN plugin multiple instances in searchpath will use %s/%s", fullpath, filepath);
            }
                
            char pluginpath[CONTROL_MAXPATH_LEN];
            strncpy(pluginpath, fullpath, sizeof(pluginpath)); 
            strncat(pluginpath, "/", sizeof(pluginpath)); 
            strncat(pluginpath, filename, sizeof(pluginpath)); 
            controlConfig->dlHandle = dlopen(pluginpath, RTLD_NOW);
            if (!controlConfig->dlHandle) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:PLUGIN Fail to load pluginpath=%s err= %s", pluginpath, dlerror());
                goto OnErrorExit;
            }
            
            CtlPluginMagicT *ctlPluginMagic = (CtlPluginMagicT*)dlsym(controlConfig->dlHandle, "CtlPluginMagic");
            if (!ctlPluginMagic || ctlPluginMagic->magic != CTL_PLUGIN_MAGIC) {
                AFB_ERROR("DISPATCH-LOAD-CONFIG:Plugin symbol'CtlPluginMagic' missing or !=  CTL_PLUGIN_MAGIC plugin=%s", pluginpath);
                goto OnErrorExit;
            } else {
                AFB_NOTICE("DISPATCH-LOAD-CONFIG:Plugin %s successfully registered", ctlPluginMagic->label);
            }
            
            // Jose hack to make verbosity visible from sharelib
            struct afb_binding_data_v2 *afbHidenData = dlsym(controlConfig->dlHandle, "afbBindingV2data");
            if (afbHidenData)  *afbHidenData = afbBindingV2data;
            
            DispatchPluginInstallCbT ctlPluginOnload = dlsym(controlConfig->dlHandle, "CtlPluginOnload");
            if (ctlPluginOnload) {
                controlConfig->context = (*ctlPluginOnload) (controlConfig->label, controlConfig->version, controlConfig->info);
            }
         }
        
        dispatchHandle->actions= DispatchLoadActions(controlConfig, actionsJ);
        if (!dispatchHandle->actions) {
            AFB_ERROR ("DISPATCH-LOAD-CONFIG:ONLOAD Error when parsing actions %s", dispatchHandle->label);
            goto OnErrorExit;                
        }
        controlConfig->onload= dispatchHandle;
        
    }
    
    if (controlsJ) {
        json_object * actionsJ;

        if (json_object_get_type(controlsJ) != json_type_array) {
            AFB_ERROR ("DISPATCH-LOAD-CONFIG:CONTROLS invalid JSON array in %s", json_object_get_string(controlsJ));
            goto OnErrorExit;            
        }
        
        int length= json_object_array_length(controlsJ);
        controlConfig->controls = (DispatchHandleT**) calloc (length+1, sizeof(void*));
        
        for (int jdx=0; jdx< length; jdx++) {
            DispatchHandleT *dispatchHandle = calloc (1, sizeof(DispatchHandleT));
            json_object *controlJ = json_object_array_get_idx(controlsJ,jdx);
            err= wrap_json_unpack(controlJ, "{ss,s?s,so !}", "label",&dispatchHandle->label, "info",&dispatchHandle->info, "actions",&actionsJ);
            if (err) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:CONTROLS Missing something label|[info]|actions in %s", json_object_get_string(controlJ));
                goto OnErrorExit;
            }
            
            dispatchHandle->actions= DispatchLoadActions(controlConfig, actionsJ);
            if (!dispatchHandle->actions) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:CONTROLS Error when parsing actions %s", dispatchHandle->label);
                goto OnErrorExit;                
            }
            controlConfig->controls[jdx]= dispatchHandle;            
        }
                
    }
    
    if (eventsJ) {
        json_object * actionsJ;
        DispatchHandleT *dispatchHandle = calloc (1, sizeof(DispatchHandleT));

        err= wrap_json_unpack(eventsJ, "{so,s?s,so !}", "label",&dispatchHandle->label, "info",&dispatchHandle->info, "actions",&actionsJ);
        if (err) {
            AFB_ERROR ("DISPATCH-LOAD-CONFIG:EVENTS Missing something label|[info]|actions in %s", json_object_get_string(eventsJ));
            goto OnErrorExit;
        }
        dispatchHandle->actions= DispatchLoadActions(controlConfig, actionsJ);
            if (!dispatchHandle->actions) {
                AFB_ERROR ("DISPATCH-LOAD-CONFIG:EVENTS Error when parsing actions %s", dispatchHandle->label);
                goto OnErrorExit;                
            }
        controlConfig->events= dispatchHandle;
    }
    
    return controlConfig;
    
OnErrorExit:
    return NULL;
}

// Load default config file at init
PUBLIC int DispatchInit () {
    int index, err;
    
    
    // search for default dispatch config file
    json_object* responseJ = ScanForConfig(CONTROL_CONFIG_PATH, CTL_SCAN_RECURSIVE,"onload", "json");
    
    for (index=0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ=json_object_array_get_idx(responseJ, index);
        
        char *filename; char*fullpath;
        err= wrap_json_unpack (entryJ, "{s:s, s:s !}", "fullpath",  &fullpath,"filename", &filename);
        if (err) {
            AFB_ERROR ("DISPATCH-INIT HOOPs invalid JSON entry= %s", json_object_get_string(entryJ));
            goto OnErrorExit;
        }
        
        if (strcasestr(filename, CONTROL_CONFIG_FILE)) {
            char filepath[CONTROL_MAXPATH_LEN];
            strncpy(filepath, fullpath, sizeof(filepath)); 
            strncat(filepath, "/", sizeof(filepath)); 
            strncat(filepath, filename, sizeof(filepath)); 
            configHandle = DispatchLoadConfig (filepath);
            if (!configHandle) {
                AFB_ERROR ("DISPATCH-INIT:ERROR No File to load [%s]", filepath);
                goto OnErrorExit;
            }
            break;
        }
    }
    
    // no dispatch config found remove control API from binder
    if (index == 0)  {
        AFB_WARNING ("DISPATCH-INIT:WARNING No Control dispatch file [%s]", CONTROL_CONFIG_FILE);
    }
    
    AFB_NOTICE ("DISPATCH-INIT:SUCCES: Audio Control Dispatch Init");
    return 0;
    
OnErrorExit:
    AFB_NOTICE ("DISPATCH-INIT:ERROR: Audio Control Dispatch Init");
    return 1;    
}
 

 
