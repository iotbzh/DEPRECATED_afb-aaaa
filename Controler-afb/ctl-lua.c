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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ref: 
 *  http://www.troubleshooters.com/codecorn/lua/lua_c_calls_lua.htm#_Anatomy_of_a_Lua_Call
 *  http://acamara.es/blog/2012/08/passing-variables-from-lua-5-2-to-c-and-vice-versa/
 *  https://john.nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dirent.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#include "ctl-binding.h"
#include "wrap-json.h"


static  lua_State* luaState;

typedef enum {
    LUA_DOCALL,
    LUA_DOSTRING,
    LUA_DOSCRIPT,
} LuaDoActionT;

#define CTX_MAGIC 123456789
#define CTX_TOKEN "AFB_ctx"

typedef struct {
    int ctxMagic;
    afb_req request;
    char *info;
} LuaAfbContextT;

typedef struct {
  const char *callback;
  const char *handle;  
} LuaCallServiceT;

typedef enum {
    AFB_MSG_WARNING,
    AFB_MSG_NOTICE,      
} LuaAfbMessageT;

STATIC LuaAfbContextT *LuaCtxCheck (lua_State *luaState, int index) {
  LuaAfbContextT *afbContext;
  luaL_checktype(luaState, index, LUA_TUSERDATA);
  afbContext = (LuaAfbContextT *)luaL_checkudata(luaState, index, CTX_TOKEN);
  if (afbContext == NULL && afbContext->ctxMagic != CTX_MAGIC) {
      luaL_error(luaState, "Fail to retrieve user data context=%s", CTX_TOKEN);
      AFB_ERROR ("afbContextCheck error retrieving afbContext");
      return NULL;
  }
  return afbContext;
}

STATIC LuaAfbContextT *LuaCtxPush (lua_State *luaState, afb_req request, const char* info) {
  LuaAfbContextT *afbContext = (LuaAfbContextT *)lua_newuserdata(luaState, sizeof(LuaAfbContextT));
  if (!afbContext) {
      AFB_ERROR ("LuaCtxPush fail to allocate user data context");
      return NULL;
  }
  luaL_setmetatable(luaState, CTX_TOKEN);
  afbContext->ctxMagic=CTX_MAGIC;
  afbContext->info=strdup(info);
  afbContext->request= request;
  return afbContext;
}

STATIC void LuaCtxFree (LuaAfbContextT *afbContext) {
    
    free (afbContext->info);
}

// List Avaliable Configuration Files
PUBLIC json_object* ScanForConfig (char* searchPath, char *pre, char *ext) {
    json_object *responseJ;
    DIR  *dirHandle;
    char *dirPath;
    char* dirList= strdup(searchPath);
    size_t extLen = strlen(ext);
    
    responseJ = json_object_new_array();
    for (dirPath= strtok(dirList, ":"); dirPath && *dirPath; dirPath=strtok(NULL,":")) {
         struct dirent *dirEnt;
         
        dirHandle = opendir (dirPath);
        if (!dirHandle) {
            AFB_NOTICE ("CONFIG-SCANNING dir=%s not readable", dirPath);
            continue;
        } 
        
        AFB_NOTICE ("CONFIG-SCANNING:ctl_listconfig scanning: %s", dirPath);
        while ((dirEnt = readdir(dirHandle)) != NULL) {
            // Unknown type is accepted to support dump filesystems
            if (dirEnt->d_type == DT_REG || dirEnt->d_type == DT_UNKNOWN) {
                
                // check prefix and extention
                size_t extIdx=strlen(dirEnt->d_name) - extLen;
                if (extIdx <= 0) continue; 
                if (pre && !strcasestr (dirEnt->d_name, pre)) continue;    
                if (ext && strcasecmp (ext, &dirEnt->d_name[extIdx])) continue;    
                
                struct json_object *pathJ = json_object_new_object();
                json_object_object_add(pathJ, "dirpath", json_object_new_string(dirPath));
                json_object_object_add(pathJ, "filename", json_object_new_string(dirEnt->d_name));
                json_object_array_add(responseJ, pathJ);
            }
        }
    }
    
    free (dirList);    
    return (responseJ);
}

