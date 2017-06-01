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

#include "HighLevelBinding.h"

static struct afb_service srvitf;


// This callback is fired when afb_service_call for api/alsacore/subctl returns
STATIC void audioLogicSetVolCB(void *handle, int iserror, struct json_object *result) {
    struct afb_req request = afb_req_unstore(handle);

    if (!cbCheckResponse(request, iserror, result)) goto OnExit;

OnExit:
    return;
}

PUBLIC void audioLogicSetVol(struct afb_req request) {
    struct json_object *queryurl;
    int volume=0; // FULUP TBD !!!!!!!!!!!!

    // keep request for callback to respond
    struct afb_req *handle = afb_req_store(request);

    // get client context
    AudioLogicCtxT *ctx = afb_req_context_get(request);

    const char *vol = afb_req_value(request, "vol");
    if (vol == NULL) {
        afb_req_fail_f(request, "argument-missing", "vol=+-%%[0,100] missing");
        goto OnExit;
    }

    switch (vol[0]) {
        case '+':
            break;
        case '-':
            break;
        case '%':
            break;

        default:
            afb_req_fail_f(request, "value-invalid", "volume should be (+-%%[0-100]xxx) vol=%s", vol);
            goto OnExit;
    }

    if (!ctx->halapi) {
        afb_req_fail_f(request, "context-invalid", "No valid halapi in client context");
        goto OnExit;
    }

    // ********** Caluler le volume en % de manière intelligente    
    queryurl = json_object_new_object();
    json_object_object_add(ctx->queryurl, "pcm", json_object_new_int(Master_Playback_Volume));
    json_object_object_add(ctx->queryurl, "value", json_object_new_int(volume));

    // subcontract HAL API to process volume
    afb_service_call(srvitf, ctx->halapi, "volume", queryurl, audioLogicSetVolCB, handle);

    // final success/failure messages handle from callback
OnExit:
    return;
}

// This callback is fired when afb_service_call for api/alsacore/subctl returns

STATIC void alsaSubcribeCB(void *handle, int iserror, struct json_object *result) {
    struct afb_req request = afb_req_unstore(handle);

    if (!cbCheckResponse(request, iserror, result)) goto OnExit;

OnExit:
    return;
}

