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
#include <systemd/sd-event.h>

typedef enum {
    ACTION_SET,
    ACTION_GET
} ActionSetGetT;

// VolRamp Handle Store current status for a given VolRam CB set

typedef struct {
    halRampEnumT mode;
    halCtlsTagT slave;
    int delay; // delay between volset in us
    int stepDown; // linear %
    int stepUp; // linear %
    int current; // current volume for slave ctl
    int target; // target volume
    sd_event_source *evtsrc; // event loop timer source
} halVolRampT;

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
    int value;
    int step;
    const char **enums;
    alsaHalDBscaleT *dbscale;
} alsaHalCtlMapT;


// avoid compiler warning [Jose does not like typedef :) ]
typedef struct afb_service alsaHalServiceT;

typedef struct {
    void (*callback)(halCtlsTagT tag, alsaHalCtlMapT *control, void* handle, json_object *valuesJ);
    void* handle;
} alsaHalCbMapT;

typedef struct {
    halCtlsTagT tag;
    const char *label;
    alsaHalCtlMapT ctl;
    alsaHalCbMapT cb;
    char* info;
} alsaHalMapT;

typedef struct {
    const char *name;
    const char *info;
    alsaHalMapT *ctls;
    const char *devid;
    json_object* (*volumeCB)(ActionSetGetT action, const alsaHalCtlMapT *halCtls, json_object *valuesJ);
} alsaHalSndCardT;

// hal-interface.c
extern afb_verb_v2 halServiceApi[];
PUBLIC void halServiceEvent(const char *evtname, json_object *object);
PUBLIC int halServiceInit(const char *apiPrefix, alsaHalSndCardT *alsaHalSndCard);
PUBLIC json_object *halGetCtlByTag(halRampEnumT tag);
PUBLIC int halSetCtlByTag(halRampEnumT tag, int value);


// hal-volramp.c
PUBLIC void volumeRamp(halCtlsTagT halTag, alsaHalCtlMapT *control, void* handle, json_object *valJ);

// hal-volume.c
PUBLIC json_object *volumeNormalise(ActionSetGetT action, const alsaHalCtlMapT *halCtls, json_object *valuesJ);


#endif /* SHAREHALLIB_H */

