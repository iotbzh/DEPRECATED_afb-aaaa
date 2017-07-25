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
 */
#define _GNU_SOURCE  // needed for vasprintf

#include <json-c/json.h>
#include <semaphore.h>
#include <string.h>

#include "audio-interface.h"


PUBLIC void pingtest(struct afb_req request) {
    json_object *query = afb_req_json(request);
    afb_req_success(request, query, NULL);
}
