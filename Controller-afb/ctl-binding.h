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
 */


#ifndef CONTROLLER_BINDING_INCLUDE
#define CONTROLLER_BINDING_INCLUDE

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <json-c/json.h>
#include <filescan-utils.h>
#include <wrap-json.h>
#include <systemd/sd-event.h>


#ifdef CONTROL_SUPPORT_LUA
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
#endif

#ifndef PUBLIC
  #define PUBLIC
#endif
#define STATIC static

#ifndef UNUSED_ARG
#define UNUSED_ARG(x) UNUSED_ ## x __attribute__((__unused__))
#define UNUSED_FUNCTION(x) __attribute__((__unused__)) UNUSED_ ## x
#endif

// polctl-binding.c
PUBLIC int CtlBindingInit ();

// ctl-timerevt.c
// ----------------------
typedef int (*timerCallbackT)(void *context);

typedef struct TimerHandleS {
    int count;
    int delay;
    const char*label;
    void *context;
    timerCallbackT callback;
    sd_event_source *evtSource;
} TimerHandleT;

PUBLIC int TimerEvtInit (void);
PUBLIC afb_event TimerEvtGet(void);
PUBLIC void TimerEvtStart(TimerHandleT *timerHandle, timerCallbackT callback, void *context);
PUBLIC void TimerEvtStop(TimerHandleT *timerHandle);

// ctl-policy
// -----------

typedef enum {
    CTL_MODE_NONE=0,
    CTL_MODE_API,
    CTL_MODE_CB,
    CTL_MODE_LUA,        
} CtlRequestModeT;


typedef enum {
    CTL_SOURCE_CLOSE=-1,
    CTL_SOURCE_UNKNOWN=0,
    CTL_SOURCE_ONLOAD=1,
    CTL_SOURCE_OPEN=2,
    CTL_SOURCE_EVENT=3,
} DispatchSourceT;

typedef struct DispatchActionS{
    const char *info;
    const char* label;
    CtlRequestModeT mode;
    const char* api;
    const char* call;
    json_object *argsJ;
    int timeout;
    int (*actionCB)(DispatchSourceT source, const char*label,  json_object *argsJ, json_object *queryJ, void *context);
} DispatchActionT;

typedef int (*Lua2cFunctionT)(char *funcname, json_object *argsJ, void*context);

PUBLIC int  DispatchInit(void);
PUBLIC int DispatchOnLoad(const char *onLoadLabel);
PUBLIC void DispatchOneEvent(const char *evtLabel, json_object *eventJ);
PUBLIC int DispatchOneL2c(lua_State* luaState, char *funcname, Lua2cFunctionT callback);
PUBLIC void ctlapi_dispatch (afb_req request);

#ifdef CONTROL_SUPPORT_LUA
// ctl-lua.c
typedef int (*Lua2cWrapperT) (lua_State* luaState, char *funcname, Lua2cFunctionT callback);

#define CTLP_LUA2C(FuncName, label,argsJ, context) static int FuncName(char*label,json_object*argsJ, void*context);\
        int lua2c_ ## FuncName(lua_State* luaState){return((*Lua2cWrap)(luaState, MACRO_STR_VALUE(FuncName), FuncName));};\
        static int FuncName(char* label, json_object* argsJ, void* context)

typedef enum {
    LUA_DOCALL,
    LUA_DOSTRING,
    LUA_DOSCRIPT,
} LuaDoActionT;

PUBLIC int LuaLibInit ();
PUBLIC void LuaL2cNewLib(const char *label, luaL_Reg *l2cFunc, int count);
PUBLIC int Lua2cWrapper(lua_State* luaState, char *funcname, Lua2cFunctionT callback, void *context);
PUBLIC int LuaCallFunc (DispatchSourceT source, DispatchActionT *action, json_object *queryJ) ;
PUBLIC void ctlapi_lua_docall (afb_req request);
PUBLIC void ctlapi_lua_dostring (afb_req request);
PUBLIC void ctlapi_lua_doscript (afb_req request);

#else
 typedef void* Lua2cWrapperT;
#endif // CONTROL_SUPPORT_LUA


// sharelib ctl-plugin*
typedef struct {
  long  magic;  
  char *label;  
} CtlPluginMagicT;


#define MACRO_STR_VALUE(arg) #arg
#define CTLP_REGISTER(pluglabel) CtlPluginMagicT CtlPluginMagic={.magic=CTL_PLUGIN_MAGIC,.label=pluglabel}; struct afb_binding_data_v2; Lua2cWrapperT Lua2cWrap;
#define CTLP_ONLOAD(label,version,info) void* CtlPluginOnload(char* label, char* version, char* info) 
#define CTLP_CAPI(funcname,source, label,argsJ, queryJ, context) int funcname(DispatchSourceT source, char* label, json_object* argsJ, json_object* queryJ, void* context)



#endif // CONTROLLER_BINDING_INCLUDE
