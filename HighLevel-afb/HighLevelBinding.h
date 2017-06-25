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

#ifndef AUDIOLOGIC_H
#define AUDIOLOGIC_H

#include "audio-interface.h"

// import from AlsaAfbBinding
extern const struct afb_binding_interface *afbIface;


// This structure hold private data for a given client of binding
typedef struct {
 
  int  cardid;
  const char  *devid;
  const char  *shortname;
  const char  *longname;
  const char  *halapi;
  struct { // volume in % [0-100]
      int masterPlaybackVolume;
      int pcmPlaybackVolume;
      int pcmPlaybackSwitch;
      int captureVolume;
  } volumes;
  json_object *queryurl;
} AudioLogicCtxT;

// import from AlsaAfbMapping
PUBLIC void audioLogicSetVol (struct afb_req request);
PUBLIC void audioLogicGetVol(struct afb_req request);
PUBLIC void audioLogicMonitor(struct afb_req request);
PUBLIC void audioLogicOpen(struct afb_req request);
PUBLIC void audioLogicClose(struct afb_req request);
PUBLIC void audioLogicSubscribe(struct afb_req request);
PUBLIC int  audioLogicInit (struct afb_service service);

#endif /* AUDIOLOGIC_H */

