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
 * reference: 
 *   amixer contents; amixer controls;
 *   http://www.tldp.org/HOWTO/Alsa-sound-6.html 
 */



#ifndef ALSAMIXERMAP_H
#define ALSAMIXERMAP_H

// Most controls are MIXER but some vendor specific are possible
typedef enum {
    OUTVOL,
    PCMVOL,
    INVOL,
    SWITCH,
    ROUTE,
    CARD,
} halGroupEnumT;

typedef enum {
    READ,
    WRITE,
    RW,
} halAclEnumT;

typedef enum {
   StartHalCrlTag=0,

   // HighLevel Audio Control List
   Master_Playback_Volume,
   PCM_Playback_Volume,
   PCM_Playback_Switch,
   Capture_Volume,

   EndHalCrlTag // used to compute number of ctls
} halControlEnumT;


#endif /* ALSAMIXERMAP_H */

