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

/* declare ALSA mixer controls */
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .tag=Master_Playback_Volume, .ctl={.name="Master Playback Volume" } },
  { .tag=Master_OnOff_Switch   , .ctl={.name="Master Playback Switch" } },
  { .tag=PCM_Playback_Volume   , .ctl={.name="PCM Playback Volume"    } },
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

// API prefix should be unique for each snd card
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "hal-most-unicens",
    .init    = sndServiceInit,
    .verbs   = halServiceApi,
    .onevent = halServiceEvent,
};