// Create and subscribe to alsacore ctl events
PUBLIC void audioLogicMonitor(struct afb_req request) {


    // get client context
    AudioLogicCtxT *ctx = afb_req_context_get(request);
    if (!ctx) {
        afb_req_fail_f(request, "ctx-notfound", "No Client Context HAL/getcontrol devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }

    // push request to low level binding
    NOTICE(afbIface, "audioLogicMonitor ctx->devid=%s [ctx->queryurl=%s]", ctx->devid, json_object_to_json_string(ctx->queryurl));

    if (ctx->queryurl) {
        json_object_get(ctx->queryurl); // Make sure usage count does not fall to zero
        struct afb_req *handle = afb_req_store(request);
        afb_service_call(srvitf, "alsacore", "subscribe", ctx->queryurl, alsaSubcribeCB, handle);
    }
    else afb_req_fail_f(request, "context-invalid", "No valid queryurl in client context");

    // success/failure messages return from callback  
OnExit:
    return;
}

// Subscribe to AudioBinding events

PUBLIC void audioLogicSubscribe(struct afb_req request) {

    return;
}


// Call when all bindings are loaded and ready to accept request
PUBLIC void audioLogicGetVol(struct afb_req request) {

    // Should call here Hardware Alsa Abstraction Layer for corresponding Sound Card
    afb_req_success(request, NULL, NULL);
    return;

}

// This callback is fired when afb_service_call for api/alsacore/subctl returns

STATIC void audioLogicOpenCB2(void *handle, int iserror, struct json_object *result) {
    struct json_object *response;

    // Make sure we got a response from API
    struct afb_req request = afb_req_unstore(handle);
    if (!cbCheckResponse(request, iserror, result)) goto OnExit;

        // get client context
    AudioLogicCtxT *ctx = afb_req_context_get(request);
    if (!ctx) {
        afb_req_fail_f(request, "ctx-notfound", "No Client Context HAL/getcontrol devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }
    
    // Get response from object
    json_object_object_get_ex(result, "response", &response);
    if (!response) {
        afb_req_fail_f(request, "response-notfound", "No Controls return from HAL/getcontrol devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }

    // extract sounds controls information from received Object 
    struct json_object *ctls;
    json_object_object_get_ex(response, "ctls", &ctls);
    if (!ctls) {
        afb_req_fail_f(request, "ctls-notfound", "No Controls return from HAL/getcontrol devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }

    // make sure return controls have a valid type
    if (json_object_get_type(ctls) != json_type_array) {
        afb_req_fail_f(request, "ctls-notarray", "Invalid Controls return from HAL/getcontrol devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }

    // loop on array and store values into client context
    for (int idx = 0; idx < json_object_array_length(ctls); idx++) {
        struct json_object *ctl;
        halCtlsEnumT control;
        int value;

        ctl = json_object_array_get_idx(ctls, idx);
        if (json_object_array_length(ctl) != 2) {
            afb_req_fail_f(request, "ctl-invalid", "Invalid Control return from HAL/getcontrol devid=[%s] name=[%s] ctl=%s"
                    , ctx->devid, ctx->shortname, json_object_get_string(ctl));
            goto OnExit;
        }

        // As HAL and Business logic use the same AlsaMixerHal.h direct conversion is not an issue
        control = (halCtlsEnumT) json_object_get_int(json_object_array_get_idx(ctl, 0));
        value = json_object_get_int(json_object_array_get_idx(ctl, 1));

        switch (control) {
            case Master_Playback_Volume:
                ctx->volumes.masterPlaybackVolume = value;
                break;

            case PCM_Playback_Volume:
                ctx->volumes.pcmPlaybackVolume = value;
                break;

            case PCM_Playback_Switch:
                ctx->volumes.pcmPlaybackSwitch = value;
                break;

            case Capture_Volume:
                ctx->volumes.captureVolume = value;
                break;

            default:
                NOTICE(afbIface, "audioLogicOpenCB2 unknown HAL control=[%s]", json_object_get_string(ctl));
        }
    }

OnExit:
    afb_req_context_set(request, ctx, free);
    return;
}

// This callback is fired when afb_service_call for api/alsacore/subctl returns
STATIC void audioLogicOpenCB1(void *handle, int iserror, struct json_object *result) {
    struct json_object *response, *subobj;

    // Make sure we got a valid API response
    struct afb_req request = afb_req_unstore(handle);
    if (!cbCheckResponse(request, iserror, result)) goto OnExit;

    // Get response from object
    json_object_object_get_ex(result, "response", &response);
    if (!response) {
        afb_req_fail_f(request, "response-notfound", "No Controls return from HAL/getcontrol");
        goto OnExit;
    }

    // attach client context to session
    AudioLogicCtxT *ctx = malloc(sizeof (AudioLogicCtxT));

    // extract information from Json Alsa Object 
    json_object_object_get_ex(response, "cardid", &subobj);
    if (subobj) ctx->cardid = json_object_get_int(subobj);

    // store devid as an object for further alsa request
    json_object_object_get_ex(response, "devid", &subobj);
    if (subobj) ctx->devid = strdup(json_object_get_string(subobj));

    json_object_object_get_ex(response, "halapi", &subobj);
    if (subobj) ctx->halapi = strdup(json_object_get_string(subobj));

    json_object_object_get_ex(response, "shortname", &subobj);
    if (subobj)ctx->shortname = strdup(json_object_get_string(subobj));

    json_object_object_get_ex(response, "longname", &subobj);
    if (subobj)ctx->longname = strdup(json_object_get_string(subobj));

    // save queryurl with devid only for further ALSA request
    ctx->queryurl = json_object_new_object();
    json_object_object_add(ctx->queryurl, "devid", json_object_new_string(ctx->devid));

    afb_req_context_set(request, ctx, free);

    // sound card was find let's store keycontrols into client session
    if (!ctx->halapi) {
        afb_req_fail_f(request, "hal-notfound", "No HAL found devid=[%s] name=[%s]", ctx->devid, ctx->shortname);
        goto OnExit;
    }

    struct json_object *queryurl = json_object_new_object();
    struct json_object *ctls = json_object_new_array();

    // add sound controls we want to keep track of into client session context
    json_object_array_add(ctls, json_object_new_int((int) Master_Playback_Volume));
    json_object_array_add(ctls, json_object_new_int((int) PCM_Playback_Volume));
    json_object_array_add(ctls, json_object_new_int((int) PCM_Playback_Switch));
    json_object_array_add(ctls, json_object_new_int((int) Capture_Volume));

    // send request to soundcard HAL binding
    json_object_object_add(queryurl, "ctls", ctls);
    handle = afb_req_store(request);  // FULUP ???? Needed for 2nd Callback ????
    afb_service_call(srvitf, ctx->halapi, "getControl", queryurl, audioLogicOpenCB2, handle);

    afb_req_success(request, response, NULL);

OnExit:
    // release original calling request
    afb_req_unref(request);
    return;
}

// Delegate to lowerlevel the mapping of soundcard name with soundcard ID
PUBLIC void audioLogicOpen(struct afb_req request) {

    // Delegate query to lower level
    struct afb_req *handle = afb_req_store(request);
    afb_service_call(srvitf, "alsacore", "getCardId", json_object_get(afb_req_json(request)), audioLogicOpenCB1, handle);
}

// Free client context create from audioLogicOpenCB
PUBLIC void audioLogicClose(struct afb_req request) {

    // retrieve current client context to print debug info
    AudioLogicCtxT *ctx = (AudioLogicCtxT*) afb_req_context_get(request);
    DEBUG(afbIface, "audioLogicClose cardid=%d devid=%s shortname=%s longname=%s", ctx->cardid, ctx->devid, ctx->shortname, ctx->longname);
}


// this function is call after all binder are loaded and initialised
PUBLIC int audioLogicInit(struct afb_service service) {
    srvitf = service;
    return 0;
}
