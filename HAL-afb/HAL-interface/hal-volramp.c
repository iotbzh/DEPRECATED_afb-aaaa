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
 *
 */

#define _GNU_SOURCE  // needed for vasprintf
#include "hal-interface.h"

STATIC int RampTimerCB(sd_event_source* source, uint64_t timer, void* handle) {
    halVolRampT *volRamp = (halVolRampT*) handle;
    int err;
    uint64_t usec;

    // RampDown
    if (volRamp->current > volRamp->target) {
        volRamp->current = volRamp->current - volRamp->stepDown;
        if (volRamp->current < volRamp->target) volRamp->current = volRamp->target;
    }

    // RampUp
    if (volRamp->current < volRamp->target) {
        volRamp->current = volRamp->current + volRamp->stepUp;
        if (volRamp->current > volRamp->target) volRamp->current = volRamp->target;
    }

    // request current Volume Level
    err = halSetCtlByTag(volRamp->slave, volRamp->current);
    if (err) goto OnErrorExit;

    // we reach target stop volram event
    if (volRamp->current == volRamp->target) sd_event_source_unref(source);
    else {
        // otherwise validate timer for a new run
        sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
        sd_event_source_set_enabled(source, SD_EVENT_ONESHOT);
        err = sd_event_source_set_time(source, usec + volRamp->delay);
    }

    return 0;

OnErrorExit:
    AFB_WARNING("RampTimerCB Fail to set HAL ctl tag=%d vol=%d", Master_Playback_Volume, volRamp->current);
    sd_event_source_unref(source); // abandon VolRamp
    return -1;
}

STATIC void SetRampTimer(void *handle) {
    halVolRampT *volRamp = (halVolRampT*) handle;
    uint64_t usec;

    // set a timer with ~250us accuracy
    sd_event_now(afb_daemon_get_event_loop(), CLOCK_MONOTONIC, &usec);
    sd_event_add_time(afb_daemon_get_event_loop(), &volRamp->evtsrc, CLOCK_MONOTONIC, usec, 250, RampTimerCB, volRamp);
}

STATIC int volumeDoRamp(halVolRampT *volRamp, int numid, json_object *volumeJ) {
    json_object *responseJ;

    // request current Volume Level
    responseJ = halGetCtlByTag(volRamp->slave);
    if (!responseJ) {
        AFB_WARNING("volumeDoRamp Fail to get HAL ctl tag=%d", Master_Playback_Volume);
        goto OnErrorExit;
    }

    // use 1st volume value as target for ramping
    switch (json_object_get_type(volumeJ)) {
        case json_type_array:
            volRamp->target = json_object_get_int(json_object_array_get_idx(volumeJ, 0));
            break;

        case json_type_int:
            volRamp->target = json_object_get_int(volumeJ);
            break;

        default:
            AFB_WARNING("volumeDoRamp Invalid volumeJ=%s", json_object_get_string(volumeJ));
            goto OnErrorExit;
    }

    // use 1st volume value as current for ramping
    switch (json_object_get_type(responseJ)) {
        case json_type_array:
            volRamp->current = json_object_get_int(json_object_array_get_idx(responseJ, 0));
            break;

        case json_type_int:
            volRamp->current = json_object_get_int(responseJ);
            break;

        default:
            AFB_WARNING("volumeDoRamp Invalid reponseJ=%s", json_object_get_string(responseJ));
            goto OnErrorExit;
    }

    SetRampTimer(volRamp);

    return 0;

OnErrorExit:
    return -1;
}

PUBLIC void volumeRamp(halCtlsTagT halTag, alsaHalCtlMapT *ctl, void* handle, json_object *valJ) {
    halVolRampT *volRamp = (halVolRampT*) handle;
    json_object *tmpJ;

    if (json_object_get_type(valJ) != json_type_array || volRamp == NULL) goto OnErrorExit;

    switch (halTag) {

            // Only config use wellknown tag. Default is DoVolRamp
        default:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volumeDoRamp(volRamp, ctl->numid, tmpJ);
            break;

        case Vol_Ramp_Set_Mode:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volRamp->mode = json_object_get_int(tmpJ);
            switch (volRamp->mode) {

                case RAMP_VOL_SMOOTH:
                    volRamp->delay = 100 * 1000;
                    volRamp->stepDown = 1;
                    volRamp->stepUp = 1;
                    break;

                case RAMP_VOL_NORMAL:
                    volRamp->delay = 100 * 1000;
                    volRamp->stepDown = 3;
                    volRamp->stepUp = 2;
                    break;

                case RAMP_VOL_EMERGENCY:
                    volRamp->delay = 50 * 1000;
                    volRamp->stepDown = 6;
                    volRamp->stepUp = 2;
                    break;

                default:
                    goto OnErrorExit;
            }
            break;

        case Vol_Ramp_Set_Slave:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volRamp->slave = json_object_get_int(tmpJ);
            break;

        case Vol_Ramp_Set_Delay:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volRamp->delay = 1000 * json_object_get_int(tmpJ);
            break;

        case Vol_Ramp_Set_Down:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volRamp->stepDown = json_object_get_int(tmpJ);
            break;

        case Vol_Ramp_Set_Up:
            tmpJ = json_object_array_get_idx(valJ, 0);
            volRamp->stepUp = json_object_get_int(tmpJ);
            break;
    }

    return;

OnErrorExit:
    AFB_WARNING("volumeRamp: Invalid Ctrl Event halCtlsTagT=%d numid=%d name=%s value=%s", halTag, ctl->numid, ctl->name, json_object_get_string(valJ));
    return;
}
