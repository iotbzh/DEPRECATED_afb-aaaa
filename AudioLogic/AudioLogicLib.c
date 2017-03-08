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
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>

#include "AudioLogicLib.h"
static struct afb_service srvitf;

#define _GNU_SOURCE  // needed for vasprintf

// this function is call after all binder are loaded and initialised
PUBLIC int audioLogicInit (struct afb_service service) {
    srvitf = service;
    return 0;
}

// This callback is fired when afb_service_call for api/alsacore/subctl returns
STATIC void alsaSubcribeCB (void *handle, int iserror, struct json_object *result) {
	struct afb_req request = afb_req_unstore(handle);
        struct json_object *x, *resp = NULL;
	const char *info = NULL;

	if (result) {
	    INFO (afbIface, "result=[%s]\n", json_object_to_json_string (result));
            if (json_object_object_get_ex(result, "request", &x) && json_object_object_get_ex(x, "info", &x))
		info = json_object_get_string(x);
		if (!json_object_object_get_ex(result, "response", &resp)) resp = NULL;
	}
        
        // push message respond
	if (iserror) afb_req_fail_f(request,  "Fail", info);
	else         afb_req_success(request, resp, info);
        
        // free calling request
	afb_req_unref(request);
}

// Create and subscribe to alsacore ctl events
PUBLIC void audioLogicMonitor(struct afb_req request) {
    
    // save request in session as it might be used after return by callback
    struct afb_req *handle = afb_req_store(request);

    // push request to low level binding
    if (!handle) afb_req_fail(request, "error", "out of memory");
    else    afb_service_call(srvitf, "alsacore", "subctl", json_object_get(afb_req_json(request)), alsaSubcribeCB, handle);

    // success/failure messages return from callback    
}

// Call when all bindings are loaded and ready to accept request
PUBLIC void audioLogicGetVol(struct afb_req request) {
   
    afb_req_success (request, NULL, NULL);
    return;
    
}

PUBLIC void audioLogicSetVol(struct afb_req request) {
   

    afb_req_success (request, NULL, NULL);
    return;
    
}

