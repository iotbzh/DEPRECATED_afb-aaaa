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

#ifndef AUDIO_INTERF_H
#define AUDIO_INTERF_H

#define AFB_BINDING_VERSION 2

#include <json-c/json.h>
#include <afb/afb-binding.h>

// Waiting for official macro from José
#define AFB_GET_VERBOSITY afb_get_verbosity_v2()

#ifndef PUBLIC
  #define PUBLIC
#endif
#define STATIC static

typedef enum {
  QUERY_QUIET   =0,  
  QUERY_COMPACT =1,  
  QUERY_VERBOSE =2,  
  QUERY_FULL    =3,  
} halQueryMode;

// Most controls are MIXER but some vendor specific are possible
typedef enum {
    OUTVOL,
    PCMVOL,
    INVOL,
    SWITCH,
    ROUTE,
    CARD,
    ENUM,
} halGroupEnumT;

typedef enum {
    READ,
    WRITE,
    RW,
} halAclEnumT;

typedef enum {
   StartHalCrlTag=0,

   // HighLevel Audio Control List,
   Master_Playback_Volume =1,
   Master_Playback_Ramp   =2,
   PCM_Playback_Volume    =3,
   PCM_Playback_Switch    =4,
   Capture_Volume         =5,
   Master_OnOff_Switch    =6,

   EndHalCrlTag // used to compute number of ctls
} halCtlsEnumT;


PUBLIC void pingtest(struct afb_req request);

#endif /* AUDIO_INTERF_H */

