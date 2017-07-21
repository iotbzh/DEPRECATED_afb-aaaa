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
 *  amixer -D hw:xx controls # get supported controls
 *  amixer -D "hw:3" cget numid=xx  # get control settings
 * 
 */
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h" 

STATIC struct json_object* MasterOnOff (alsaHalCtlMapT *control, void* handle, struct json_object *valJ) {
    struct json_object *reponseJ;
    
    AFB_INFO ("Power Set value=%s", json_object_get_string(valJ));
    
    reponseJ=json_object_new_object();
    json_object_object_add (reponseJ, "Callback", json_object_new_string("Hello From HAL"));
    
    return reponseJ;
}
