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
typedef struct {
    int value;
    const char *label;
} AutoTestCtxT;

static afb_event afbevt;

STATIC int TimerNext (sd_event_source* source, uint64_t timer, void* handle) {
    TimerHandleT *timerHandle = (TimerHandleT*) handle;
    int done;
    uint64_t usec;

    // Rearm timer if needed
    timerHandle->count --;
    if (timerHandle->count == 0) {
        sd_event_source_unref(source);
        free (handle);
        return 0;
    }
    else {
        // otherwise validate timer for a new run    
        sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
        sd_event_source_set_enabled(source, SD_EVENT_ONESHOT);
        sd_event_source_set_time(source, usec + timerHandle->delay*1000);
    }

    done= timerHandle->callback(timerHandle->context);
    if (!done) goto OnErrorExit;
    
    return 0;

OnErrorExit:
    AFB_WARNING("TimerNext Callback Fail Tag=%s", timerHandle->label);
    return -1;  
}

PUBLIC void TimerEvtStop(TimerHandleT *timerHandle) {

    sd_event_source_unref(timerHandle->evtSource);
    free (timerHandle);
}


PUBLIC void TimerEvtStart(TimerHandleT *timerHandle, timerCallbackT callback, void *context) {
    uint64_t usec;
    
    // populate CB handle
    timerHandle->callback=callback;
    timerHandle->context=context;
    
    // set a timer with ~250us accuracy 
    sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
    sd_event_add_time(afb_daemon_get_event_loop(), &timerHandle->evtSource, CLOCK_MONOTONIC, usec+timerHandle->delay*1000, 250, TimerNext, timerHandle);
}

PUBLIC afb_event TimerEvtGet(void) {
    return afbevt;
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
 
