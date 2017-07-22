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
 * reference: 
 *   amixer contents; amixer controls;
 *   http://www.tldp.org/HOWTO/Alsa-sound-6.html 
 */
#define _GNU_SOURCE  // needed for vasprintf
#include <string.h>
#include "hal-interface.h"

static alsaHalMapT *halCtls;
static const char *halDevid;



STATIC const char *halCtlsLabels[] = {
   [StartHalCrlTag] = "NOT_USED",
   
   [Master_Playback_Volume] = "Master_Playback_Volume",
   [Master_OnOff_Switch] = "Master_OnOff_Switch",
   [Master_Playback_Ramp] = "Master_Playback_Ramp",
   [PCM_Playback_Volume] = "PCM_Playback_Volume",
   [PCM_Playback_Switch] = "PCM_Playback_Switch",
   [Capture_Volume] = "Capture_Volume",

   [EndHalCrlTag] = "NOT_USED"
  
};

// Force specific HAL to depend on ShareHalLib
PUBLIC char* SharedHalLibVersion="1.0";
 
// Subscribe to AudioBinding events
STATIC void halSubscribe(afb_req request) {
    const char *devidJ = afb_req_value(request, "devid");
    if (devidJ == NULL) {
        afb_req_fail_f(request, "devidJ-missing", "devidJ=hw:xxx missing");
    }
}



// Map HAL ctlName to ctlLabel
STATIC int halCtlStringToIndex (const char* label) {

    for (int idx = 0;   halCtls[idx].ctl.numid; idx++) {
       if (!strcmp (halCtls[idx].label, label)) return idx;
    }
    
    // not found
    return -1;
}

STATIC int halCtlTagToIndex (halCtlsEnumT tag) {

    for (int idx = 0;  halCtls[idx].ctl.numid; idx++) {
       if (halCtls[idx].tag == tag) return idx;
    }
    
    // not found
    return -1;
}


// Return ALL HAL snd controls
PUBLIC void halListCtls(afb_req request) {
    struct json_object *ctlsHalJ = json_object_new_array();
    
    for (int idx = 0; halCtls[idx].ctl.numid; idx++) {
        struct json_object *ctlHalJ = json_object_new_object();
        
        json_object_object_add(ctlHalJ, "label", json_object_new_string(halCtls[idx].label));
        json_object_object_add(ctlHalJ, "tag"  , json_object_new_int(halCtls[idx].tag));
        json_object_object_add(ctlHalJ, "count", json_object_new_int(halCtls[idx].ctl.count));
        
        json_object_array_add (ctlsHalJ, ctlHalJ);
    }
    
    afb_req_success (request, ctlsHalJ, NULL);
}


STATIC int halGetCtlIndex (afb_req request, struct json_object*ctlInJ) {
    struct json_object *tmpJ;
    int tag, index;

    // check 1st short command mode [tag1, tag2, ...]
    tag = json_object_get_type (ctlInJ);
    
    if (!tag) {
        json_object_object_get_ex (ctlInJ, "tag" , &tmpJ);
        tag = json_object_get_int(tmpJ);
    }
    
    if (tag) {
        index = halCtlTagToIndex((halCtlsEnumT) tag);
    } else {
        // tag was not provided let's try label
        const char *label;
        
        json_object_object_get_ex (ctlInJ, "label" , &tmpJ);
        label = json_object_get_string(tmpJ);
        index = halCtlStringToIndex(label);
    }

    if (index < 0) {
        afb_req_fail_f(request, "ctl-invalid", "No Label/Tag given ctl='%s'", json_object_get_string(ctlInJ));
        goto OnErrorExit;                
    }
    
    // return corresponding lowlevel numid to querylist
    return index;
    
    OnErrorExit:
        return -1;
} 

// HAL normalise volume values to 0-100%
STATIC struct json_object *UnNormaliseValue(const alsaHalCtlMapT *halCtls,  struct json_object *valuesJ) {
    int length;

    // assert response as the right length    
    length = json_object_array_length(valuesJ);
    if (length != halCtls->count) {
        AFB_WARNING ("UnNormaliseValue invalid ctl='%s' values count=%d len=%d", halCtls->name, halCtls->count, length);
        return NULL;
    }
    
    json_object *normalisedJ= json_object_new_array();
    for (int idx=0; idx < halCtls->count; idx++) {
        int value;
        
        // use last value in array when number of values does not match with actual ctl count
        if (idx < length) {
            json_object *valueJ = json_object_array_get_idx (valuesJ, idx);
            value = json_object_get_int(valueJ);
            
            // cleanup and normalise value
            if (value > halCtls->maxval) value= halCtls->maxval;
            if (value < halCtls->minval) value= halCtls->minval;

            // If Integer move from 0-100% to effective value
            if (halCtls->type == SND_CTL_ELEM_TYPE_INTEGER) {
                value = (value * (halCtls->maxval-halCtls->minval))/100;
            } 
        }
        
        // add unnormalised value into response
        json_object_array_add(normalisedJ, json_object_new_int(value));  
    }
    
    return (normalisedJ);
}


