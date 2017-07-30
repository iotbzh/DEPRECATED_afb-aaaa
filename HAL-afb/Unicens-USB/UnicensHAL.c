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
 * 
 * To find out which control your sound card uses
 *  aplay -l  # Check sndcard name name in between []
 *  amixer -D hw:xx controls # get supported controls
 *  amixer -D "hw:3" cget numid=xx  # get control settings
 * 
 */
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h" 



// Map HAL hight sndctl with Alsa numid and optionally with a custom callback for non Alsa supported functionalities. 
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .tag=Master_Playback_Volume, .ctl={.numid=04 } },
  { .tag=PCM_Playback_Volume   , .ctl={.numid=06 } },
  { .tag=PCM_Playback_Switch   , .ctl={.numid=05 } },
  { .tag=Capture_Volume        , .ctl={.numid=12 } },

  { .tag=EndHalCrlTag}  /* marker for end of the array */
} ;

// HAL sound card mapping info
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "HDA Intel PCH", //  WARNING: name MUST match with 'aplay -l'
    .info  = "Hardware Abstraction Layer for IntelHDA sound card",
    .ctls  = alsaHalMap,
};


STATIC int sndServiceInit () {
    int err;
    AFB_DEBUG ("IntelHalBinding Init");

    err = halServiceInit (afbBindingV2.api, &alsaHalSndCard);
    return err;
}

// API prefix should be unique for each snd card
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "unicens-usb",
    .init    = sndServiceInit,
    .verbs   = halServiceApi,
    .onevent = halServiceEvent,
};
