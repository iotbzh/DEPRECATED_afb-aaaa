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
#include "hal-interface.h"
#include "audio-interface.h" 

// Force a hard dependency to ShareHallLib
PUBLIC char* SharedHalLibVersion;

// Init is call after all binding are loaded
STATIC int IntelHalInit (const struct afb_binding_interface *itf, struct afb_service service) {
    DEBUG (itf, "IntelHalBinding Initialised");
    
    return 0; // 0=OK 
}

/******************************************************************************************
 * alsaCtlsMap link hight level sound control with low level Alsa numid ctls. 
 * 
 * To find out which control your sound card uses
 *  aplay -l
 *  amixer -D hw:xx controls
 *  amixer -D hw:xx contents
 *  amixer -D "hw:3" cget numid=xx
 * 
 * When automatic mapping to Alsa numid is not enough a custom callback might be used
 *  .cb={.handle=xxxx, .callback=(json_object)MyCtlFunction(struct afb_service service, int controle, int value, const struct alsaHalCtlMapS *map)};
 ********************************************************************************************/
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .alsa={.control=Master_Playback_Volume,.numid=16,.group=OUTVOL,.values=1,.minval=0,.maxval= 87 ,.step=0,.acl=RW}, .info= "Master Playback Volume" },
  { .alsa={.control=PCM_Playback_Volume   ,.numid=27,.group=PCMVOL,.values=2,.minval=0,.maxval= 255,.step=0,.acl=RW}, .info= "PCM Playback Volume" },
  { .alsa={.control=PCM_Playback_Switch   ,.numid=17,.group=SWITCH,.values=1,.minval=0,.maxval= 1  ,.step=0,.acl=RW}, .info= "Master Playback Switch" },
  { .alsa={.control=Capture_Volume        ,.numid=12,.group=INVOL ,.values=2,.minval=0,.maxval= 31 ,.step=0,.acl=RW}, .info= "Capture Volume" },
  { .alsa={.numid=0}, .cb={.callback=NULL, .handle=NULL}} /* marker for end of the array */
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
    .name  = "HDA Intel PCH",
    .info  = "Hardware Abstraction Layer for IntelHDA sound card",
    .ctls  = alsaHalMap,
    .prefix="intel-hda",
    .initCB=IntelHalInit, // if NULL no initcallback
};