// Translate high level control to low level and call lower layer
PUBLIC void halSetCtls(afb_req request) {
    int err, index;
    struct json_object *ctlsInJ, *ctlsOutJ, *queryJ, *valuesJ, *responseJ;

    // get query from request
    ctlsInJ = afb_req_json(request);
    ctlsOutJ = json_object_new_array();
    
    switch (json_object_get_type(ctlsInJ)) {
        case json_type_object: {
            // control is in literal form {tag=xxx, label=xxx, value=xxxx}
            index = halGetCtlIndex (request, ctlsInJ);
            if (index <=0) goto OnErrorExit;
            
            err= json_object_object_get_ex (ctlsInJ, "value" , &valuesJ);
            if (err) {
                afb_req_fail_f(request, "ctl-invalid", "No val=[val1, ...] ctl='%s'", json_object_get_string(ctlsInJ));
                goto OnErrorExit;                                
            }
            
            json_object_array_add (ctlsOutJ, json_object_new_int(halCtls[index].ctl.numid));
            json_object_array_add (ctlsOutJ, UnNormaliseValue (&halCtls[index].ctl, valuesJ));                
            break;
        }
        
        case json_type_array: {
            
            for (int idx= 0; idx < json_object_array_length (ctlsInJ); idx++) {
                struct json_object *ctlInJ = json_object_array_get_idx (ctlsInJ, idx);
                index= halGetCtlIndex (request, ctlInJ);
                if (index<=0) goto OnErrorExit;

                err= json_object_object_get_ex (ctlInJ, "value" , &valuesJ);
                if (err) {
                    afb_req_fail_f(request, "ctl-invalid", "No val=[val1, ...] ctl='%s'", json_object_get_string(ctlsInJ));
                    goto OnErrorExit;                                
                }
                // let's create alsa low level set control request
                struct json_object *ctlOutJ = json_object_new_array();
                json_object_array_add (ctlOutJ, json_object_new_int(halCtls[index].ctl.numid));
                json_object_array_add (ctlOutJ, UnNormaliseValue (&halCtls[index].ctl, valuesJ));                
                
                json_object_array_add (ctlsOutJ, ctlOutJ);
            }
            break;
        }
                
        default:
            afb_req_fail_f(request, "ctl-invalid", "Not a valid JSON ctl='%s'", json_object_get_string(ctlsInJ));
            goto OnErrorExit;                
    }

    // Call now level CTL
    queryJ = json_object_new_object();
    json_object_object_add(queryJ, "devid", json_object_new_string (halDevid));
    json_object_object_add(queryJ, "numid", ctlsOutJ);

    err= afb_service_call_sync("alsacore", "setctls", queryJ, &responseJ);
    if (err) {
        afb_req_fail_f(request, "subcall:alsacore/setctl", "%s", json_object_get_string(responseJ));
        goto OnErrorExit;        
    }
    
    // Let ignore info data if any and keep on response
    json_object_object_get_ex (responseJ, "response", &responseJ);
    
    // map back low level response to HAL ctl with normalised values
    //struct json_object *halResponse =  CtlSetPrepareResponse(request, responseJ);
    //if (!halResponse) goto OnErrorExit;
   
    afb_req_success (request, NULL, NULL);
    return;

OnErrorExit:
    return;
};

