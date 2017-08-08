/*
 * Copyright (C) 2017, Microchip Technology Inc. and its subsidiaries.
 * Author Tobias Jahnke
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
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h"
#include "wrap-json.h"

static int master_volume;
static json_bool master_switch;
static int pcm_volume[6];

void unicens_master_vol_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[i!]", &master_volume) == 0) {
        AFB_NOTICE("master_volume: %s, value=%d", j_str, master_volume);
    }
    else {
        AFB_NOTICE("master_volume: INVALID STRING %s", j_str);
    } 
}

void unicens_master_switch_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[b!]", &master_switch) == 0) {
        AFB_NOTICE("master_switch: %s, value=%d", j_str, master_switch);
    }
    else {
        AFB_NOTICE("master_switch: INVALID STRING %s", j_str);
    }    
}

void unicens_pcm_vol_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[iiiiii!]", &pcm_volume[0], &pcm_volume[1], &pcm_volume[2], &pcm_volume[3],
                                             &pcm_volume[4], &pcm_volume[5]) == 0) {
        AFB_NOTICE("pcm_vol: %s", j_str);
    }
    else {
        AFB_NOTICE("pcm_vol: INVALID STRING %s", j_str);
    }
}

/* declare ALSA mixer controls */
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .tag=Master_Playback_Volume, .cb={.callback=unicens_master_vol_cb, .handle=&master_volume}, .info="Sets master playback volume",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .value=50, .name="Master Playback Volume"}   
  },
  { .tag=Master_OnOff_Switch, .cb={.callback=unicens_master_switch_cb, .handle=&master_switch}, .info="Sets master playback switch",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_BOOLEAN, .count=1, .minval=0, .maxval=1, .step=1, .value=1, .name="Master Playback Switch"}   
  }, 
  { .tag=PCM_Playback_Volume, .cb={.callback=unicens_pcm_vol_cb, .handle=&pcm_volume}, .info="Sets PCM playback volume",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=6, .minval=0, .maxval=100, .step=1, .value=100, .name="PCM Playback Volume"}   
  },
  { .tag=EndHalCrlTag}              /* marker for end of the array */
} ;

/* HAL sound card mapping info */
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "Microchip MOST:1",   /*  WARNING: name MUST match with 'aplay -l' */
    .info  = "HAL for MICROCHIP MOST sound card controlled by UNICENS binding",
    .ctls  = alsaHalMap,
    .volumeCB = NULL,               /* use default volume normalization function */
};

STATIC int sndServiceInit () {
    int err;
    AFB_DEBUG ("Initializing HAL-MOST-UNICENS-BINDING");
    
    err = halServiceInit (afbBindingV2.api, &alsaHalSndCard);
    return err;
}

/* API prefix should be unique for each snd card */
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "hal-most-unicens",
    .init    = sndServiceInit,
    .verbs   = halServiceApi,
    .onevent = halServiceEvent,
};
