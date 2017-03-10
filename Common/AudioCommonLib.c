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
#include <afb/afb-binding.h>
#include <afb/afb-service-itf.h>
#include <semaphore.h>
#include <string.h>

#include "AudioCommonLib.h"

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

// This function should be part of Generic AGL Framework
PUBLIC json_object* afb_service_call_sync(struct afb_service srvitf, struct afb_req request, char* api, char* verb, struct json_object* queryurl, void *handle) {
    json_object* response = NULL;
    int status = 0;
    sem_t semid;

    // Nested procedure are allow in GNU and allow us to keep caller stack valid

    void callback(void *handle, int iserror, struct json_object * result) {

        // Process Basic Error
        if (!cbCheckResponse(request, iserror, result)) {
            status = -1;
            goto OnExitCB;
        }

        // Get response from object
        json_object_object_get_ex(result, "response", &response);
        if (!response) {
            afb_req_fail_f(request, "response-notfound", "No Controls return from alsa/getcontrol result=[%s]", json_object_get_string(result));
            goto OnExitCB;
        }

OnExitCB:
        sem_post(&semid);
    }

    // Create an exclusive semaphore
    status = sem_init(&semid, 0, 0);
    if (status < 0) {
        afb_req_fail_f(request, "error:seminit", "Fail to allocate semaphore err=[%s]", strerror(status));
        goto OnExit;
    }

    // Call service and wait for call back to finish before moving any further
    afb_service_call(srvitf, "alsacore", "getctl", queryurl, callback, handle);
    sem_wait(&semid);

OnExit:
    return (response);
}

PUBLIC void pingtest(struct afb_req request) {
    json_object *query = afb_req_json(request);
    afb_req_success(request, query, NULL);
}
