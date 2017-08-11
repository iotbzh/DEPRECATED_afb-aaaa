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


#ifndef CONTROLER_BINDING_INCLUDE
#define CONTROLER_BINDING_INCLUDE

#define AFB_BINDING_VERSION 2
#include <afb/afb-binding.h>
#include <json-c/json.h>

#ifndef PUBLIC
  #define PUBLIC
#endif
#define STATIC static

// sharelib ctl-plugin*
typedef struct {
  long  magic;  
  char *label;  
} CtlPluginMagicT;

#define CTL_PLUGIN_REGISTER(pluglabel) CtlPluginMagicT CtlPluginMagic={.magic=CTL_PLUGIN_MAGIC,.label=pluglabel}; struct afb_binding_data_v2;

// polctl-binding.c
PUBLIC int CtlBindingInit ();

// ctl-timerevt.c
// ----------------------


PUBLIC int TimerEvtInit (void);
PUBLIC afb_event TimerEvtGet(void);
PUBLIC void ctlapi_event_test (afb_req request);

// ctl-policy
// -----------

typedef enum {
  CTL_SCAN_FLAT=0,        
  CTL_SCAN_RECURSIVE=1,
} CtlScanDirModeT;

typedef enum {
    CTL_MODE_NONE=0,
    CTL_MODE_API,
    CTL_MODE_CB,
    CTL_MODE_LUA,        
} CtlRequestModeT;

typedef struct DispatchActionS{
    const char *info;
    const char* label;
    CtlRequestModeT mode;
    const char* api;
    const char* call;
    json_object *argsJ;
    int timeout;
    int (*actionCB)(afb_req request, struct DispatchActionS *action, void *context);
} DispatchActionT;

PUBLIC int  DispatchInit(void);
PUBLIC void ctlapi_dispatch (char* control, afb_req request);

// ctl-lua.c

typedef enum {
    LUA_DOCALL,
    LUA_DOSTRING,
    LUA_DOSCRIPT,
} LuaDoActionT;

PUBLIC int LuaLibInit ();
PUBLIC json_object* ScanForConfig (char* searchPath, CtlScanDirModeT mode, char *pre, char *ext);
PUBLIC int LuaCallFunc (afb_req request, DispatchActionT *action);
PUBLIC void ctlapi_lua_docall (afb_req request);
PUBLIC void ctlapi_lua_dostring (afb_req request);
PUBLIC void ctlapi_lua_doscript (afb_req request);

#endif
