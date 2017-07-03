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
 *  amixer -D "hw:4" cget numid=xx  # get control settings
 * 
 */
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h" 

STATIC struct json_object* MasterOnOff (alsaHalCtlMapT *control, void* handle) {
    static int powerStatus=0;
    
    if (! powerStatus) {
        powerStatus = 1;
        AFB_DEBUG ("Power Set to On");
    } else {
        powerStatus  = 0;
        AFB_DEBUG ("Power Set to Off");        
    }
    
    return NULL;
}

// Map HAL hight sndctl with Alsa numid and optionally with a custom callback for non Alsa supported functionalities. 
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .alsa={.control=Master_Playback_Volume,.numid=04, .name="Matrix 03 Mix A Playback Volume"   , .values=1,.minval=0,.maxval= 87 ,.step=0}, .info= "Master Playback Volume" },
  { .alsa={.control=PCM_Playback_Volume   ,.numid=06, .name="play-vol"     , .values=2,.minval=0,.maxval= 255,.step=0}, .info= "PCM Playback Volume" },
  { .alsa={.control=PCM_Playback_Switch   ,.numid=05, .name="play-switch"  , .values=1,.minval=0,.maxval= 1  ,.step=0}, .info= "Master Playback Switch" },
  { .alsa={.control=Capture_Volume        ,.numid=12, .name="capt-vol"     , .values=2,.minval=0,.maxval= 31 ,.step=0}, .info= "Capture Volume" },
  { .alsa={.control=Master_OnOff_Switch, .name="Power-Switch"}, .cb={.callback=MasterOnOff, .handle=NULL}, .info= "OnOff Global Switch"},
  { .alsa={.numid=0}, .cb={.callback=NULL, .handle=NULL}} /* marker for end of the array */
} ;

// HAL sound card mapping info
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "Scarlett 18i8 USB", //  WARNING: name MUST match with 'aplay -l'
    .info  = "Hardware Abstraction Layer for Scarlett Focusrite USB professional music sound card",
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
    .api     = "scarlett-usb",
    .init    = sndServiceInit,
    .verbs   = halServiceApi,
    .onevent = halServiceEvent,
};