STATIC int LuaPrintMessage(LuaAfbMessageT action, lua_State* luaState) {
    
    int count = lua_gettop(luaState);
    for (int idx = 1; idx <= count; ++idx) {
        const char *str = lua_tostring(luaState, idx); // Get string
        
        switch (action) {
            case AFB_MSG_NOTICE:
                AFB_NOTICE(str);
                break;
                
            case AFB_MSG_WARNING:
                AFB_WARNING(str);
                break;
             
            default:
                AFB_ERROR(str);
        }
    }
    return 0; // no value return
}

STATIC int LuaPrintWarning(lua_State* luaState) { 
 
    LuaPrintMessage (AFB_MSG_WARNING, luaState); 
    return 0; // no value return
}

STATIC int LuaPrintNotice(lua_State* luaState) { 
 
    LuaPrintMessage (AFB_MSG_NOTICE, luaState); 
    return 0; // no value return
}

STATIC int LuaPushArgument (json_object *arg) {
    
    switch (json_object_get_type(arg)) {
        case json_type_object:
            lua_newtable (luaState);
            json_object_object_foreach (arg, key, val) {
                int done = LuaPushArgument (val);
                if (done) {
                    lua_pushstring(luaState, key); // table.key = val
                    lua_settable(luaState, -3);  
                }
            }

            break;
        case json_type_int:
            lua_pushinteger(luaState, json_object_get_int(arg));
            break;
        case json_type_string:
            lua_pushstring(luaState, json_object_get_string(arg));
            break;
        case json_type_boolean:
            lua_pushboolean(luaState, json_object_get_boolean(arg));
            break;
        case json_type_double:
            lua_pushnumber(luaState, json_object_get_double(arg));
            break;
        default:
            return 0;
    }
    
    return 1;
}

static json_object *LuaPopArgs (lua_State* luaState, int count);
STATIC  json_object *PopOneArg (lua_State* luaState, int idx);
static json_object *LuaTableToJson (lua_State* luaState, int index) {

    json_object *tableJ= json_object_new_object();
    const char *key;
    char number[3];
    lua_pushnil(luaState); // 1st key
    for (int jdx=1; lua_next(luaState, index) != 0; jdx++) {

        printf("jdx=%d %s - %s\n", jdx,
        lua_typename(luaState, lua_type(luaState, -2)),
        lua_typename(luaState, lua_type(luaState, -1)));

        // uses 'key' (at index -2) and 'value' (at index -1)
        if (lua_type(luaState,-2) == LUA_TSTRING) key= lua_tostring(luaState, -2);
        else {
            snprintf(number, sizeof(number),"%d", jdx);
            key=number;
        } 
         
        json_object_object_add(tableJ, key, PopOneArg(luaState, -1));
        lua_pop(luaState, 1); // removes 'value'; keeps 'key' for next iteration 
    } 
    
    return tableJ;
}

STATIC  json_object *PopOneArg (lua_State* luaState, int idx) {
    json_object *value=NULL;

    switch(lua_type(luaState, idx))  {
        case LUA_TNUMBER:
            value= json_object_new_double(lua_tonumber(luaState, idx));
            break;
        case LUA_TBOOLEAN:
            value=  json_object_new_boolean(lua_toboolean(luaState, idx));
            break;
        case LUA_TSTRING:
           value=  json_object_new_string(lua_tostring(luaState, idx));
            break;
        case LUA_TTABLE: {
    AFB_NOTICE ("-++-- idx=%d ", idx);
            value= LuaTableToJson(luaState, idx);
            break;                
        }    
        default:
            AFB_NOTICE ("script returned Unknown/Unsupported idx=%d type: %s", idx, lua_typename(luaState, idx));
            value=NULL;
    }
    if (value) AFB_NOTICE ("---- idx=%d value=%s", idx, json_object_get_string(value));

    return value;
}
    
