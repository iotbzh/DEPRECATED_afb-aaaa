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
#include <alsa/asoundlib.h>

#include "audio-interface.h"

typedef struct {
     halCtlsEnumT control;
     char* name;
     int numid;
     halGroupEnumT group;
     int values;
     int minval;
     int maxval;
     int step;
     snd_ctl_elem_type_t type;
     halAclEnumT acl;
} alsaHalCtlMapT;

// avoid compiler warning [Jose does not like typedef :) ]
typedef struct afb_service alsaHalServiceT;

// static value for HAL sound card API prefix
extern const char sndCardApiPrefix[];

typedef struct {
   struct json_object* (*callback)(alsaHalCtlMapT *control, void* handle);
   void* handle;        
} alsaHalCbMapT;

typedef struct {
    alsaHalCtlMapT alsa;
    alsaHalCbMapT cb;
    char* info;
} alsaHalMapT;


typedef const struct  {
    const char  *name;
    const char  *info;
    alsaHalMapT *ctls;
    int (*initCB) (void);
    
} alsaHalSndCardT;

PUBLIC alsaHalSndCardT alsaHalSndCard;

#endif /* SHAREHALLIB_H */

