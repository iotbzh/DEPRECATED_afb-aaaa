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

#ifndef AUDIOCOMMON_H
#define AUDIOCOMMON_H

#include <json-c/json.h>
#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>

#ifndef PUBLIC
  #define PUBLIC
#endif
#define STATIC static

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

   // HighLevel Audio Control List
   Master_Playback_Volume,
   PCM_Playback_Volume,
   PCM_Playback_Switch,
   Capture_Volume,

   EndHalCrlTag // used to compute number of ctls
} halCtlsEnumT;

PUBLIC int cbCheckResponse(struct afb_req request, int iserror, struct json_object *result) ;
PUBLIC json_object* afb_service_call_sync(struct afb_service srvitf, struct afb_req request, char* api, char* verb, struct json_object* queryurl, void *handle);
PUBLIC void pingtest(struct afb_req request);

#endif /* AUDIOCOMMON_H */

