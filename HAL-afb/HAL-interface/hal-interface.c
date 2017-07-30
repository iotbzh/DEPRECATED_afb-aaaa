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
#include <systemd/sd-event.h>

alsaHalSndCardT *halSndCard;

STATIC const char *halCtlsLabels[] = {
   
    [Master_Playback_Volume] = "Master_Playback_Volume",
    [Master_OnOff_Switch] = "Master_OnOff_Switch",
    [Master_Playback_Ramp]= "Master_Playback_Ramp",
    [PCM_Playback_Volume] = "PCM_Playback_Volume",
    [PCM_Playback_Switch] = "PCM_Playback_Switch",
    [Capture_Volume]      = "Capture_Volume",

    [Vol_Ramp]           = "Volume_Ramp",        
    [Vol_Ramp_Set_Mode]  = "Volume_Ramp_Mode",        
    [Vol_Ramp_Set_Delay] = "Volume_Ramp_Delay",        
    [Vol_Ramp_Set_Down]  = "Volume_Ramp_Down",        
    [Vol_Ramp_Set_Up]    = "Volume_Ramp_Up",        
       
   [EndHalCrlTag] = NULL
};

PUBLIC char *halVolRampModes[] = {
   
   [RAMP_VOL_NONE]      = "None",
   [RAMP_VOL_NORMAL]    = "Normal",
   [RAMP_VOL_SMOOTH]    = "Smooth",
   [RAMP_VOL_EMERGENCY] = "Emergency",
   
   [EndHalVolMod] = NULL,
   
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
    alsaHalMapT *halCtls= halSndCard->ctls;

    for (int idx = 0;  halCtls[idx].tag != EndHalCrlTag; idx++) {
       if (!strcmp (halCtls[idx].label, label)) return idx;
    }
    
    // not found
    return -1;
}

STATIC int halCtlTagToIndex (halCtlsEnumT tag) {
    alsaHalMapT *halCtls= halSndCard->ctls;

    for (int idx = 0;  halCtls[idx].tag != EndHalCrlTag; idx++) {
       if (halCtls[idx].tag == tag) return idx;
    }
    
    // not found
    return -1;
}


// Return ALL HAL snd controls
PUBLIC void halListCtls(afb_req request) {
    alsaHalMapT *halCtls= halSndCard->ctls;
    json_object *ctlsHalJ = json_object_new_array();
    
    for (int idx = 0; halCtls[idx].ctl.numid; idx++) {
        json_object *ctlHalJ = json_object_new_object();
        
        json_object_object_add(ctlHalJ, "label", json_object_new_string(halCtls[idx].label));
        json_object_object_add(ctlHalJ, "tag"  , json_object_new_int(halCtls[idx].tag));
        json_object_object_add(ctlHalJ, "count", json_object_new_int(halCtls[idx].ctl.count));
        
        json_object_array_add (ctlsHalJ, ctlHalJ);
    }
    
    afb_req_success (request, ctlsHalJ, NULL);
}

STATIC int halGetCtlIndex (afb_req request, json_object*ctlInJ) {
    json_object *tmpJ;
    int tag, index, done;

    // check 1st short command mode [tag1, tag2, ...]
    enum json_type jtype = json_object_get_type (ctlInJ);
    switch (jtype) {
        case json_type_array: 
            tmpJ = json_object_array_get_idx (ctlInJ, 0);
            tag = json_object_get_int(tmpJ);
            index = halCtlTagToIndex(tag);  
            break;
            
        case json_type_int: 
            tag = json_object_get_int(ctlInJ);
            index = halCtlTagToIndex(tag);  
            break;
        
        case json_type_object:
            done = json_object_object_get_ex (ctlInJ, "tag" , &tmpJ);
            if (done) {
                tag = json_object_get_int(tmpJ);
                index = halCtlTagToIndex(tag);  
            } else {
                const char *label;
                done = json_object_object_get_ex (ctlInJ, "label" , &tmpJ);
                if (!done) goto OnErrorExit;
                label = json_object_get_string(tmpJ);
                index = halCtlStringToIndex(label);
            }
            break;
        
        default: 
            goto OnErrorExit;
    }
    
    if (index < 0) goto OnErrorExit;
    
    // return corresponding lowlevel numid to querylist
    return index;
    
    OnErrorExit:
        afb_req_fail_f(request, "ctl-invalid", "No Label/Tag given ctl='%s'", json_object_get_string(ctlInJ));
        return -1;
} 


