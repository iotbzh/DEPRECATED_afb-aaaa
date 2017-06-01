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

typedef struct {
    int index;
    int numid;
} shareHallMap_T;


PUBLIC int cbCheckResponse(struct afb_req request, int iserror, struct json_object *result) {
    struct json_object *response, *status, *info;

    if (iserror) { // on error proxy information we got from lower layer
        if (result) {
            if (json_object_object_get_ex(result, "request", &response)) {
                json_object_object_get_ex(response, "info", &info);
                json_object_object_get_ex(response, "status", &status);
                afb_req_fail(request, json_object_get_string(status), json_object_get_string(info));
                goto OnErrorExit;
            }
        } else {
            afb_req_fail(request, "cbCheckFail", "No Result inside API response");
        }
        goto OnErrorExit;
    }
    return (0);

OnErrorExit:
    return (-1);
}


PUBLIC void pingtest(struct afb_req request) {
    json_object *query = afb_req_json(request);
    afb_req_success(request, query, NULL);
}
