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
#include "AlsaHalIface.h"  // Include Share Interface to Alsa Sound Card HAL

/*****************************************************************************
 * alsaCtlsMap link hight level sound control with low level Alsa numid ctls. 
 * 
 * To find out which control your sound card uses
 *  aplay -l
 *  amixer -D hw:xx controls
 *  amixer -D hw:xx contents
 *  amixer -D "hw:3" cget numid=xx
 *****************************************************************************/
STATIC alsaHalCtlMapT  alsaHalCtlsMap[]= { 
  { .control=Master_Playback_Volume, .numid=16, .group=OUTVOL, .values=1, .minval=0, .maxval= 87 , .step=0, .acl=RW, .info= "Master Playback Volume" },
  { .control=PCM_Playback_Volume   , .numid=27, .group=PCMVOL, .values=2, .minval=0, .maxval= 255, .step=0, .acl=RW, .info= "PCM Playback Volume" },
  { .control=PCM_Playback_Switch   , .numid=17, .group=SWITCH, .values=1, .minval=0, .maxval= 1  , .step=0, .acl=RW, .info= "Master Playback Switch" },
  { .control=Capture_Volume        , .numid=12, .group=INVOL , .values=2, .minval=0, .maxval= 31 , .step=0, .acl=RW, .info= "Capture Volume" },
  { .numid=0  } /* marker for end of the array */
} ;

/***********************************************************************************
 * AlsaHalSndT provides 
 *  - cardname used to map a given card to its HAL 
 *  - ctls previously defined AlsaHalMapT control maps
 *  - info free text
 * 
 *    WARNING: name should fit with 'aplay -l' as it used to map from devid to HAL
 *    you may also retreive shortname when AudioBinder is running from a browser
 *    http://localhost:1234/api/alsacore/getcardid?devid=hw:xxx
 *   
 ***********************************************************************************/
PUBLIC alsaHalSndCardT alsaHalSndCard  = {
    .name = "HDA Intel PCH",
    .info = "Hardware Abstraction Layer for IntelHDA sound card",
    .ctls = alsaHalCtlsMap,
};

/***********************************************************************************
 * AlsaHalSndT provides 
 *  - cardname used to map a given card to its HAL 
 *  - ctls previously defined AlsaHalMapT control maps
 *  - info free text
 * 
 *    WARNING: name should fit with 'aplay -l' as it used to map from devid to HAL
 *    you may also retreive shortname when AudioBinder is running from a browser
 *    http://localhost:1234/api/alsacore/getcardid?devid=hw:xxx
 *   
 ***********************************************************************************/
PUBLIC struct afb_binding alsaHalBinding = {
  /* description conforms to VERSION 1 */
  .type= AFB_BINDING_VERSION_1,
  .v1= {
    .prefix= "intel-hda",
    .info  = "Hardware Abstraction Layer for IntelHDA sound card",
  }
};


