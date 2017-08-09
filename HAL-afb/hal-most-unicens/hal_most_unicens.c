/*
 * Copyright (C) 2017, Microchip Technology Inc. and its subsidiaries.
 * Author Tobias Jahnke
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
#define _GNU_SOURCE 
#include "hal-interface.h"
#include "audio-interface.h"
#include "wrap-json.h"
#include "string.h"

static int master_volume;
static json_bool master_switch;
static int pcm_volume[6];

void unicens_master_vol_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[i!]", &master_volume) == 0) {
        AFB_NOTICE("master_volume: %s, value=%d", j_str, master_volume);
    }
    else {
        AFB_NOTICE("master_volume: INVALID STRING %s", j_str);
    } 
}

void unicens_master_switch_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[b!]", &master_switch) == 0) {
        AFB_NOTICE("master_switch: %s, value=%d", j_str, master_switch);
    }
    else {
        AFB_NOTICE("master_switch: INVALID STRING %s", j_str);
    }    
}

void unicens_pcm_vol_cb(halCtlsEnumT tag, alsaHalCtlMapT *control, void* handle,  json_object *j_obj) {

    const char *j_str = json_object_to_json_string(j_obj);
    
    if (wrap_json_unpack(j_obj, "[iiiiii!]", &pcm_volume[0], &pcm_volume[1], &pcm_volume[2], &pcm_volume[3],
                                             &pcm_volume[4], &pcm_volume[5]) == 0) {
        AFB_NOTICE("pcm_vol: %s", j_str);
    }
    else {
        AFB_NOTICE("pcm_vol: INVALID STRING %s", j_str);
    }
}

/* declare ALSA mixer controls */
STATIC alsaHalMapT  alsaHalMap[]= { 
  { .tag=Master_Playback_Volume, .cb={.callback=unicens_master_vol_cb, .handle=&master_volume}, .info="Sets master playback volume",
    .ctl={.numid=1, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=1, .minval=0, .maxval=100, .step=1, .value=50, .name="Master Playback Volume"}   
  },
  /*{ .tag=Master_OnOff_Switch, .cb={.callback=unicens_master_switch_cb, .handle=&master_switch}, .info="Sets master playback switch",
    .ctl={.numid=2, .type=SND_CTL_ELEM_TYPE_BOOLEAN, .count=1, .minval=0, .maxval=1, .step=1, .value=1, .name="Master Playback Switch"}   
  },*/
  { .tag=PCM_Playback_Volume, .cb={.callback=unicens_pcm_vol_cb, .handle=&pcm_volume}, .info="Sets PCM playback volume",
    .ctl={.numid=3, .type=SND_CTL_ELEM_TYPE_INTEGER, .count=6, .minval=0, .maxval=100, .step=1, .value=100, .name="PCM Playback Volume"}   
  },
  { .tag=EndHalCrlTag}              /* marker for end of the array */
} ;

/* HAL sound card mapping info */
STATIC alsaHalSndCardT alsaHalSndCard  = {
    .name  = "Microchip MOST:1",   /*  WARNING: name MUST match with 'aplay -l' */
    .info  = "HAL for MICROCHIP MOST sound card controlled by UNICENS binding",
    .ctls  = alsaHalMap,
    .volumeCB = NULL,               /* use default volume normalization function */
};

STATIC int unicens_start_binding() {
    
    json_object *j_response, *j_query = NULL;
    int err;
    
    /* Build an empty JSON object */
    err = wrap_json_pack(&j_query, "{}");
    if (err) {
        AFB_ERROR("Failed to create subscribe json object");
        goto OnErrorExit; 
    }
    
    err = afb_service_call_sync("UNICENS", "subscribe", j_query, &j_response);
    if (err) {
        AFB_ERROR("Fail subscribing to UNICENS events");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("Subscribed to UNICENS events, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    json_object_put(j_query);
#if 0
    /* Build JSON object to retrieve UNICENS configuration */
    err = wrap_json_pack(&j_query, "{}");
    if (err) {
        AFB_ERROR("Failed to create listconfig json object");
        goto OnErrorExit; 
    }
    
    err = afb_service_call_sync("UNICENS", "listconfig", j_query, &j_response);
    if (err) {
        AFB_ERROR("Failed to call listconfig");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("UNICENS listconfig result, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    json_object_put(j_query);
#endif
    
    /* Build JSON object to initialize UNICENS */
    err = wrap_json_pack(&j_query, "{s:s}", "filename", "/home/agluser/DEVELOPMENT/AGL/BINDING/unicens2-binding/data/config_multichannel_audio_kit.xml");
    if (err) {
        AFB_ERROR("Failed to create initialize json object");
        goto OnErrorExit; 
    }
    err = afb_service_call_sync("UNICENS", "initialise", j_query, &j_response);
    if (err) {
        AFB_ERROR("Failed to initialize UNICENS");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("Initialized UNICENS, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    json_object_put(j_query);
    
    
    j_query = NULL;
  OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return NULL;
}

STATIC int unicens_service_init() {
    int err;
    AFB_NOTICE("Initializing HAL-MOST-UNICENS-BINDING");
    
    err = halServiceInit(afbBindingV2.api, &alsaHalSndCard);
    if (err) {
        AFB_ERROR("Cannot initialize hal-most-unicens binding.");
        goto OnErrorExit;        
    }    
    
    err= afb_daemon_require_api("UNICENS", 1);
    if (err) {
        AFB_ERROR("UNICENS is missing or not initialized");
        goto OnErrorExit;        
    }
    
    unicens_start_binding();
    
    
OnErrorExit:
    AFB_NOTICE("Initializing HAL-MOST-UNICENS-BINDING done..");
    return err;
}

// This receive all event this binding subscribe to 
PUBLIC void unicens_event_cb(const char *evtname, json_object *j_event) {
    
    if (strncmp(evtname, "alsacore/", 9) == 0) {
        halServiceEvent(evtname, j_event);
        return;
    }
    
    if (strncmp(evtname, "UNICENS/", 8) == 0) {
        AFB_NOTICE("unicens_event_cb: evtname=%s [msg=%s]", evtname, json_object_get_string(j_event));
        return;
    }
    
    AFB_NOTICE("unicens_event_cb: UNHANDLED EVENT, evtname=%s [msg=%s]", evtname, json_object_get_string(j_event));
}

/* API prefix should be unique for each snd card */
PUBLIC const struct afb_binding_v2 afbBindingV2 = {
    .api     = "hal-most-unicens",
    .init    = unicens_service_init,
    .verbs   = halServiceApi,
    .onevent = unicens_event_cb,
};
