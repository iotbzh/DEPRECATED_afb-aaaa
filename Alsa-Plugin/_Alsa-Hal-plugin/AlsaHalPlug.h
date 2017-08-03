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
 * Syntaxe in .asoundrc file
 *   CrlLabel    { cb MyFunctionName name "My_Second_Control" }
 */


#include <alsa/asoundlib.h>
#include <alsa/control_external.h>
#include <linux/soundcard.h>

#ifndef SOUND_HAL_MAX_CTLS
#define SOUND_HAL_MAX_CTLS 255
#endif

typedef enum {
    CTLCB_INIT,
    CTLCB_CLOSE,
    CTLCB_ELEM_COUNT,
    CTLCB_ELEM_LIST,
    CTLCB_FIND_ELEM,
    CTLCB_FREE_KEY,
    CTLCB_GET_ATTRIBUTE,
    CTLCB_GET_INTEGER_INFO,
    CTLCB_GET_INTEGER64_INFO,
    CTLCB_GET_ENUMERATED_INFO,
    CTLCB_GET_ENUMERATED_NAME,
    CTLCB_READ_INTEGER,
    CTLCB_READ_INTEGER64,
    CTLCB_READ_ENUMERATED,
    CTLCB_READ_BYTES,
    CTLCB_READ_IEC958,
    CTLCB_WRITE_INTEGER,
    CTLCB_WRITE_INTEGER64,
    CTLCB_WRITE_ENUMERATED,
    CTLCB_WRITE_BYTES,
    CTLCB_WRITE_IEC958,
    CTLCB_SUBSCRIBE_EVENTS,
    CTLCB_READ_EVENT,
    CTLCB_POLL_DESCRIPTORS_COUNT,
    CTLCB_POLL_DESCRIPTORS
} snd_ctl_action_t;

typedef struct {
    int ctlNumid;
    const char *ctlName;
} snd_ctl_conf_t;

typedef struct {
    int type;
    int acc;
    unsigned count;
} snd_ctl_get_attrib_t;

typedef struct {
    int imin;
    int imax;
    int istep;
} snd_ctl_get_int_info_t;

typedef int(*snd_ctl_cb_t)(void *handle, snd_ctl_action_t action, snd_ctl_ext_key_t key, void *response);

typedef struct snd_ctl_hal {
    snd_ctl_ext_t ext;
    char *devid;
    snd_ctl_t *ctlDev;
    unsigned int ctlsCount;
    void *dlHandle;
    snd_ctl_conf_t ctls[SOUND_HAL_MAX_CTLS];
    snd_ctl_elem_info_t *infos[SOUND_HAL_MAX_CTLS];
    snd_ctl_cb_t cbs[SOUND_HAL_MAX_CTLS];
} snd_ctl_hal_t;


