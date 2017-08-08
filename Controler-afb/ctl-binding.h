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
#include <systemd/sd-event.h>

#include "audio-common.h"

#ifndef CONTROLER_BINDING_INCLUDE
#define CONTROLER_BINDING_INCLUDE

// polctl-binding.c
PUBLIC int CtlBindingInit ();

// ctl-timerevt.c
// ----------------------
#define DEFAULT_PAUSE_DELAY 3000
#define DEFAULT_TEST_COUNT 1
typedef int (*timerCallbackT)(void *context);
typedef struct {
    int value;
    const char *label;
} AutoTestCtxT;

typedef struct TimerHandleS {
    int count;
    int delay;
    AutoTestCtxT *context;
    timerCallbackT callback;
    sd_event_source *evtSource;
} TimerHandleT;

PUBLIC int TimerEvtInit (void);
PUBLIC afb_event TimerEvtGet(void);
PUBLIC void ctlapi_event_test (afb_req request);

// ctl-policy
// -----------

typedef struct PolicyActionS{
    const char* label;
    const char* api;
    const char* verb;
    json_object *argsJ;
    const char *info;
    int timeout;
    json_object*  (*actionCB)(struct PolicyActionS *action,json_object *response, void *context);
} PolicyActionT;

typedef struct {
    const char* label;
    const char *info;
    const char *version;
    void *context;
} PolicyHandleT;

typedef struct {
  char *sharelib;
  void *dlHandle;
  PolicyHandleT **handle;
  PolicyActionT **onload;
  PolicyActionT **events;
  PolicyActionT **controls;
} PolicyCtlConfigT;

typedef enum {
    CTLAPI_NAVIGATION,
    CTLAPI_MULTIMEDIA,
    CTLAPI_EMERGENCY,

    CTL_NONE=-1
} PolicyCtlEnumT;

PUBLIC int  PolicyInit(void);
PUBLIC json_object* ScanForConfig (char* searchPath, char * pre, char *ext);
PUBLIC void ctlapi_authorize (PolicyCtlEnumT control, afb_req request);

// ctl-lua.c
PUBLIC int LuaLibInit ();
PUBLIC void ctlapi_lua_docall (afb_req request);
PUBLIC void ctlapi_lua_dostring (afb_req request);
PUBLIC void ctlapi_lua_doscript (afb_req request);

#endif