STATIC int  halCallAlsaSetCtls (json_object *ctlsOutJ) {
    json_object *responseJ, *queryJ;
    int err;
    
    // Call now level CTL
    queryJ = json_object_new_object();
    json_object_object_add(queryJ, "devid", json_object_new_string (halSndCard->devid));
    json_object_object_add(queryJ, "ctl", ctlsOutJ);
    
        err= afb_service_call_sync("alsacore", "setctl", queryJ, &responseJ);
        json_object_put (responseJ); // let's ignore response
    
    return err;
}


// retrieve a single HAL control from its tag.
PUBLIC int halSetCtlByTag (halRampEnumT tag, int value) {
    json_object *ctlJ = json_object_new_array();
    alsaHalMapT *halCtls= halSndCard->ctls;
    int err, index;
            
    index = halCtlTagToIndex(tag); 
    if (index < 0) goto OnErrorExit;
    
    json_object_array_add(ctlJ, json_object_new_int(halCtls[index].ctl.numid));
    json_object_array_add(ctlJ, volumeNormalise (ACTION_SET, &halCtls[index].ctl, json_object_new_int(value)));
    
    err = halCallAlsaSetCtls(ctlJ);
    
    return err;
    
 OnErrorExit:
        return -1;
}


// Translate high level control to low level and call lower layer
PUBLIC void halSetCtls(afb_req request) {
    alsaHalMapT *halCtls= halSndCard->ctls;
    int err, done, index;
    json_object *ctlsInJ, *ctlsOutJ, *valuesJ;

    // get query from request
    ctlsInJ = afb_req_json(request);
    
    switch (json_object_get_type(ctlsInJ)) {
        case json_type_object: {
            ctlsOutJ = json_object_new_object();
            
            // control is in literal form {tag=xxx, label=xxx, value=xxxx}
            index = halGetCtlIndex (request, ctlsInJ);
            if (index < 0) goto OnErrorExit;
            
            done= json_object_object_get_ex (ctlsInJ, "val" , &valuesJ);
            if (!done) {
                afb_req_fail_f(request, "ctl-invalid", "No val=[val1, ...] ctl='%s'", json_object_get_string(ctlsInJ));
                goto OnErrorExit;                                
            }
            
            json_object_object_add (ctlsOutJ, "id", json_object_new_int(halCtls[index].ctl.numid));
            json_object_object_add (ctlsOutJ,"val", volumeNormalise (ACTION_SET, &halCtls[index].ctl, valuesJ));                
            break;
        }
        
        case json_type_array: {
            ctlsOutJ = json_object_new_array();
            
            for (int idx= 0; idx < json_object_array_length (ctlsInJ); idx++) {
                json_object *ctlInJ = json_object_array_get_idx (ctlsInJ, idx);
                index= halGetCtlIndex (request, ctlInJ);
                if (index < 0) goto OnErrorExit;

                done= json_object_object_get_ex (ctlInJ, "val" , &valuesJ);
                if (!done) {
                    afb_req_fail_f(request, "ctl-invalid", "No val=[val1, ...] ctl='%s'", json_object_get_string(ctlsInJ));
                    goto OnErrorExit;                                
                }
                // let's create alsa low level set control request
                json_object *ctlOutJ = json_object_new_object();
                json_object_object_add (ctlOutJ, "id", json_object_new_int(halCtls[index].ctl.numid));
                json_object_object_add (ctlOutJ, "val", volumeNormalise (ACTION_SET, &halCtls[index].ctl, valuesJ));                
                
                json_object_array_add (ctlsOutJ, ctlOutJ);
            }
            break;
        }
                
        default:
            afb_req_fail_f(request, "ctl-invalid", "Not a valid JSON ctl='%s'", json_object_get_string(ctlsInJ));
            goto OnErrorExit;                
    }

    err = halCallAlsaSetCtls (ctlsOutJ);
    if (err) {
        afb_req_fail_f(request, "subcall:alsacore/setctl", "%s", json_object_get_string(ctlsOutJ));
        goto OnErrorExit;        
    }
    
    afb_req_success (request, NULL, NULL);
    return;

OnErrorExit:
    return;
};

