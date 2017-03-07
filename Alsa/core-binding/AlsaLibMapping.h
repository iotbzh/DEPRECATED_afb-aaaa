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


// few coding convention
typedef int BOOL;
#ifndef PUBLIC
  #define PUBLIC
#endif
#ifndef FALSE
  #define FALSE 0
#endif
#ifndef TRUE
  #define TRUE 1
#endif
#define STATIC    static
 
#ifndef ALSALIBMAPPING_H
#define ALSALIBMAPPING_H

#include <json-c/json.h>
#include <afb/afb-binding.h>


// import from AlsaAfbBinding
extern const struct afb_binding_interface *binderIface;
extern struct sd_event *afb_common_get_event_loop();

// import from AlsaAfbMapping
PUBLIC void alsaGetInfo (struct afb_req request);
PUBLIC void alsaGetCtl(struct afb_req request);
PUBLIC void alsaSubCtl (struct afb_req request);


#endif /* ALSALIBMAPPING_H */