// Remap low level controls into HAL hight level ones
STATIC json_object *CtlGetPrepareResponse(afb_req request, struct json_object *ctlsJ) {
    struct json_object *halResponseJ;

    // make sure return controls have a valid type
    if (json_object_get_type(ctlsJ) != json_type_array) {
        afb_req_fail_f(request, "ctls-notarray", "Invalid Controls return from alsa/getcontrol ctlsJ=%s", json_object_get_string(ctlsJ));
        goto OnErrorExit;
    }
    
    // responseJ is a JSON array
    halResponseJ = json_object_new_array();

    // loop on array and store values into client context
    for (int idx = 0; idx < json_object_array_length(ctlsJ); idx++) {
        struct json_object *sndCtlJ, *valJ, *numidJ;
        int numid;

        sndCtlJ = json_object_array_get_idx(ctlsJ, idx);
        if (!json_object_object_get_ex(sndCtlJ, "numid", &numidJ) ||  !json_object_object_get_ex(sndCtlJ, "val", &valJ)) {
            afb_req_fail_f(request, "ctl-invalid", "Invalid Control return from alsa/getcontrol ctl=%s", json_object_get_string(sndCtlJ));
            goto OnErrorExit;
        }
        
        // HAL and Business logic use the same AlsaMixerHal.h direct conversion
        numid= (halCtlsEnumT) json_object_get_int(numidJ);

        for (int idx = 0; halCtls[idx].ctl.numid; idx++) {
            if (halCtls[idx].ctl.numid == numid) {
                
                // translate low level numid to HAL one and normalise values
                struct json_object *halCtlJ = json_object_new_object();
                json_object_object_add(halCtlJ, "label", json_object_new_string(halCtls[idx].label)); // idx+1 == HAL/NUMID
                json_object_object_add(halCtlJ, "tag"  , json_object_new_int(halCtls[idx].tag)); // idx+1 == HAL/NUMID
                json_object_object_add(halCtlJ, "val"  , GetNormaliseVolume(&halCtls[idx].ctl, valJ));
                json_object_array_add(halResponseJ, halCtlJ);
                break;
            }           
        }
        if ( halCtls[idx].ctl.numid == 0) {
            afb_req_fail_f(request, "ctl-invalid", "Invalid Control numid=%d from alsa/getcontrol ctlJ=%s", numid, json_object_get_string(sndCtlJ));
            goto OnErrorExit;
        }   
    }
    return halResponseJ;
    
    OnErrorExit:
        return NULL;
}

// Translate high level control to low level and call lower layer
PUBLIC void halGetCtls(afb_req request) {
    int err, index;
    struct json_object *ctlsInJ, *ctlsOutJ, *queryJ, *responseJ;

    // get query from request
    ctlsInJ = afb_req_json(request);
    ctlsOutJ = json_object_new_array();
    
    switch (json_object_get_type(ctlsInJ)) {
        case json_type_object: {

            index = halGetCtlIndex (request, ctlsInJ);
            if (index <=0) goto OnErrorExit;
            json_object_array_add (ctlsOutJ, json_object_new_int(halCtls[index].ctl.numid));
            break;
        }
        
        case json_type_array: {
            
            for (int idx= 0; idx < json_object_array_length (ctlsInJ); idx++) {
                struct json_object *ctlInJ = json_object_array_get_idx (ctlsInJ, idx);
                index= halGetCtlIndex (request, ctlInJ);
                if (index<=0) goto OnErrorExit;
                json_object_array_add (ctlsOutJ, json_object_new_int(halCtls[index].ctl.numid));
            }
            break;
        }
                
        default:
            afb_req_fail_f(request, "ctl-invalid", "Not a valid JSON ctl='%s'", json_object_get_string(ctlsInJ));
            goto OnErrorExit;                
    }

    // Call now level CTL
    queryJ = json_object_new_object();
    json_object_object_add(queryJ, "devid", json_object_new_string (halDevid));
    json_object_object_add(queryJ, "numid", ctlsOutJ);

    err= afb_service_call_sync("alsacore", "getctl", queryJ, &responseJ);
    if (err) {
        afb_req_fail_f(request, "subcall:alsacore/getctl", "%s", json_object_get_string(responseJ));
        goto OnErrorExit;        
    }
    
    // Let ignore info data if any and keep on response
    json_object_object_get_ex (responseJ, "response", &responseJ);
    
    // map back low level response to HAL ctl with normalised values
    struct json_object *halResponse =  CtlGetPrepareResponse(request, responseJ);
    if (!halResponse) goto OnErrorExit;
   
    afb_req_success (request, halResponse, NULL);
    return;

OnErrorExit:
    return;
};

