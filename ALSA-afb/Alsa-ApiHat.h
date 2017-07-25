/*
 * AlsaLibMapping -- provide low level interface with ALSA lib (extracted from alsa-json-gateway code)
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

 
#ifndef ALSALIBMAPPING_H
#define ALSALIBMAPPING_H


#include <alsa/asoundlib.h>
#include <systemd/sd-event.h>
#include "audio-interface.h"

typedef enum {
    ACTION_SET,
    ACTION_GET
} ActionSetGetT;

// generic structure to pass parsed query values
typedef struct {
  const char *devid;
  json_object *numidsJ;
  halQueryMode mode;
  int count;
} queryValuesT;

// use to store crl numid user request
typedef struct {
    unsigned int numId;
    json_object *jToken;
    json_object *valuesJ;
    int used;
} ctlRequestT;

// import from AlsaAfbBinding
extern const struct afb_binding_interface *afbIface;
PUBLIC json_object *alsaCheckQuery (struct afb_req request, queryValuesT *queryValues);

// AlseCoreSetGet exports
PUBLIC int alsaGetSingleCtl (snd_ctl_t *ctlDev, snd_ctl_elem_id_t *elemId, ctlRequestT *ctlRequest, halQueryMode queryMode);
PUBLIC void alsaGetInfo (struct afb_req request);
PUBLIC void alsaGetCtls(struct afb_req request);
PUBLIC void alsaSetCtls(struct afb_req request);


// AlsaUseCase exports
PUBLIC void alsaUseCaseQuery(struct afb_req request); 
PUBLIC void alsaUseCaseSet(struct afb_req request); 
PUBLIC void alsaUseCaseGet(struct afb_req request); 
PUBLIC void alsaUseCaseClose(struct afb_req request); 
PUBLIC void alsaUseCaseReset(struct afb_req request); 
PUBLIC void alsaAddCustomCtls(struct afb_req request); 

// AlsaRegEvt
PUBLIC void alsaEvtSubcribe (struct afb_req request);
PUBLIC void alsaGetCardId (struct afb_req request);
PUBLIC void alsaRegisterHal (struct afb_req request);
PUBLIC void alsaActiveHal (struct afb_req request);

#endif /* ALSALIBMAPPING_H */