static json_object *LuaPopArgs (lua_State* luaState, int count) {


    json_object *responseJ;
    
    if(count <=0) return NULL;
    
    // start at 2 because we are using a function array lib
    if (count == 2) {
        responseJ=PopOneArg (luaState, 2);
    } else {
        // loop on remaining return arguments
        responseJ= json_object_new_array();
        for (int idx=2; idx <= count; idx++) {
            json_object_array_add(responseJ, PopOneArg (luaState, idx));     
       }
    }
    
    return responseJ;
}


STATIC int LuaAfbSuccess(lua_State* luaState) {

    
    int count = lua_gettop(luaState);


    //AFB_NOTICE ("LuaAfbSuccess args=%s",json_object_get_string(LuaPopArgs(luaState, count)));
    
    LuaAfbContextT *afbContext= LuaCtxCheck(luaState, 1);
    if (!afbContext) goto OnErrorExit;
        
    afb_req_success(afbContext->request, LuaPopArgs(luaState, count), NULL);

    LuaCtxFree(afbContext);
    return 0;
 
 OnErrorExit:  
        lua_error(luaState);
        return 1;
}

STATIC int LuaAfbFail(lua_State* luaState) {

    LuaAfbContextT *afbContext= LuaCtxCheck(luaState, 1);
    if (!afbContext) goto OnErrorExit;
    
    int count = lua_gettop(luaState);

    afb_req_fail(afbContext->request, afbContext->info, json_object_get_string(LuaPopArgs(luaState, count)));
    
    LuaCtxFree(afbContext);    
    return 0;   

 OnErrorExit:  
        lua_error(luaState);
        return 1;
}

STATIC void LuaAfbServiceCB(void *handle, int iserror, struct json_object *responseJ) { 
    LuaCallServiceT *contextCB= (LuaCallServiceT*)handle;
    
    // load function (should exist in CONTROL_PATH_LUA
    lua_getglobal(luaState, contextCB->callback);

    // push error status & response
    lua_pushboolean(luaState, iserror);
    (void)LuaPushArgument(responseJ);
    
    int err=lua_pcall(luaState, 2, LUA_MULTRET, 0);
    if (err) {
        AFB_ERROR ("LUA-SERICE-CB:FAIL response=%s err=%s", json_object_get_string(responseJ), lua_tostring(luaState,-1) );
    }
}

STATIC int LuaAfbService(lua_State* luaState) {
    int count = lua_gettop(luaState);
    
    // retrieve userdate context
    LuaAfbContextT *afbContext= luaL_checkudata(luaState, 1, "CTX_TOKEN");
    if (!afbContext) {
        lua_pushliteral (luaState, "LuaAfbServiceCall-Hoops no CTX_TOKEN");
        goto OnErrorExit;
    }
    
    if (count <5 || !lua_isstring(luaState, 2) || !lua_isstring(luaState, 3) || !lua_istable(luaState, 4) || !lua_isstring(luaState, 5)) {
        lua_pushliteral (luaState, "LuaAfbServiceCall-Syntax is AFB_service_call (api, verb, query, callback, handle");
        goto OnErrorExit;        
    }
    
    LuaCallServiceT *contextCB = calloc (1, sizeof(LuaCallServiceT));  
    
    // get api/verb+query
    const char *api = lua_tostring(luaState,1);
    const char *verb= lua_tostring(luaState,2);    
    json_object *queryJ= LuaTableToJson(luaState, 3);
    if (!queryJ) goto OnErrorExit;
    
    if (count >= 6) {
        if (!lua_istable(luaState, 6)) {
            lua_pushliteral (luaState, "LuaAfbServiceCall-Syntax optional handle should be a table");
            goto OnErrorExit;                    
        }
        contextCB->handle= lua_tostring(luaState, 5);
    }    
        
    afb_service_call(api, verb, queryJ, LuaAfbServiceCB, contextCB);
    
    return 0; // no value return
 
  OnErrorExit:  
        lua_error(luaState);
        return 1;

}


