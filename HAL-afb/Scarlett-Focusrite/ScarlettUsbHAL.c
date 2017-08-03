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
 *  amixer -D hw:USB controls # get supported controls
 *  amixer -Dhw:USB cget name=Power-Switch
 *  amixer -Dhw:USB cset name=Power-Switch true
 * 
 */
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h" 

// Default Values for MasterVolume Ramping
STATIC halVolRampT volRampMaster= {
    .mode    = RAMP_VOL_NORMAL,
    .slave   = Master_Playback_Volume,
    .delay   = 100*1000, // ramping delay in us
    .stepDown=1,
    .stepUp  =1,
};

// Map HAL hight sndctl with Alsa numid and optionally with a custom callback for non Alsa supported functionalities. 
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .tag=Master_Playback_Volume, .  ctl={.name="Master Playback Volume" } },
  { .tag=PCM_Playback_Volume     , .ctl={.name="Master 1 (Monitor) Playback Volume" } },
  { .tag=PCM_Playback_Switch     , .ctl={.numid=05 } },
  { .tag=Capture_Volume          , .ctl={.numid=12 } },

  { .tag=Vol_Ramp_Set_Mode       , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="select volramp speed",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_ENUMERATED, .count=1, .value=RAMP_VOL_NORMAL, .name="Hal-VolRamp-Mode", .enums=halVolRampModes}   
  },  
  { .tag=Vol_Ramp_Set_Slave      , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="set slave volume master numid",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .value=Master_Playback_Volume, .name="Hal-VolRamp-Slave", .enums=halVolRampModes}   
  },  
  { .tag=Vol_Ramp_Set_Delay    , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="set ramp delay [default 250ms]",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=1000, .step=100, .value=250, .name="Hal-VolRamp-Step-Down"}
  },
  { .tag=Vol_Ramp_Set_Down   , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="set linear step down ramp [default 10]",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .value=10, .name="Hal-VolRamp-Step-Down"}
  },
  { .tag=Vol_Ramp_Set_Up   , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="set linear step up ramp [default 10]",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .value=10, .name="Hal-VolRamp-Step-Up"}
  },
  { .tag=Master_Playback_Volume   , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="ramp volume linearly according to current ramp setting",
    .ctl={.numid=0, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .name="Hal-VolRamp"}
  },
  
  { .tag=EndHalCrlTag}  /* marker for end of the array */
} ;

// HAL sound card mapping info
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "Scarlett 18i8 USB", //  WARNING: name MUST match with 'aplay -l'
    .info  = "Hardware Abstraction Layer for Scarlett Focusrite USB professional music sound card",
    .ctls  = alsaHalMap,
    .volumeCB = NULL, // use default volume normalisation function
};


STATIC int sndServiceInit () {
    int err;
    AFB_DEBUG ("Scarlett Binding Init");
    
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
