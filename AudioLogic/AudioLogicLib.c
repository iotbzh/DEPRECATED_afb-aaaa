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


PUBLIC void audioLogicSetVol(struct afb_req request) {
   
    const char *vol = afb_req_value(request, "vol");
    if (vol == NULL) {
        afb_req_fail_f (request, "argument-missing", "vol=+-%[0,100] missing");
        goto ExitOnError;
    }
    
    switch (vol[0]) {
        case '+':
            break;
        case '-':
            break;
        case '%':
            break;
        
        default:
           afb_req_fail_f (request, "value-invalid", "volume should be (+-%[0-100]xxx) vol=%s", vol);
           goto ExitOnError;  
    }
    
    // Should call here Hardware Alsa Abstraction Layer for corresponding Sound Card
    afb_req_success (request, NULL, NULL);
    return;
    
    ExitOnError:
        return;  
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
        
}

// Create and subscribe to alsacore ctl events
PUBLIC void audioLogicMonitor(struct afb_req request) {
    
    // keep request for callback to respond
    struct afb_req *handle = afb_req_store(request);
    
    // get client context
    AudioLogicCtxT *ctx = afb_req_context_get(request);
    
    // push request to low level binding
    NOTICE (afbIface, "audioLogicMonitor ctx->devid=%s [ctx->queryurl=%s]", ctx->devid, json_object_to_json_string(ctx->queryurl));
    
    if (ctx->queryurl) {
        json_object_get (ctx->queryurl); // Make sure usage count does not fall to zero
        afb_service_call(srvitf, "alsacore", "subscribe", ctx->queryurl, alsaSubcribeCB, handle);
    }
    
    else afb_req_fail_f(request,  "context-invalid", "No valid queryurl in client context");

    // success/failure messages return from callback    
}

// Subscribe to AudioBinding events
PUBLIC void audioLogicSubscribe (struct afb_req request) {
   
        return;
}


// Call when all bindings are loaded and ready to accept request
PUBLIC void audioLogicGetVol(struct afb_req request) {
   
    // Should call here Hardware Alsa Abstraction Layer for corresponding Sound Card
    afb_req_success (request, NULL, NULL);
    return;
    
}

// This callback is fired when afb_service_call for api/alsacore/subctl returns
STATIC void audioLogicOpenCB (void *handle, int iserror, struct json_object *result) {
    struct afb_req request = afb_req_unstore(handle);
    struct json_object *response;
    //INFO (afbIface, "result=[%s]\n", json_object_to_json_string (result));


    if (iserror) { // on error proxy information we got from lower layer
        if (result) {
            struct json_object *status, *info;
        
            if (json_object_object_get_ex(result,   "request", &response)) {
                json_object_object_get_ex(response, "info"  , &info);
                json_object_object_get_ex(response, "status", &status);
                afb_req_fail(request, json_object_get_string(status), json_object_get_string(info));
                goto OnExit;
            }
        } else {
            afb_req_fail(request,  "Fail", "Unknown Error" );
        }
        goto OnExit;
    }    
    
    // Get response from object
    json_object_object_get_ex(result, "response", &response);
    if (response) {
        struct json_object *subobj;
        
        // attach client context to session
        AudioLogicCtxT *ctx = malloc (sizeof(AudioLogicCtxT));
        
        // extract information from Json Alsa Object 
        json_object_object_get_ex(response, "cardid", &subobj);
        if (subobj) ctx->cardid= json_object_get_int(subobj);
        
        // store devid as an object for further alsa request
        json_object_object_get_ex(response, "devid", &subobj);
        if (subobj) ctx->devid= strdup(json_object_get_string(subobj));        
        
        json_object_object_get_ex(response, "shortname", &subobj);
        if (subobj)ctx->shortname=strdup(json_object_get_string(subobj));
        
        json_object_object_get_ex(response, "longname", &subobj);
        if (subobj)ctx->longname=strdup(json_object_get_string(subobj));
    
        // add AudioLogicCtxT to Client Session
        NOTICE (afbIface, "audioLogicOpen ctx->devid=[%s]", ctx->devid);
                
        // save queryurl with devid only for further ALSA request
        ctx->queryurl=json_object_new_object();
        json_object_object_add(ctx->queryurl, "devid",json_object_new_string(ctx->devid));

        afb_req_context_set(request, ctx, free);
    }

    // release original calling request
    afb_req_success(request, response, NULL);

    
  OnExit:
    afb_req_unref(request);
    return;
}

// Delegate to lowerlevel the mapping of soundcard name with soundcard ID
PUBLIC void audioLogicOpen(struct afb_req request) {

    // Delegate query to lower level
    struct afb_req *handle = afb_req_store(request);
    if (!handle) afb_req_fail(request, "error", "out of memory");
    else afb_service_call(srvitf, "alsacore", "getCardId", json_object_get(afb_req_json(request)), audioLogicOpenCB, handle);
}

// Free client context create from audioLogicOpenCB
PUBLIC void audioLogicClose (struct afb_req request) {
  
    // retrieve current client context to print debug info
    AudioLogicCtxT *ctx = (AudioLogicCtxT*) afb_req_context_get(request);
    DEBUG (afbIface, "audioLogicClose cardid=%d devid=%s shortname=%s longname=%s", ctx->cardid, ctx->devid, ctx->shortname, ctx->longname);
}


// this function is call after all binder are loaded and initialised
PUBLIC int audioLogicInit (struct afb_service service) {
    srvitf = service;
    
    return 0;
}