// This receive all event this binding subscribe to 
PUBLIC void halServiceEvent(const char *evtname, struct json_object *eventJ) {
    int numid;
    struct json_object *numidJ, *valuesJ, *valJ;
    
    AFB_NOTICE("halServiceEvent evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));
    
    json_object_object_get_ex (eventJ, "values" , &valuesJ);
    if (!valuesJ) {
        AFB_ERROR("halServiceEvent novalues: evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));
        return;
    }
         
    json_object_object_get_ex (valuesJ, "numid" , &numidJ);
    numid = json_object_get_int (numidJ);
    
    // search it corresponding numid in halCtls attach a callback
    if (numid) {
        for (int idx= 0; halCtls[idx].ctl.numid; idx++) {
            if (halCtls[idx].ctl.numid == numid && halCtls[idx].cb.callback != NULL) {
                json_object_object_get_ex (valuesJ, "val" , &valJ);
                halCtls[idx].cb.callback (&halCtls[idx].ctl, halCtls[idx].cb.handle, valJ);
            }
        }
    }    
}

STATIC int UpdateOneSndCtl (alsaHalCtlMapT *ctl, struct json_object *sndCtlJ) {
    struct json_object *tmpJ, *ctlJ;

    json_object_object_get_ex (sndCtlJ, "name" , &tmpJ);
    ctl->name  = (char*)json_object_get_string(tmpJ);
    
    json_object_object_get_ex (sndCtlJ, "numid" , &tmpJ);
    ctl->numid  = json_object_get_int(tmpJ);
    
    // make sure we face a valid Alsa Low level ctl
    if (!json_object_object_get_ex (sndCtlJ, "ctl" , &ctlJ)) goto OnErrorExit;
    
    json_object_object_get_ex (ctlJ, "min" , &tmpJ);
    ctl->minval  = json_object_get_int(tmpJ);
    
    json_object_object_get_ex (ctlJ, "max" , &tmpJ);
    ctl->maxval  = json_object_get_int(tmpJ);
    
    json_object_object_get_ex (ctlJ, "step" , &tmpJ);
    ctl->step  = json_object_get_int(tmpJ);
    
    json_object_object_get_ex (ctlJ, "count" , &tmpJ);
    ctl->count  = json_object_get_int(tmpJ);
    
    json_object_object_get_ex (ctlJ, "type" , &tmpJ);
    ctl->type  = (snd_ctl_elem_type_t)json_object_get_int(tmpJ);
    
    // process dbscale TLV if any
    if (json_object_object_get_ex (sndCtlJ, "tlv" , &tmpJ)) {
        struct json_object *dbscaleJ;
        
        if (!json_object_object_get_ex (tmpJ, "dbscale" , &dbscaleJ)) {
            AFB_WARNING("TLV found but not DBscale attached ctl name=%s numid=%d", ctl->name, ctl->numid);
        } else {
            ctl->dbscale = malloc (sizeof (alsaHalDBscaleT));
            
            json_object_object_get_ex (dbscaleJ, "min" , &tmpJ);
            ctl->dbscale->min = (snd_ctl_elem_type_t)json_object_get_int(tmpJ);
            
            json_object_object_get_ex (dbscaleJ, "max" , &tmpJ);
            ctl->dbscale->max = (snd_ctl_elem_type_t)json_object_get_int(tmpJ);
            
            json_object_object_get_ex (dbscaleJ, "step" , &tmpJ);
            ctl->dbscale->step = (snd_ctl_elem_type_t)json_object_get_int(tmpJ);
            
            json_object_object_get_ex (dbscaleJ, "mute" , &tmpJ);
            ctl->dbscale->mute = (snd_ctl_elem_type_t)json_object_get_int(tmpJ);
        }
    }
    
    return 0;
    
    OnErrorExit:
            return -1;
}

// this is call when after all bindings are loaded
PUBLIC int halServiceInit (const char *apiPrefix, alsaHalSndCardT *alsaHalSndCard) {
    int err;
    struct json_object *queryurl, *responseJ, *devidJ, *ctlsJ, *tmpJ;
    halCtls = alsaHalSndCard->ctls; // Get sndcard specific HAL control mapping
    
    err= afb_daemon_require_api("alsacore", 1);
    if (err) {
        AFB_ERROR ("AlsaCore missing cannot use AlsaHAL");
        goto OnErrorExit;        
    }

    // register HAL with Alsa Low Level Binder
    queryurl = json_object_new_object();
    json_object_object_add(queryurl, "prefix", json_object_new_string(apiPrefix));
    json_object_object_add(queryurl, "sndname", json_object_new_string(alsaHalSndCard->name));
    
    err= afb_service_call_sync ("alsacore", "halregister", queryurl, &responseJ);
    json_object_put(queryurl);
    if (err) {
        AFB_NOTICE ("Fail to register HAL to ALSA lowlevel binding Response='%s'", json_object_get_string(responseJ));
        goto OnErrorExit;
    }
    
    // extract sound devidJ from HAL registration
    if (!json_object_object_get_ex(responseJ, "response", &tmpJ) || !json_object_object_get_ex(tmpJ, "devid", &devidJ)) {
        AFB_ERROR ("Ooops: Internal error no devidJ return from HAL registration Response='%s'", json_object_get_string(responseJ));
        goto OnErrorExit;
    }
    
    // save devid for future use
    halDevid = strdup (json_object_get_string(devidJ));
            
    // for each Non Alsa Control callback create a custom control
    ctlsJ= json_object_new_array();
    for (int idx= 0; (halCtls[idx].ctl.name||halCtls[idx].ctl.numid); idx++) {
        struct json_object *ctlJ;
        
        // map HAL ctlTad with halCrlLabel to simplify set/get ctl operations using Labels
        halCtls[idx].label = halCtlsLabels[halCtls[idx].tag];
        
        if (halCtls[idx].cb.callback != NULL) {
            ctlJ = json_object_new_object();
            if (halCtls[idx].ctl.name)   json_object_object_add(ctlJ, "name" , json_object_new_string(halCtls[idx].ctl.name));
            if (halCtls[idx].ctl.minval) json_object_object_add(ctlJ, "min"  , json_object_new_int(halCtls[idx].ctl.minval));
            if (halCtls[idx].ctl.maxval) json_object_object_add(ctlJ, "max"  ,  json_object_new_int(halCtls[idx].ctl.maxval));
            if (halCtls[idx].ctl.step)   json_object_object_add(ctlJ, "step" , json_object_new_int(halCtls[idx].ctl.step));
            if (halCtls[idx].ctl.type)   json_object_object_add(ctlJ, "type" , json_object_new_int(halCtls[idx].ctl.type));
            json_object_array_add(ctlsJ, ctlJ);             
        } else  {
            ctlJ = json_object_new_object();
            if (halCtls[idx].ctl.numid)  json_object_object_add(ctlJ, "numid" , json_object_new_int(halCtls[idx].ctl.numid));
            if (halCtls[idx].ctl.name)   json_object_object_add(ctlJ, "name"  , json_object_new_string(halCtls[idx].ctl.name));
            json_object_array_add(ctlsJ, ctlJ);             
        }           
    }
    
    // Build new queryJ to add HAL custom control if any
    if (json_object_array_length (ctlsJ) > 0) {
        queryurl = json_object_new_object();
        json_object_get(devidJ); // make sure devidJ does not get free by 1st call.
        json_object_object_add(queryurl, "devid",devidJ);
        json_object_object_add(queryurl, "ctl",ctlsJ);
        json_object_object_add(queryurl, "mode",json_object_new_int(QUERY_COMPACT));
        err= afb_service_call_sync ("alsacore", "addcustomctl", queryurl, &responseJ);
        if (err) {
            AFB_ERROR ("Fail creating HAL Custom ALSA ctls Response='%s'", json_object_get_string(responseJ));
            goto OnErrorExit;
        }    
    }

    // Make sure response is valid 
    json_object_object_get_ex (responseJ, "response" , &ctlsJ);
    if (json_object_get_type(ctlsJ) != json_type_array) {
        AFB_ERROR ("Response Invalid JSON array ctls Response='%s'", json_object_get_string(responseJ));
        goto OnErrorExit;        
    }
    
    // update HAL data from JSON response
    for (int idx= 0; idx < json_object_array_length (ctlsJ); idx++) {
        json_object *ctlJ = json_object_array_get_idx (ctlsJ, idx);
        err= UpdateOneSndCtl(&halCtls[idx].ctl, ctlJ) ;
        if (err) {
           AFB_ERROR ("Fail found MAP Alsa Low level=%s",  json_object_get_string(ctlJ)); 
           goto OnErrorExit;
        }
    }

    
    // finally register for alsa lowlevel event
    queryurl = json_object_new_object();
    json_object_object_add(queryurl, "devid",devidJ);
    err= afb_service_call_sync ("alsacore", "subscribe", queryurl, &responseJ);
    if (err) {
        AFB_ERROR ("Fail subscribing to ALSA lowlevel events");
        goto OnErrorExit;
    }
    
    return (0);   
    
  OnErrorExit:
    return (1);
};

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
PUBLIC afb_verb_v2 halServiceApi[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping",     .callback = pingtest},
    { .verb = "ctl-list", .callback = halListCtls},
    { .verb = "ctl-get",  .callback = halGetCtls},
    { .verb = "evt-sub",  .callback = halSubscribe},
    { .verb = NULL} /* marker for end of the array */
};