// Generated some fake event based on watchdog/counter
PUBLIC void LuaDoAction (LuaDoActionT action, afb_req request) {
    
    int err, count=0;

    json_object* queryJ = afb_req_json(request);  
    
    switch (action) {
        
        case LUA_DOSTRING: {
            const char *script = json_object_get_string(queryJ);
            err=luaL_loadstring(luaState, script);
            if (err) {
                AFB_ERROR ("LUA-DO-COMPILE:FAIL String=%s err=%s", script, lua_tostring(luaState,-1) );
                goto OnErrorExit;
            }
            // Push AFB client context on the stack
            LuaAfbContextT *afbContext= LuaCtxPush(luaState, request, script);
            if (!afbContext) goto OnErrorExit;
            
            break;
        }
    
        case LUA_DOCALL: {
            const char *func;
            json_object *args;
            err= wrap_json_unpack (queryJ, "{s:s, s?o !}", "func",  &func,"args", &args);
            if (err) {
                AFB_ERROR ("LUA-DOCALL-SYNTAX missing func|args args=%s", json_object_get_string(queryJ));
                goto OnErrorExit;
            }

            // Push AFB client context on the stack
            LuaAfbContextT *afbContext= LuaCtxPush(luaState, request, func);
            if (!afbContext) goto OnErrorExit;
            
            // load function (should exist in CONTROL_PATH_LUA
            lua_getglobal(luaState, func);
            
            
            
            // push arguments on the stack
            if (json_object_get_type(args) != json_type_array) { 
                count= LuaPushArgument (args);
            } else {
                for (int idx=0; idx<json_object_array_length(args); idx++)  {
                    count += LuaPushArgument (json_object_array_get_idx(args, idx));
                    if (err) break;
                }
            } 
                
            break;
        }
            
        case LUA_DOSCRIPT: {
            const char *script;
            json_object *args;
            int index;
            
            // scan luascript search path once
            static json_object *luaScriptPathJ =NULL;
            if (!luaScriptPathJ)  luaScriptPathJ= ScanForConfig(CONTROL_PATH_LUA , NULL, "lua");

            err= wrap_json_unpack (queryJ, "{s:s, s?o s?o !}", "script", &script,"args", &args, "arg", &args);
            if (err) {
                AFB_ERROR ("LUA-DOCALL-SYNTAX missing script|(args,arg) args=%s", json_object_get_string(queryJ));
                goto OnErrorExit;
            }
            
            // push arguments on the stack
            if (json_object_get_type(args) != json_type_array) { 
                lua_createtable(luaState, 1, 0);
                count= LuaPushArgument (args);
            } else {
                int length= json_object_array_length(args);
                lua_createtable(luaState, length, 0);
                for (int idx=0; idx < length; idx++)  {
                    count += LuaPushArgument (json_object_array_get_idx(args, idx));
                    if (err) break;
                }
            } 

            LuaAfbContextT *afbContext= LuaCtxPush(luaState, request, script);
            if (!afbContext) goto OnErrorExit;
            
            for (index=0; index < json_object_array_length(luaScriptPathJ); index++) {
                char *filename; char*dirpath;
                json_object *entryJ=json_object_array_get_idx(luaScriptPathJ, index);

                err= wrap_json_unpack (entryJ, "{s:s, s:s !}", "dirpath",  &dirpath,"filename", &filename);
                if (err) {
                    AFB_ERROR ("LUA-DOSCRIPT HOOPs invalid LUA script path = %s", json_object_get_string(entryJ));
                    goto OnErrorExit;
                }
                
                if (strcmp(filename, script)) continue;

                char filepath[255];
                strncpy(filepath, dirpath, sizeof(filepath)); 
                strncat(filepath, "/", sizeof(filepath)); 
                strncat(filepath, filename, sizeof(filepath)); 
                err= luaL_loadfile(luaState, filepath);   
                if (err) {
                    AFB_ERROR ("LUA-DOSCRIPT HOOPs Error in LUA loading scripts=%s err=%s", filepath, lua_tostring(luaState,-1));
                    goto OnErrorExit;
                }
                break;
            }
        
            if (index == json_object_array_length(luaScriptPathJ)) {
                AFB_ERROR ("LUA-DOSCRIPT HOOPs script=%s not in path=%s", script, CONTROL_PATH_LUA);
                goto OnErrorExit;

            }           
            break;
        }
                
        default: 
            AFB_ERROR ("LUA-DOCALL-ACTION unknown query=%s", json_object_get_string(queryJ));
            goto OnErrorExit;
    }

        
    // effectively exec LUA code (afb_reply/fail done later from callback) 
    err=lua_pcall(luaState, count, 0, 0);
    if (err) {
        AFB_ERROR ("LUA-DO-EXEC:FAIL query=%s err=%s", json_object_get_string(queryJ), lua_tostring(luaState,-1) );
        goto OnErrorExit;
    }

    return;
    
 OnErrorExit:
    afb_req_fail(request,"LUA:ERROR", lua_tostring(luaState,-1));
    return;
}

