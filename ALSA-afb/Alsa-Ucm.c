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

   References:
      http://www.alsa-project.org/alsa-doc/alsa-lib/group__ucm.html
      https://www.alsa-project.org/main/index.php/DAPM
      http://alsa-lib.sourcearchive.com/documentation/1.0.24.1-2/group__Use_ga4332c6bb50481bbdaf21be11551fb930.html
      https://android.googlesource.com/platform/hardware/qcom/audio/+/jb-mr1-dev/libalsa-intf/alsa_ucm.h
 
   Sample alsaucm commands using /usr/share/alsa/ucm/PandaBoard 
    - alsaucm  -c PandaBoard list _verbs
    - alsaucm  -c PandaBoard list _devices/HiFi
    - alsaucm  -c PandaBoard list _modifiers/HiFi  #need to uncomment modifiers section
    - alsaucm  -c PandaBoard list TQ/HiFi
    - alsaucm  -c PandaBoard get TQ/HiFi/Voice
    - alsaucm  -c PandaBoard get PlaybackPCM//HiFi
    - alsaucm  -c PandaBoard set _verb HiFi
    - alsaucm  -c PandaBoard set _verb HiFi _enadev Headset
    - alsaucm  -c 'HDA Intel PCH'  set _verb HiFi set  _enadev Headphone set _enamod RecordMedia
    - alsaucm  -c 'HDA Intel PCH'  set _verb HiFi get OutputDspName//

*/
#define _GNU_SOURCE  // needed for vasprintf

#include <alsa/asoundlib.h>
#include <alsa/asoundlib.h>
#include <alsa/use-case.h>

#include "Alsa-ApiHat.h"

typedef struct {
     snd_use_case_mgr_t *ucm;
     int  cardId;
     char *cardName;
}    ucmHandleT;

static ucmHandleT ucmHandles[MAX_SND_CARD];

