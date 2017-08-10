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
    CTLAPI_NAVIGATION,
    CTLAPI_MULTIMEDIA,
    CTLAPI_EMERGENCY,

    CTL_NONE=-1
} DispatchCtlEnumT;


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
    json_object*  (*actionCB)(struct DispatchActionS *action,json_object *response, void *context);
} DispatchActionT;

PUBLIC int  DispatchInit(void);
PUBLIC void ctlapi_dispatch (DispatchCtlEnumT control, afb_req request);

// ctl-lua.c
PUBLIC int LuaLibInit ();
PUBLIC json_object* ScanForConfig (char* searchPath, char * pre, char *ext);
PUBLIC void ctlapi_lua_docall (afb_req request);
PUBLIC void ctlapi_lua_dostring (afb_req request);
PUBLIC void ctlapi_lua_doscript (afb_req request);

#endif