PUBLIC void ctlapi_lua_dostring (afb_req request) {       
    LuaDoAction (LUA_DOSTRING, request);
}

PUBLIC void ctlapi_lua_docall (afb_req request) {
    LuaDoAction (LUA_DOCALL, request);
}

PUBLIC void ctlapi_lua_doscript (afb_req request) {
    LuaDoAction (LUA_DOSCRIPT, request);
}

static const luaL_Reg afbFunction[] = {
    {"notice" , LuaPrintNotice},
    {"warning", LuaPrintWarning},
    {"service", LuaAfbService},
    {"success", LuaAfbSuccess},
    {"fail"   , LuaAfbFail},
    {NULL, NULL}  /* sentinel */
};

// Create Binding Event at Init
PUBLIC int LuaLibInit () {
    int err, index;
          
    // search for default policy config file
    json_object *luaScriptPathJ = ScanForConfig(CONTROL_PATH_LUA , "onload", "lua");
    
    // open a new LUA interpretor
    luaState = luaL_newstate();
    if (!luaState)  {
        AFB_ERROR ("LUA_INIT: Fail to open lua interpretor");
        goto OnErrorExit;
    }
    
    // load auxiliary libraries
    luaL_openlibs(luaState);

    // redirect print to AFB_NOTICE    
    luaL_newlib(luaState, afbFunction);
    lua_setglobal(luaState, "AFB");
    
    // load+exec any file found in LUA search path
    for (index=0; index < json_object_array_length(luaScriptPathJ); index++) {
        json_object *entryJ=json_object_array_get_idx(luaScriptPathJ, index);
        
        char *filename; char*dirpath;
        err= wrap_json_unpack (entryJ, "{s:s, s:s !}", "dirpath",  &dirpath,"filename", &filename);
        if (err) {
            AFB_ERROR ("LUA-INIT HOOPs invalid config file path = %s", json_object_get_string(entryJ));
            goto OnErrorExit;
        }
        
        char filepath[255];
        strncpy(filepath, dirpath, sizeof(filepath)); 
        strncat(filepath, "/", sizeof(filepath)); 
        strncat(filepath, filename, sizeof(filepath)); 
        err= luaL_loadfile(luaState, filepath);   
        if (err) {
            AFB_ERROR ("LUA-LOAD HOOPs Error in LUA loading scripts=%s err=%s", filepath, lua_tostring(luaState,-1));
            goto OnErrorExit;
        }
        
        // exec/compil script
        err = lua_pcall(luaState, 0, 0, 0);
        if (err) {
            AFB_ERROR ("LUA-LOAD HOOPs Error in LUA exec scripts=%s err=%s", filepath, lua_tostring(luaState,-1));
            goto OnErrorExit;
        }
        
    }
    
    // no policy config found remove control API from binder
    if (index == 0)  {
        AFB_WARNING ("POLICY-INIT:WARNING No Control LUA file in path=[%s]", CONTROL_PATH_LUA);
    }
    
    
    AFB_DEBUG ("Audio control-LUA Init Done");
    return 0;
    
 OnErrorExit:    
    return -1;
}
 