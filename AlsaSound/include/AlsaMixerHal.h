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
} groupEnum;

typedef enum {
    READ,
    WRITE,
    RW,
} aclEnum;

typedef enum {
   Master_Playback_Volume,
   PCM_Playback_Volume,
   PCM_Playback_Switch,
   Capture_Volume,
} actionEnum;

typedef const struct {
    actionEnum action;
    int numid;
    groupEnum group;
    int values;
    int minval;
    int maxval;
    int step;
    char* info;
    aclEnum acl;
    
} AlsaHalMapT;

typedef struct  {
    const char  *halname;
    const char  *longname;
    const char  *info;
    AlsaHalMapT *ctls;
    
} AlsaHalSndT;

#endif /* ALSAMIXERMAP_H */