// Cache opened UCM handles 
STATIC int alsaUseCaseOpen (struct afb_req request, queryValuesT *queryValues, int allowNewMgr) {
    snd_ctl_t   *ctlDev;
    snd_ctl_card_info_t *cardinfo;
    snd_use_case_mgr_t *ucmHandle;
    const char *cardName;
    int cardId, idx, idxFree=-1, err;
    
    // open control interface for devid
    err = snd_ctl_open(&ctlDev, queryValues->devid, SND_CTL_READONLY);
    if (err < 0) {
        ctlDev=NULL;
        afb_req_fail_f (request, "devid-unknown", "SndCard devid=[%s] Not Found err=%d", queryValues->devid, err);
        goto OnErrorExit;
    }
    
    snd_ctl_card_info_alloca(&cardinfo);    
    if ((err = snd_ctl_card_info(ctlDev, cardinfo)) < 0) {        
        afb_req_fail_f (request, "devid-invalid", "SndCard devid=[%s] Not Found err=%s", queryValues->devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    // search for an existing subscription and mark 1st free slot
    cardId   = snd_ctl_card_info_get_card(cardinfo); 
    for (idx= 0; idx < MAX_SND_CARD; idx ++) {
      if (ucmHandles[idx].ucm != NULL) {
          if (ucmHandles[idx].cardId == cardId) goto OnSuccessExit;
      } else if (idxFree == -1) idxFree= idx;
    };
    
    if (!allowNewMgr) {
        afb_req_fail_f (request, "ucm-nomgr", "SndCard devid=[%s] no exiting UCM manager session", queryValues->devid);
        goto OnErrorExit;                
    }

    if (idxFree < 0 && idx == MAX_SND_CARD) {
        afb_req_fail_f (request, "ucm-toomany", "SndCard devid=[%s] too many open UCM Max=%d", queryValues->devid, MAX_SND_CARD);
        goto OnErrorExit;        
    }
    
    idx = idxFree;
    cardName = snd_ctl_card_info_get_name(cardinfo); 
    err = snd_use_case_mgr_open(&ucmHandle, cardName);
    if (err) {
       afb_req_fail_f (request, "ucm-open", "SndCard devid=[%s] name=[%s] No UCM Profile err=%s", queryValues->devid, cardName, snd_strerror(err));
       goto OnErrorExit; 
    }
    ucmHandles[idx].ucm = ucmHandle;
    ucmHandles[idx].cardId = cardId;        
    ucmHandles[idx].cardName = strdup(cardName);        

  OnSuccessExit:
    if (ctlDev) snd_ctl_close(ctlDev);            
    return idx;
    
  OnErrorExit:   
    if (ctlDev) snd_ctl_close(ctlDev);            
    return -1;
}
      

PUBLIC void alsaUseCaseQuery(struct afb_req request) { 
    int err, verbCount, ucmIdx;
    const char **verbList;
    snd_use_case_mgr_t *ucmHandle;
    queryValuesT queryValues;
    json_object *ucmJs;
    const char *cardName;

    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    
    ucmIdx = alsaUseCaseOpen (request, &queryValues, TRUE);
    if (ucmIdx < 0) goto OnErrorExit;
    ucmHandle = ucmHandles [ucmIdx].ucm;
    cardName  = ucmHandles [ucmIdx].cardName;
    
    verbCount = snd_use_case_get_list (ucmHandle, "_verbs", &verbList);
    if (verbCount < 0) {
       afb_req_fail_f (request, "ucm-list", "SndCard devid=[%s] name=[%s] No UCM Verbs", queryValues.devid, cardName);
       goto OnErrorExit; 
    }
    
    ucmJs = json_object_new_array();
    for (int idx=0; idx < verbCount; idx +=2) {
        int devCount, modCount, tqCount;
        const char **devList, **modList, **tqList;
        json_object *ucmJ = json_object_new_object();
        char identifier[32];
        
        json_object_object_add (ucmJ, "verb", json_object_new_string(verbList[idx]));
        if (verbList[idx+1]) json_object_object_add (ucmJ, "info", json_object_new_string(verbList[idx+1]));
        
        DEBUG (afbIface, "Verb[%d] Action=%s Info=%s", idx, verbList[idx], verbList[idx+1]);

        snprintf (identifier, sizeof(identifier), "_devices/%s", verbList[idx]);
        devCount = snd_use_case_get_list (ucmHandle, identifier, &devList);
        if (devCount > 0) {
            json_object *devsJ = json_object_new_array();
            
            for (int jdx=0; jdx < devCount; jdx+=2) {
                json_object *devJ = json_object_new_object();
                DEBUG (afbIface, "device[%d] Action=%s Info=%s", jdx, devList[jdx], devList[jdx+1]);
                json_object_object_add (devJ, "dev", json_object_new_string(devList[jdx]));
                if (devList[jdx+1]) json_object_object_add (devJ, "info", json_object_new_string(devList[jdx+1]));                
                json_object_array_add (devsJ, devJ);
            }
            json_object_object_add(ucmJ,"devices", devsJ);
            snd_use_case_free_list(devList, err);
        }
        
        snprintf (identifier, sizeof(identifier), "_modifiers/%s", verbList[idx]);
        modCount = snd_use_case_get_list (ucmHandle, identifier, &modList);
        if (modCount > 0) {
            json_object *modsJ = json_object_new_array();
            
            for (int jdx=0; jdx < modCount; jdx+=2) {
                json_object *modJ = json_object_new_object();
                DEBUG (afbIface, "modifier[%d] Action=%s Info=%s", jdx, modList[jdx], modList[jdx+1]);
                json_object_object_add (modJ, "mod", json_object_new_string(modList[jdx]));
                if (modList[jdx+1]) json_object_object_add (modJ, "info", json_object_new_string(modList[jdx+1]));                
                json_object_array_add (modsJ, modJ);
            }
            json_object_object_add(ucmJ,"modifiers", modsJ);
            snd_use_case_free_list(modList, err);
        }

        snprintf (identifier, sizeof(identifier), "TQ/%s", verbList[idx]);
        tqCount = snd_use_case_get_list (ucmHandle, identifier, &tqList);
        if (tqCount > 0) {
            json_object *tqsJ = json_object_new_array();
            
            for (int jdx=0; jdx < tqCount; jdx+=2) {
                json_object *tqJ = json_object_new_object();
                DEBUG (afbIface, "toneqa[%d] Action=%s Info=%s", jdx, tqList[jdx], tqList[jdx+1]);
                json_object_object_add (tqJ, "tq", json_object_new_string(tqList[jdx]));
                if (tqList[jdx+1]) json_object_object_add (tqJ, "info", json_object_new_string(tqList[jdx+1]));                
                json_object_array_add (tqsJ, tqJ);
            }
            json_object_object_add(ucmJ,"tqs", tqsJ);
            snd_use_case_free_list(tqList, err);
        }
        
        json_object_array_add (ucmJs, ucmJ);
    }
    
    afb_req_success (request, ucmJs, NULL);
    snd_use_case_free_list(verbList, err);
        
  OnErrorExit:
    return;
}

STATIC json_object *ucmGetValue (ucmHandleT *ucmHandle, const char *verb, const char *mod, const char *label) {
    char identifier[80];
    char *value;
    int err;
    json_object *jValue;
    
    // handle optional parameters
    if (!mod)  mod=""; 
    if (!verb) verb="";
    
    if (!label) {
        NOTICE (afbIface, "ucmGetValue cardname=[%s] value label missing", ucmHandle->cardName);
        goto OnErrorExit;        
    }
    
    snprintf (identifier, sizeof(identifier), "%s/%s/%s", label, mod, verb);
    err = snd_use_case_get (ucmHandle->ucm, identifier, (const char**)&value); // Note: value casting is a known "FEATURE" of AlsaUCM API
    if (err) {
        DEBUG (afbIface, "ucmGetValue cardname=[%s] identifier=[%s] error=%s", ucmHandle->cardName, identifier, snd_strerror (err));
        goto OnErrorExit;
    }
    
    // copy value into json object and free string
    jValue = json_object_new_string (value);
    free (value);
    return (jValue);    
    
  OnErrorExit:
    return (NULL);
}

PUBLIC void alsaUseCaseGet (struct afb_req request) {
    int err, ucmIdx, labelCount;
    queryValuesT queryValues;
    json_object *jResponse = json_object_new_object();
    json_object *jWarnings = json_object_new_array();
    const char *warnings=NULL;
    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    
    ucmIdx = alsaUseCaseOpen (request, &queryValues, TRUE);
    if (ucmIdx < 0) goto OnErrorExit;
    
    const char *cardName= ucmHandles[ucmIdx].cardName;
    
    const char *verb  = afb_req_value(request, "verb");
    const char *mod   = afb_req_value(request, "mod");
    const char *dev   = afb_req_value(request, "dev");
    
    if (dev && mod) {
        afb_req_fail_f (request, "ucmget-labels", "SndCard devid=[%s] name=[%s] UCM mod+dev incompatible", queryValues.devid, cardName);
        goto OnErrorExit;          
    } 
    
    // device selection is handle as a modifier
    if (dev) mod=dev;

    const char *labels = afb_req_value(request, "values");
    if (!labels) {
        afb_req_fail_f (request, "ucmget-labels", "SndCard devid=[%s] name=[%s] UCM values name missing", queryValues.devid, cardName);
        goto OnErrorExit;  
    } 
    
    json_object *jLabels = json_tokener_parse(labels);
    if (!jLabels) {
        afb_req_fail_f (request, "ucmget-notjson","labels=%s not a valid json entry", labels);
        goto OnErrorExit;        
    };
    
    enum json_type jtype= json_object_get_type(jLabels);
    switch (jtype) {
        json_object *jTmp;
        
        case json_type_array:
            labelCount = json_object_array_length (jLabels);
            break;
            
        case json_type_string:
            jTmp = json_object_new_array ();
            labelCount = 1;
            json_object_array_add (jTmp, jLabels);
            jLabels=jTmp;
            break;
        
        default:           
            afb_req_fail_f (request, "ucmget-notarray","labels=%s not valid JSON array", labels);
            goto OnErrorExit;        
    }

    for (int idx=0; idx < labelCount; idx++) {
        json_object *jValue, *jLabel;
        const char *label ;
        
        jLabel= json_object_array_get_idx (jLabels, idx);
        label= json_object_get_string (jLabel);
        jValue = ucmGetValue (&ucmHandles[ucmIdx], verb, mod, label);
        if (jValue) json_object_object_add (jResponse, label, jValue);
        else {
            json_object_array_add (jWarnings, jLabel);
        }
    }
    
    // use info section to notified not found values label
    if (json_object_array_length (jWarnings) > 0) {
       json_object *jTmp =  json_object_new_object ();
       json_object_object_add (jTmp, "no-context", jWarnings);
       warnings= json_object_to_json_string (jTmp);
    }
    afb_req_success (request, jResponse, warnings);
    
  OnErrorExit:
    return;        
}
    
PUBLIC void alsaUseCaseSet(struct afb_req request) { 
    int err, ucmIdx;
    queryValuesT queryValues;
    json_object *jResponse = json_object_new_object();
    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    
    ucmIdx = alsaUseCaseOpen (request, &queryValues, TRUE);
    if (ucmIdx < 0) goto OnErrorExit;
    
    snd_use_case_mgr_t *ucmMgr= ucmHandles[ucmIdx].ucm;
    const char *cardName= ucmHandles[ucmIdx].cardName;
    
    const char *verb = afb_req_value(request, "verb");
    const char *mod  = afb_req_value(request, "mod");
    const char *dev  = afb_req_value(request, "dev");
    // Known identifiers: _verb - set current verb = value _enadev - enable given device = value _disdev - disable given device = value _swdev/{old_device} - new_device = value

    if (verb) {
        err = snd_use_case_set (ucmMgr, "_verb", verb);
        if (err) {        
            afb_req_fail_f (request, "ucmset-verb", "SndCard devid=[%s] name=[%s] Invalid UCM verb=[%s] err=%s", queryValues.devid, cardName, verb, snd_strerror(err));
            goto OnErrorExit;
        }
    }

    if (dev) {
        err = snd_use_case_set (ucmMgr, "_enadev", dev);
        if (err) {        
            afb_req_fail_f (request, "ucmset-dev", "SndCard devid=[%s] name=[%s] Invalid UCMverb=[%s] dev=%s err=%s", queryValues.devid, cardName, verb, dev, snd_strerror(err));
            goto OnErrorExit;
        }
    }
    
    if (mod) {
        err = snd_use_case_set (ucmMgr, "_enamod", mod);
        if (err) {        
            afb_req_fail_f (request, "ucmset-mod", "SndCard devid=[%s] name=[%s] Invalid UCM verb=[%s] mod=[%s] err=%s", queryValues.devid, cardName, verb, mod, snd_strerror(err));
            goto OnErrorExit;
        }
    }
    
    // label are requested transfert request to get
    if (afb_req_value(request, "values")) return alsaUseCaseGet(request);
    
    if (queryValues.quiet <= 3) {
        json_object *jValue;

        jValue = ucmGetValue (&ucmHandles[ucmIdx], verb, dev, "OutputDspName");
        if (jValue) json_object_object_add (jResponse, "OutputDspName", jValue);
        
        jValue = ucmGetValue (&ucmHandles[ucmIdx], verb, dev, "PlaybackPCM");
        if (jValue) json_object_object_add (jResponse, "PlaybackPCM", jValue);

        jValue = ucmGetValue (&ucmHandles[ucmIdx], verb, mod, "CapturePCM");
        if (jValue) json_object_object_add (jResponse, "CapturePCM", jValue);
    }
    afb_req_success (request, jResponse, NULL);
 
  OnErrorExit:
    return;
}



PUBLIC void alsaUseCaseReset (struct afb_req request) {
    int err, ucmIdx;
    queryValuesT queryValues;
    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    
    ucmIdx = alsaUseCaseOpen (request, &queryValues, FALSE);
    if (ucmIdx < 0) goto OnErrorExit;
    
    err= snd_use_case_mgr_reset (ucmHandles[ucmIdx].ucm);
    if (err) {
        afb_req_fail_f (request, "ucmreset-fail","devid=%s Card Name=%s", queryValues.devid, ucmHandles[ucmIdx].cardName);
        goto OnErrorExit;        
    }
    
    afb_req_success (request, NULL, NULL);
    
  OnErrorExit:
    return;        
}
    
PUBLIC void alsaUseCaseClose (struct afb_req request) {
    int err, ucmIdx;
    queryValuesT queryValues;
    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    
    ucmIdx = alsaUseCaseOpen (request, &queryValues, FALSE);
    if (ucmIdx < 0) goto OnErrorExit;
    
    err= snd_use_case_mgr_close (ucmHandles[ucmIdx].ucm);
    if (err) {
        afb_req_fail_f (request, "ucmreset-close","devid=%s Card Name=%s", queryValues.devid, ucmHandles[ucmIdx].cardName);
        goto OnErrorExit;        
    }
    
    // do not forget to release sound card name string
    free (ucmHandles[ucmIdx].cardName);
    
    afb_req_success (request, NULL, NULL);
    
  OnErrorExit:
    return;        
}
    

