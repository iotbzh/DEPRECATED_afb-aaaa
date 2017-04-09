/*
 * AlsaLibMapping -- provide low level interface with AUDIO lib (extracted from alsa-json-gateway code)
 * Copyright (C) 2015,2016,2017, Fulup Ar Foll fulup@iot.bzh
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

#ifndef SHAREHALLIB_H
#define SHAREHALLIB_H

#include <stdio.h>
#include "audio-interface.h"

typedef struct {
     halCtlsEnumT control;
     int numid;
     halGroupEnumT group;
     int values;
     int minval;
     int maxval;
     int step;
     halAclEnumT acl;
} alsaHalCtlMapT;

// avoid compiler warning [Jose does not like typedef :) ]
typedef struct afb_service alsaHalServiceT;

typedef struct {
   struct json_object* (*callback)(alsaHalServiceT service, int control, int value, alsaHalCtlMapT *map, void* handle);
   void* handle;        
} alsaHalCbMapT;

typedef struct {
    alsaHalCtlMapT alsa;
    alsaHalCbMapT cb;
    char* info;
} alsaHalMapT;

typedef struct  {
    const char  *prefix;
    const char  *name;
    const char  *info;
    alsaHalMapT *ctls;
    int (*initCB) (const struct afb_binding_interface *itf, struct afb_service service);
    
} alsaHalSndCardT;

PUBLIC alsaHalSndCardT alsaHalSndCard;
PUBLIC char* SharedHalLibVersion;

#endif /* SHAREHALLIB_H */

