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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <systemd/sd-event.h>

#include "ctl-binding.h"

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

static afb_event afbevt;

STATIC int TimerNext (sd_event_source* source, uint64_t timer, void* handle) {
    TimerHandleT *timerHandle = (TimerHandleT*) handle;
    int done;
    uint64_t usec;

    // Rearm timer if needed
    timerHandle->count --;
    if (timerHandle->count == 0) sd_event_source_unref(source);
    else {
        // otherwise validate timer for a new run    
        sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
        sd_event_source_set_enabled(source, SD_EVENT_ONESHOT);
        sd_event_source_set_time(source, usec + timerHandle->delay);
    }

    done= timerHandle->callback(timerHandle->context);
    if (!done) goto OnErrorExit;
    
    return 0;

OnErrorExit:
    AFB_WARNING("TimerNext Callback Fail Tag=%s", timerHandle->context->label);
    return -1;  
}


STATIC int DoSendEvent (void *context) {
    AutoTestCtxT *ctx= (AutoTestCtxT*)context;
    json_object *ctlEventJ;
    
    if (ctx->value) ctx->value =0;
    else ctx->value =1;
    
    ctlEventJ = json_object_new_object();
    json_object_object_add(ctlEventJ,"signal", json_object_new_string(ctx->label));
    json_object_object_add(ctlEventJ,"value" , json_object_new_int(ctx->value));
    int done = afb_event_push(afbevt, ctlEventJ);
    
    AFB_NOTICE ("DoSendEvent {action: '%s', value:%d} status=%d", ctx->label, ctx->value, done);
    
    return (done);
}

STATIC void TimerEvtStart(TimerHandleT *timerHandle, void *context) {
    uint64_t usec;
    
    // populate CB handle
    timerHandle->callback=DoSendEvent;
    timerHandle->context=context;
    
    // set a timer with ~250us accuracy 
    sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
    sd_event_add_time(afb_daemon_get_event_loop(), &timerHandle->evtSource, CLOCK_MONOTONIC, usec+timerHandle->delay, 250, TimerNext, timerHandle);
}

PUBLIC afb_event TimerEvtGet(void) {
    return afbevt;
}


// Generated some fake event based on watchdog/counter
PUBLIC void ctlapi_event_test (afb_req request) {
    json_object *queryJ, *tmpJ;
    TimerHandleT *timerHandle = malloc (sizeof (TimerHandleT));
    AutoTestCtxT *context = calloc (1, sizeof (AutoTestCtxT));
    int done;
       
    queryJ= afb_req_json(request);
    
    // Closing call only has one parameter
    done=json_object_object_get_ex(queryJ, "closing", &tmpJ);
    if (done) return;
    
    done=json_object_object_get_ex(queryJ, "label", &tmpJ);
    if (!done) {
         afb_req_fail_f(request, "TEST-LABEL-MISSING", "label is mandatory for event_test");
        goto OnErrorExit;
    }
    context->label = strdup(json_object_get_string (tmpJ));
    
    json_object_object_get_ex(queryJ, "delay", &tmpJ);
    timerHandle->delay = json_object_get_int (tmpJ) * 1000;
    if (timerHandle->delay == 0) timerHandle->delay=DEFAULT_PAUSE_DELAY * 1000;
    
    json_object_object_get_ex(queryJ, "count", &tmpJ);
    timerHandle->count = json_object_get_int (tmpJ);
    if (timerHandle->count == 0) timerHandle->count=DEFAULT_TEST_COUNT;
    
    // start a lool timer  
    TimerEvtStart (timerHandle, context);
    
    afb_req_success(request, NULL, NULL);
    return;
    
 OnErrorExit:    
    return;
}

// Create Binding Event at Init
PUBLIC int TimerEvtInit () {
    
    // create binder event to send test pause/resume
    afbevt = afb_daemon_make_event("control");
    if (!afb_event_is_valid(afbevt)) {
        AFB_ERROR ("POLCTL_INIT: Cannot register ctl-events");
        return 1;
    }
    
    AFB_DEBUG ("Audio Control-Events Init Done");
    return 0;
}
 
