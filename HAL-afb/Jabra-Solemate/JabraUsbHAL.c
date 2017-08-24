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
 *  amixer -D hw:v1340 controls # get supported controls
 *  amixer -Dhw:v1340 cget name=Power-Switch
 *  amixer -Dhw:v1340 cset name=Power-Switch true
 *
 */
#define _GNU_SOURCE
#include "hal-interface.h"
#include "audio-common.h"

// Define few private tag for not standard functions
#define PCM_Volume_Multimedia 1000
#define PCM_Volume_Navigation 1001
#define PCM_Volume_Emergency      1002

// Default Values for MasterVolume Ramping
STATIC halVolRampT volRampMaster= {
    .mode    = RAMP_VOL_NORMAL,
    .slave   = Master_Playback_Volume,
    .delay   = 100*1000, // ramping delay in us
    .stepDown=1,
    .stepUp  =1,
};

STATIC halVolRampT volRampMultimedia= {
    .slave   = PCM_Volume_Multimedia,
    .delay   = 100*1000, // ramping delay in us
    .stepDown= 2,
    .stepUp  = 1,
};

STATIC halVolRampT volRampNavigation= {
    .slave   = PCM_Volume_Navigation,
    .delay   = 100*1000, // ramping delay in us
    .stepDown= 4,
    .stepUp  = 2,
};
// Default Values for MasterVolume Ramping
STATIC halVolRampT volRampEmergency= {
    .slave   = PCM_Volume_Emergency,
    .delay   = 50*1000, // ramping delay in us
    .stepDown= 6,
    .stepUp  = 3,
};

// Map HAL hight sndctl with Alsa numid and optionally with a custom callback for non Alsa supported functionalities.
STATIC alsaHalMapT  alsaHalMap[]= {
  { .tag=Master_Playback_Volume, .  ctl={.name="PCM Playback Volume", .value=12 } },
  { .tag=PCM_Playback_Volume     , .ctl={.name="PCM Playback Volume", .value=12 } },
  { .tag=PCM_Playback_Switch     , .ctl={.name="PCM Playback Switch" } },
  { .tag=Capture_Volume          , .ctl={.name="Mic Capture Volume"  } },

  // Sound card does not have hardware volume ramping
  { .tag=Master_Playback_Ramp   , .cb={.callback=volumeRamp, .handle=&volRampMaster}, .info="ramp volume linearly according to current ramp setting",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .name="Rampup Master"}
  },

  // Implement Rampup Volume for Virtual Channels
  { .tag=Navigation_Playback_Volume, .cb={.callback=volumeRamp, .handle=&volRampNavigation}, .info="RampUp Navigation Volume",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER,.name="Playback Navigation Ramp", .value=80 }
  },
  { .tag=Emergency_Playback_Volume, .cb={.callback=volumeRamp, .handle=&volRampEmergency}, .info="Rampup Emergency Volume",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER,.name="Playback Emergency Ramp", .value=80 }
  },

  // Sound Card does not support hardware channel volume mixer (note softvol default range 0-256)
  { .tag=Multimedia_Playback_Volume, .cb={.callback=volumeRamp, .handle=&volRampMultimedia}, .info="Ramp-up Multimedia Volume",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER, .name="Playback Multimedia Ramp", .value=80 }
  },
  { .tag=PCM_Volume_Multimedia, .info="Playback Multimedia Softvol",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=2, .maxval=255, .value=200, .name="Playback Multimedia"}
  },
  { .tag=PCM_Volume_Navigation, .info="Playback Navigation Softvol",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=2, .maxval=255,.value=200, .name="Playback Navigation"}
  },
  { .tag=PCM_Volume_Emergency, .info="Playback Emergency Softvol",
    .ctl={.numid=CTL_AUTO, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=2, .maxval=255, .value=200, .name="Playback Emergency"}
  },


  { .tag=EndHalCrlTag}  /* marker for end of the array */
} ;

// HAL sound card mapping info
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "Jabra SOLEMATE v1.34.0", //  WARNING: name MUST match with 'aplay -l'
    .info  = "Hardware Abstraction Layer for Jabra Solamte USB speakers",
    .ctls  = alsaHalMap,
    .volumeCB = NULL, // use default volume normalisation function
};


STATIC int sndServiceInit () {
    int err;
    err = halServiceInit (afbBindingV2.api, &alsaHalSndCard);
    return err;
}

// API prefix should be unique for each snd card
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "jabra-usb",
    .init    = sndServiceInit,
    .verbs   = halServiceApi,
    .onevent = halServiceEvent,
};
