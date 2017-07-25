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

typedef enum {
    ACTION_SET,
    ACTION_GET
} ActionSetGetT;

typedef struct {
    int min;
    int max;
    int step;
    int mute;
} alsaHalDBscaleT;

typedef struct {
     char* name;
     int numid;
     snd_ctl_elem_type_t type;
     int count;    
     int minval;
     int maxval;
     int step;
     alsaHalDBscaleT  *dbscale;
} alsaHalCtlMapT;

// avoid compiler warning [Jose does not like typedef :) ]
typedef struct afb_service alsaHalServiceT;

typedef struct {
   struct json_object* (*callback)(alsaHalCtlMapT *control, void* handle,  struct json_object *valuesJ);
   void* handle;        
} alsaHalCbMapT;

typedef struct {
    halCtlsEnumT tag;
    const char *label;
    alsaHalCtlMapT ctl;
    alsaHalCbMapT cb;
    char* info;
} alsaHalMapT;


typedef const struct  {
    char  *name;
    const char  *info;
    alsaHalMapT *ctls;    
} alsaHalSndCardT;

extern afb_verb_v2 halServiceApi[];
PUBLIC void halServiceEvent(const char *evtname, struct json_object *object);
PUBLIC int  halServiceInit (const char *apiPrefix, alsaHalSndCardT *alsaHalSndCard);

// hal-volmap.c
PUBLIC struct json_object *SetGetNormaliseVolumes(ActionSetGetT action, const alsaHalCtlMapT *halCtls,  struct json_object *valuesJ);


#endif /* SHAREHALLIB_H */

