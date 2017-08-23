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
 * AfbCallBack (snd_ctl_hal_t *handle, int numid, void **response);
 *  AfbHalInit is mandatory and called with numid=0
 *
 * Syntax in .asoundrc file
 *   CrlLabel    { cb MyFunctionName name "My_Second_Control" }
 *
 * Testing:
 *   amixer -Dagl_hal controls
 *   amixer -Dagl_hal cget name=My_Sample_Callback
 */


#include "AlsaHalPlug.h"
#include <stdio.h>

int AfbHalInitCB(void *handle, snd_ctl_action_t action, int numid, void*response) {
    snd_ctl_hal_t *plughandle = (snd_ctl_hal_t*) handle;

    fprintf(stdout, "\n  - CB AfbHalInit devid=%s\n", plughandle->devid);
    return 0;
}

int AfbHalSampleCB(void *handle, snd_ctl_action_t action, snd_ctl_ext_key_t key, void*response) {

    switch (action) {
        case CTLCB_GET_ATTRIBUTE:
        {
            fprintf(stdout, "  - AfbHalSampleCB CTLCB_GET_ATTRIBUTE numid=%d\n", (int) key + 1);
            snd_ctl_get_attrib_t *item = (snd_ctl_get_attrib_t*) response;
            item->type = 1;
            item->count = 2;
            item->acc = SND_CTL_EXT_ACCESS_READWRITE;
            break;
        }

        case CTLCB_GET_INTEGER_INFO:
        {
            fprintf(stdout, "  - AfbHalSampleCB CTLCB_GET_INTEGER_INFO numid=%d\n", (int) key + 1);
            snd_ctl_get_int_info_t *item = (snd_ctl_get_attrib_t*) response;
            item->istep = 10;
            item->imin = 20;
            item->imax = 200;
            break;
        }

        default:
            fprintf(stdout, "CB AfbHalSampleCB unsupported action=%d numid=%d\n", action, (int) key + 1);
    }
    return 0;
}
