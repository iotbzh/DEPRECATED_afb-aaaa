/*
 * AlsaUseCase -- provide low level interface with ALSA lib (extracted from alsa-json-gateway code)
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
#define _GNU_SOURCE  // needed for vasprintf

#include <alsa/asoundlib.h>
#include <systemd/sd-event.h>
#include "Alsa-ApiHat.h"

STATIC int addOneSndCtl(struct afb_req request, snd_ctl_t  *ctlDev, json_object *ctlJ) {
    snd_ctl_elem_info_t *cinfo;
    int err;
    int i;
    unsigned int def_val;
    json_object *jName, *jNumid, *jTmp;
    const char *ctlName;
    int  ctlNumid, ctlMax, ctlMin, ctlStep, ctlCount, ctlSubDev;
    snd_ctl_elem_type_t  ctlType;
    snd_ctl_elem_info_t  *elemInfo;
    
    // parse json ctl object
    json_object_object_get_ex (ctlJ, "name" , &jName);
    json_object_object_get_ex (ctlJ, "numid", &jNumid);
    if (!jName || !jNumid) {
        afb_req_fail_f (request, "ctl-invalid", "crl=%s name/numid missing", json_object_to_json_string(ctlJ));
        goto OnErrorExit;
    }
    
    ctlName  = json_object_to_json_string(jName);
    ctlNumid = json_object_get_int(jNumid);

    // default for json_object_get_int is zero
    json_object_object_get_ex (ctlJ, "min" , &jTmp);
    ctlMin = json_object_get_int(jTmp);

    json_object_object_get_ex (ctlJ, "max" , &jTmp);
    if (!jTmp) ctlMax=1;
    else ctlMax = json_object_get_int(jTmp);
    
    json_object_object_get_ex (ctlJ, "step" , &jTmp);
    if (!jTmp) ctlStep=1;
    else ctlStep = json_object_get_int(jTmp);
    
    json_object_object_get_ex (ctlJ, "count" , &jTmp);
    if (!jTmp) ctlCount=2;
    else ctlCount = json_object_get_int(jTmp);

    json_object_object_get_ex (ctlJ, "subdev" , &jTmp);
    ctlSubDev = json_object_get_int(jTmp);
   
    json_object_object_get_ex (ctlJ, "type" , &jTmp);
    if (!jTmp) ctlType=SND_CTL_ELEM_TYPE_INTEGER;
    else ctlType = json_object_get_int(jTmp);
    
    // set info event ID and get value
    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_info_set_name (elemInfo, ctlName);  // map ctlInfo to ctlId elemInfo is updated !!!
    snd_ctl_elem_info_set_numid(elemInfo, ctlNumid);  // map ctlInfo to ctlId elemInfo is updated !!!
    
    if (snd_ctl_elem_info(ctlDev, elemInfo) >= 0) {
        afb_req_fail_f (request, "ctl-already-exist", "crl=%s name/numid not unique", json_object_to_json_string(ctlJ));
        goto OnErrorExit;
    }    
    
    snd_ctl_elem_info_set_subdevice(elemInfo, ctlSubDev);
    
        switch (ctlType) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            err = snd_ctl_add_boolean_elem_set(ctlDev, cinfo, 1, ctlCount);
            if (err) {
                afb_req_fail_f (request, "ctl-invalid-bool", "crl=%s invalid boolean data", json_object_to_json_string(ctlJ));
                goto OnErrorExit;                
            }            
            break;
            
        case SND_CTL_ELEM_TYPE_INTEGER:
            break;
            
        case SND_CTL_ELEM_TYPE_INTEGER64:
            break;
            
        case SND_CTL_ELEM_TYPE_ENUMERATED:
            break;
            
        case SND_CTL_ELEM_TYPE_BYTES:
            break;
            
        default:
            afb_req_fail_f (request, "ctl-invalid-type", "crl=%s invalid/unknown type", json_object_to_json_string(ctlJ));
            goto OnErrorExit;                
        }

 
    snd_ctl_elem_write(ctlDev, &ctlElem);
    
    return 0;
    
    OnErrorExit:
        return -1;
}

// Subscribe to every Alsa CtlEvent send by a given board
PUBLIC void alsaAddCustomCtl (struct afb_req request) {
    snd_ctl_t  *ctlDev;
    int err;
    const char *devid, *ctls;
    json_object *ctlsJ;
    
    devid = afb_req_value(request, "devid");
    if (!devid) {
        afb_req_fail_f (request, "missing-devid", "devid=xxx missing");
        goto OnErrorExit;
    }
           
    // open control interface for devid
    err = snd_ctl_open(&ctlDev, devid, 0);
    if (err < 0) {
        ctlDev=NULL;
        afb_req_fail_f (request, "devid-unknown", "SndCard devid=%s Not Found err=%s", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    ctls = afb_req_value(request, "ctls");
    if (!ctls) {
        afb_req_fail_f (request, "missing-ctls", "ctls=[{name:xxx, numdid=xx, ...}] missing");
        goto OnErrorExit;
    }
    
    ctlsJ = json_tokener_parse(ctls);
    if (!ctlsJ) {
        afb_req_fail_f (request, "ctls-notjson","ctls=%s not a valid json entry", ctls);
        goto OnErrorExit;        
    };
    
    enum json_type jtype= json_object_get_type(ctlsJ);
    switch (jtype) {
        int error;
        json_object *ctlJ;
        
        case json_type_array:
            int count = json_object_array_length (ctlsJ);
            for (int idx=0; idx < count; idx ++) {                
                ctlJ = json_object_array_get_idx (ctlsJ, idx);
                error = alsaAddCtrl (request, ctlDev, ctlsJ);
                if (error)  goto OnErrorExit;
            }        
            break;
            
        case json_type_object:
            error = alsaAddCtrl (request, ctlDev, ctlsJ);
            if (error)  goto OnErrorExit;
            break;
        
        default:           
            afb_req_fail_f (request, "ctls-notarray","ctls=%s not valid JSON control object", ctls);
            goto OnErrorExit;        
    }

    snd_ctl_close(ctlDev);
    return;
    
  OnErrorExit:
        if (ctlDev) snd_ctl_close(ctlDev);            
        return;
}