// Remap low level controls into HAL hight level ones
STATIC json_object *HalGetPrepareResponse(afb_req request, json_object *ctlsJ) {
    alsaHalMapT *halCtls= halSndCard->ctls;
    json_object *halResponseJ;
    int length;

    switch (json_object_get_type(ctlsJ)) {
        case json_type_array:
            // responseJ is a JSON array
            halResponseJ = json_object_new_array();
            length = json_object_array_length(ctlsJ);
            break;
        case json_type_object:
            halResponseJ=NULL;
            length = 1;
            break;
        default:
            afb_req_fail_f(request, "ctls-notarray", "Invalid Controls return from alsa/getcontrol ctlsJ=%s", json_object_get_string(ctlsJ));
            goto OnErrorExit;
    }
    
    // loop on array and store values into client context
    for (int idx = 0; idx < length; idx++) {
        json_object *sndCtlJ, *valJ, *numidJ;
        int numid;

        // extract control from array if any
        if (halResponseJ) sndCtlJ = json_object_array_get_idx(ctlsJ, idx);
        else sndCtlJ=ctlsJ;
        
        if (!json_object_object_get_ex(sndCtlJ, "id", &numidJ) ||  !json_object_object_get_ex(sndCtlJ, "val", &valJ)) {
            afb_req_fail_f(request, "ctl-invalid", "Invalid Control return from alsa/getcontrol ctl=%s", json_object_get_string(sndCtlJ));
            goto OnErrorExit;
        }
        
        // HAL and Business logic use the same AlsaMixerHal.h direct conversion
        numid= (halCtlsEnumT) json_object_get_int(numidJ);

        for (int idx = 0; halCtls[idx].ctl.numid; idx++) {
            if (halCtls[idx].ctl.numid == numid) {               
                // translate low level numid to HAL one and normalise values
                json_object *halCtlJ = json_object_new_object();
                json_object_object_add(halCtlJ, "label", json_object_new_string(halCtls[idx].label)); // idx+1 == HAL/NUMID
                json_object_object_add(halCtlJ, "tag"  , json_object_new_int(halCtls[idx].tag)); // idx+1 == HAL/NUMID
                json_object_object_add(halCtlJ, "val"  , volumeNormalise(ACTION_GET, &halCtls[idx].ctl, valJ));
                
                if (halResponseJ) json_object_array_add(halResponseJ, halCtlJ);
                else halResponseJ=halCtlJ;
                
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



STATIC json_object *halCallAlsaGetCtls (json_object *ctlsOutJ) {
    json_object *responseJ, *queryJ;
    int err, done;
    
    // Call now level CTL
    queryJ = json_object_new_object();
    json_object_object_add(queryJ, "devid", json_object_new_string (halSndCard->devid));
    json_object_object_add(queryJ, "ctl", ctlsOutJ);

    err= afb_service_call_sync("alsacore", "getctl", queryJ, &responseJ);
    if (err) goto OnErrorExit;
    
    // Let ignore info data if any and keep on response
    done = json_object_object_get_ex (responseJ, "response", &responseJ);
    if (!done) goto OnErrorExit;
    
    return responseJ;
    
  OnErrorExit:
        return NULL;
}

// retrieve a single HAL control from its tag.
PUBLIC json_object *halGetCtlByTag (halRampEnumT tag) {
    json_object *responseJ, *valJ;
    alsaHalMapT *halCtls= halSndCard->ctls;
    int done, index;
            
    index = halCtlTagToIndex(tag); 
    if (index < 0) goto OnErrorExit;
    responseJ = halCallAlsaGetCtls(json_object_new_int(halCtls[index].ctl.numid));
    
    done = json_object_object_get_ex(responseJ, "val", &valJ);
    if (!done) goto OnErrorExit;
    
    return volumeNormalise(ACTION_GET, &halCtls[index].ctl, valJ);
    
 OnErrorExit:
        return NULL;
}


// Translate high level control to low level and call lower layer
PUBLIC void halGetCtls(afb_req request) {
    int index;
    alsaHalMapT *halCtls= halSndCard->ctls;
    json_object *ctlsInJ, *ctlsOutJ, *responseJ;

    // get query from request
    ctlsInJ = afb_req_json(request);
    ctlsOutJ = json_object_new_array();
    
    switch (json_object_get_type(ctlsInJ)) {
        case json_type_object: {

            index = halGetCtlIndex (request, ctlsInJ);
            if (index < 0) goto OnErrorExit;
            json_object_array_add (ctlsOutJ, json_object_new_int(halCtls[index].ctl.numid));
            break;
        }
        
        case json_type_array: {
            
            for (int idx= 0; idx < json_object_array_length (ctlsInJ); idx++) {
                json_object *ctlInJ = json_object_array_get_idx (ctlsInJ, idx);
                index= halGetCtlIndex (request, ctlInJ);
                if (index < 0) goto OnErrorExit;
                json_object_array_add (ctlsOutJ, json_object_new_int(halCtls[index].ctl.numid));
            }
            break;
        }
                
        default:
            afb_req_fail_f(request, "ctl-invalid", "Not a valid JSON ctl='%s'", json_object_get_string(ctlsInJ));
            goto OnErrorExit;                
    }

    // Call now level CTL
    responseJ = halCallAlsaGetCtls (ctlsOutJ);
    if (!responseJ) {
        afb_req_fail_f(request, "subcall:alsacore/getctl", "%s", json_object_get_string(responseJ));
        goto OnErrorExit;        
    }
    
    // map back low level response to HAL ctl with normalised values
    json_object *halResponse =  HalGetPrepareResponse(request, responseJ);
    if (!halResponse) goto OnErrorExit;
   
    afb_req_success (request, halResponse, NULL);
    return;

OnErrorExit:
    return;
};


STATIC int UpdateOneSndCtl (alsaHalCtlMapT *ctl, json_object *sndCtlJ) {
    json_object *tmpJ, *ctlJ;

    json_object_object_get_ex (sndCtlJ, "name" , &tmpJ);
    ctl->name  = (char*)json_object_get_string(tmpJ);
    
    json_object_object_get_ex (sndCtlJ, "id" , &tmpJ);
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
        json_object *dbscaleJ;
        
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
    json_object *queryurl, *responseJ, *devidJ, *ctlsJ, *tmpJ;
    alsaHalMapT *halCtls= alsaHalSndCard->ctls;
    
    // if not volume normalisation CB provided use default one
    if (!alsaHalSndCard->volumeCB) alsaHalSndCard->volumeCB=volumeNormalise;
    halSndCard= alsaHalSndCard;
    
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
    halSndCard->devid = strdup (json_object_get_string(devidJ));
            
    // for each Non Alsa Control callback create a custom control
    ctlsJ= json_object_new_array();
    for (int idx= 0; (halCtls[idx].ctl.name||halCtls[idx].ctl.numid); idx++) {
        json_object *ctlJ;
        
        // map HAL ctlTad with halCrlLabel to simplify set/get ctl operations using Labels
        halCtls[idx].label = halCtlsLabels[halCtls[idx].tag];
        
        if (halCtls[idx].cb.callback != NULL) {
            ctlJ = json_object_new_object();
            if (halCtls[idx].ctl.name)   json_object_object_add(ctlJ, "name" , json_object_new_string(halCtls[idx].ctl.name));
            if (halCtls[idx].ctl.minval) json_object_object_add(ctlJ, "min"  , json_object_new_int(halCtls[idx].ctl.minval));
            if (halCtls[idx].ctl.maxval) json_object_object_add(ctlJ, "max"  ,  json_object_new_int(halCtls[idx].ctl.maxval));
            if (halCtls[idx].ctl.step)   json_object_object_add(ctlJ, "step" , json_object_new_int(halCtls[idx].ctl.step));
            if (halCtls[idx].ctl.type)   json_object_object_add(ctlJ, "type" , json_object_new_int(halCtls[idx].ctl.type));
            if (halCtls[idx].ctl.count)  json_object_object_add(ctlJ, "count", json_object_new_int(halCtls[idx].ctl.count));
            if (halCtls[idx].ctl.value)  json_object_object_add(ctlJ, "value", json_object_new_int(halCtls[idx].ctl.value));
            
            if (halCtls[idx].ctl.dbscale) {
                json_object *dbscaleJ=json_object_new_object();
                if (halCtls[idx].ctl.dbscale->max)  json_object_object_add(dbscaleJ, "max" , json_object_new_int(halCtls[idx].ctl.dbscale->max));
                if (halCtls[idx].ctl.dbscale->min)  json_object_object_add(dbscaleJ, "min" , json_object_new_int(halCtls[idx].ctl.dbscale->min));
                if (halCtls[idx].ctl.dbscale->step) json_object_object_add(dbscaleJ, "step", json_object_new_int(halCtls[idx].ctl.dbscale->step));
                if (halCtls[idx].ctl.dbscale->mute) json_object_object_add(dbscaleJ, "mute", json_object_new_int(halCtls[idx].ctl.dbscale->mute));
                 json_object_object_add(ctlJ, "dbscale" , dbscaleJ);
            }
            
            if (halCtls[idx].ctl.enums)  {
                json_object *enumsJ=json_object_new_array();
                for (int jdx=0; halCtls[idx].ctl.enums[jdx]; jdx++) {
                    json_object_array_add(enumsJ, json_object_new_string(halCtls[idx].ctl.enums[jdx]));
                }
                json_object_object_add(ctlJ, "enums" , enumsJ);
            }
            json_object_array_add(ctlsJ, ctlJ);             
        } else  {
            ctlJ = json_object_new_object();
            if (halCtls[idx].ctl.numid)  json_object_object_add(ctlJ, "ctl" , json_object_new_int(halCtls[idx].ctl.numid));
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


// This receive all event this binding subscribe to 
PUBLIC void halServiceEvent(const char *evtname, json_object *eventJ) {
    int numid;
    alsaHalMapT *halCtls= halSndCard->ctls;
    json_object *numidJ, *valuesJ;
    
    AFB_DEBUG("halServiceEvent evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));
    
    json_object_object_get_ex (eventJ, "id" , &numidJ);
    numid = json_object_get_int (numidJ);
    if (!numid) {
        AFB_ERROR("halServiceEvent noid: evtname=%s [msg=%s]", evtname, json_object_get_string(eventJ));
        return;
    }
    json_object_object_get_ex (eventJ, "val" , &valuesJ);
    
    // search it corresponding numid in halCtls attach a callback
    if (numid) {
        for (int idx= 0; halCtls[idx].ctl.numid; idx++) {
            if (halCtls[idx].ctl.numid == numid && halCtls[idx].cb.callback != NULL) {
                halCtls[idx].cb.callback (halCtls[idx].tag, &halCtls[idx].ctl, halCtls[idx].cb.handle, valuesJ);
            }
        }
    }    
}

// Every HAL export the same API & Interface Mapping from SndCard to AudioLogic is done through alsaHalSndCardT
PUBLIC afb_verb_v2 halServiceApi[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping",     .callback = pingtest    , .info="ping test for API"},
    { .verb = "ctl-list", .callback = halListCtls , .info="List AGL normalised Sound Controls"},
    { .verb = "ctl-get",  .callback = halGetCtls  , .info="Get one/many sound controls"},
    { .verb = "ctl-set",  .callback = halSetCtls  , .info="Set one/many sound controls"},
    { .verb = "evt-sub",  .callback = halSubscribe, .info="Subscribe to HAL events"},
    { .verb = NULL} /* marker for end of the array */
};
