/*
 * AlsaLibMapping -- provide low level interface with ALSA lib (extracted from alsa-json-gateway code)
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
   https://github.com/fulup-bzh/AlsaJsonGateway (original code)
   http://alsa-lib.sourcearchive.com/documentation/1.0.20/modules.html
   http://alsa-lib.sourcearchive.com/documentation/1.0.8/group__Control_gd48d44da8e3bfe150e928267008b8ff5.html
   http://alsa.opensrc.org/HowTo_access_a_mixer_control
   https://github.com/gch1p/alsa-volume-monitor/blob/master/main.c
   https://github.com/DongheonKim/android_hardware_alsa-sound/blob/master/ALSAControl.cpp (ALSA low level API)

*/

#define _GNU_SOURCE  // needed for vasprintf

#include <alsa/asoundlib.h>
#include <systemd/sd-event.h>

#include "Alsa-ApiHat.h"

// use to store crl numid user request
typedef struct {
    unsigned int numId;
    json_object *jToken;
    json_object *jValues;
    int used;
} ctlRequestT;

// generic sndctrl event handle hook to event callback when pooling
typedef struct {
    struct pollfd pfds;
    sd_event_source *src;
    snd_ctl_t  *ctlDev;
    int quiet;
    struct afb_event afbevt;
} evtHandleT;

typedef struct {
    int ucount;
    int cardId;
    evtHandleT *evtHandle;
} sndHandleT;

typedef struct {
    char *apiprefix;
    char *shortname;    
}cardRegistryT;

cardRegistryT *cardRegistry[MAX_SND_CARD+1];

PUBLIC void NumidsListParse (queryValuesT *queryValues, ctlRequestT *ctlRequest) {
    json_object *jValues;
    int length;
    
    for (int idx=0; idx < queryValues->count; idx ++) {                
        ctlRequest[idx].jToken = json_object_array_get_idx (queryValues->jNumIds, idx);
        ctlRequest[idx].jValues = NULL;
        ctlRequest[idx].used=0;

        enum json_type jtype=json_object_get_type(ctlRequest[idx].jToken);
        switch (jtype) {
            json_object *jId, *jVal;
            
            case json_type_int:
                // if NUMID is not an array then it should be an integer numid with no value
                ctlRequest[idx].numId = json_object_get_int (ctlRequest[idx].jToken);
                break;

            case  json_type_array:   
                // NUMID is an array 1st slot should be numid, optionally values may come after
                length=json_object_array_length (ctlRequest[idx].jToken);
                if (length < 1 || length >2) {
                    ctlRequest[idx].used=-1;
                    continue; 
                }

                ctlRequest[idx].numId =json_object_get_int(json_object_array_get_idx (ctlRequest[idx].jToken, 0));

                if (length == 2) {
                    jValues = json_object_array_get_idx (ctlRequest[idx].jToken, 1);
                    if (jValues == NULL) {
                        ctlRequest[idx].used=-1;
                        continue; 
                    }            
                    // Value is an int or an array with potentially multiple subvalues
                    ctlRequest[idx].jValues = jValues;
                }
                break;
                
            case json_type_object:
                // numid+values formated as {id:xxx, val:[aa,bb...,nn]}
                if (!json_object_object_get_ex (ctlRequest[idx].jToken,"id", &jId) || !json_object_object_get_ex (ctlRequest[idx].jToken,"val",&jVal)) {
                    NOTICE (afbIface,"Invalid Json=%s missing 'id'|'val'", json_object_get_string(ctlRequest[idx].jToken));
                    ctlRequest[idx].used=-1;
                } else {
                    ctlRequest[idx].numId   =json_object_get_int(jId);
                    ctlRequest[idx].jValues =jVal;
                }
                
                
            default:                
                ctlRequest[idx].used=-1;   
        }
    }
}

PUBLIC  int alsaCheckQuery (struct afb_req request, queryValuesT *queryValues) {

    queryValues->devid = afb_req_value(request, "devid");
    if (queryValues->devid == NULL) goto OnErrorExit;
    const char *numids;
    json_object *jNumIds;

    const char *rqtQuiet = afb_req_value(request, "quiet");
    if (!rqtQuiet) queryValues->quiet=99; // default super quiet
    else if (rqtQuiet && ! sscanf (rqtQuiet, "%d", &queryValues->quiet)) {
        json_object *query = afb_req_json(request);
        
        afb_req_fail_f (request, "quiet-notinteger","Query=%s Quiet not integer &quiet=%s&", json_object_to_json_string(query), rqtQuiet);
        goto OnErrorExit;
    };
   
    // no NumId is interpreted as ALL for get and error for set
    numids = afb_req_value(request, "numids");
    if (numids == NULL) {
        queryValues->count=0;
        goto OnExit;
    }

    jNumIds = json_tokener_parse(numids);
    if (!jNumIds) {
        afb_req_fail_f (request, "numids-notjson","numids=%s not a valid json entry", numids);
        goto OnErrorExit;        
    };
    
    enum json_type jtype= json_object_get_type(jNumIds);
    switch (jtype) {
        case json_type_array:
            queryValues->jNumIds = jNumIds;
            queryValues->count = json_object_array_length (jNumIds);
            break;
            
        case json_type_int:
            queryValues->count = 1;
            queryValues->jNumIds = json_object_new_array ();
            json_object_array_add (queryValues->jNumIds, jNumIds);
            break;
        
        default:           
            afb_req_fail_f (request, "numid-notarray","NumId=%s NumId not valid JSON array", numids);
            goto OnErrorExit;        
    }

OnExit:    
    return 0;
    
OnErrorExit:
    return 1;
}

STATIC json_object *DB2StringJsonOject (long dB) {
    char label [20];
	if (dB < 0) {
		snprintf(label, sizeof(label), "-%li.%02lidB", -dB / 100, -dB % 100);
	} else {
		snprintf(label, sizeof(label), "%li.%02lidB", dB / 100, dB % 100);
	}

    // json function takes care of string copy
	return (json_object_new_string (label));
}


// Direct port from amixer TLV decode routine. This code is too complex for me.
// I hopefully did not break it when porting it.

STATIC json_object *decodeTlv(unsigned int *tlv, unsigned int tlv_size) {
    char label[20];
    unsigned int type = tlv[0];
    unsigned int size;
    unsigned int idx = 0;
    const char *chmap_type = NULL;
    json_object * decodeTlvJson = json_object_new_object();

    if (tlv_size < (unsigned int) (2 * sizeof (unsigned int))) {
        printf("TLV size error!\n");
        return NULL;
    }
    type = tlv[idx++];
    size = tlv[idx++];
    tlv_size -= (unsigned int) (2 * sizeof (unsigned int));
    if (size > tlv_size) {
        fprintf(stderr, "TLV size error (%i, %i, %i)!\n", type, size, tlv_size);
        return NULL;
    }
    switch (type) {

        case SND_CTL_TLVT_CONTAINER:
        {
            json_object * containerJson = json_object_new_array();

            size += (unsigned int) (sizeof (unsigned int) - 1);
            size /= (unsigned int) (sizeof (unsigned int));
            while (idx < size) {
                json_object *embedJson;

                if (tlv[idx + 1] > (size - idx) * sizeof (unsigned int)) {
                    fprintf(stderr, "TLV size error in compound!\n");
                    return NULL;
                }
                embedJson = decodeTlv(tlv + idx, tlv[idx + 1] + 8);
                json_object_array_add(containerJson, embedJson);
                idx += (unsigned int) (2 + (tlv[idx + 1] + sizeof (unsigned int) - 1) / sizeof (unsigned int));
            }
            json_object_object_add(decodeTlvJson, "container", containerJson);
            break;
        }

        case SND_CTL_TLVT_DB_SCALE:
        {
            json_object * dbscaleJson = json_object_new_object();

            if (size != 2 * sizeof (unsigned int)) {
                json_object * arrayJson = json_object_new_array();
                while (size > 0) {
                    snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                    json_object_array_add(arrayJson, json_object_new_string(label));
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbscaleJson, "array", arrayJson);
            } else {
                json_object_object_add(dbscaleJson, "min", DB2StringJsonOject((int) tlv[2]));
                json_object_object_add(dbscaleJson, "step", DB2StringJsonOject(tlv[3] & 0xffff));
                json_object_object_add(dbscaleJson, "mute", DB2StringJsonOject((tlv[3] >> 16) & 1));
            }
            json_object_object_add(decodeTlvJson, "dbscale", dbscaleJson);
            break;
        }

#ifdef SND_CTL_TLVT_DB_LINEAR
        case SND_CTL_TLVT_DB_LINEAR:
        {
            json_object * dbLinearJson = json_object_new_object();

            if (size != 2 * sizeof (unsigned int)) {
                json_object * arrayJson = json_object_new_array();
                while (size > 0) {
                    snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                    json_object_array_add(arrayJson, json_object_new_string(label));
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbLinearJson, "offset", arrayJson);
            } else {
                json_object_object_add(dbLinearJson, "min", DB2StringJsonOject((int) tlv[2]));
                json_object_object_add(dbLinearJson, "max", DB2StringJsonOject((int) tlv[3]));
            }
            json_object_object_add(decodeTlvJson, "dblinear", dbLinearJson);
            break;
        }
#endif

#ifdef SND_CTL_TLVT_DB_RANGE
        case SND_CTL_TLVT_DB_RANGE:
        {
            json_object *dbRangeJson = json_object_new_object();

            if ((size % (6 * sizeof (unsigned int))) != 0) {
                json_object *arrayJson = json_object_new_array();
                while (size > 0) {
                    snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                    json_object_array_add(arrayJson, json_object_new_string(label));
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbRangeJson, "dbrange", arrayJson);
                break;
            }
            while (size > 0) {
                json_object * embedJson = json_object_new_object();
                snprintf(label, sizeof (label), "%i,", tlv[idx++]);
                json_object_object_add(embedJson, "rangemin", json_object_new_string(label));
                snprintf(label, sizeof (label), "%i", tlv[idx++]);
                json_object_object_add(embedJson, "rangemax", json_object_new_string(label));
                embedJson = decodeTlv(tlv + idx, 4 * sizeof (unsigned int));
                json_object_object_add(embedJson, "tlv", embedJson);
                idx += 4;
                size -= (unsigned int) (6 * sizeof (unsigned int));
                json_object_array_add(dbRangeJson, embedJson);
            }
            json_object_object_add(decodeTlvJson, "dbrange", dbRangeJson);
            break;
        }
#endif
#ifdef SND_CTL_TLVT_DB_MINMAX
        case SND_CTL_TLVT_DB_MINMAX:
        case SND_CTL_TLVT_DB_MINMAX_MUTE:
        {
            json_object * dbMinMaxJson = json_object_new_object();

            if (size != 2 * sizeof (unsigned int)) {
                json_object * arrayJson = json_object_new_array();
                while (size > 0) {
                    snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                    json_object_array_add(arrayJson, json_object_new_string(label));
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbMinMaxJson, "array", arrayJson);

            } else {
                json_object_object_add(dbMinMaxJson, "min", DB2StringJsonOject((int) tlv[2]));
                json_object_object_add(dbMinMaxJson, "max", DB2StringJsonOject((int) tlv[3]));
            }

            if (type == SND_CTL_TLVT_DB_MINMAX_MUTE) {
                json_object_object_add(decodeTlvJson, "dbminmaxmute", dbMinMaxJson);
            } else {
                json_object_object_add(decodeTlvJson, "dbminmax", dbMinMaxJson);
            }
            break;
        }
#endif
#ifdef SND_CTL_TLVT_CHMAP_FIXED
        case SND_CTL_TLVT_CHMAP_FIXED:
            chmap_type = "fixed";
            /* Fall through */
        case SND_CTL_TLVT_CHMAP_VAR:
            if (!chmap_type)
                chmap_type = "variable";
            /* Fall through */
        case SND_CTL_TLVT_CHMAP_PAIRED:
            if (!chmap_type)
                chmap_type = "paired";

            json_object * chmapJson = json_object_new_object();
            json_object * arrayJson = json_object_new_array();

            while (size > 0) {
                snprintf(label, sizeof (label), "%s", snd_pcm_chmap_name(tlv[idx++]));
                size -= (unsigned int) sizeof (unsigned int);
                json_object_array_add(arrayJson, json_object_new_string(label));
            }
            json_object_object_add(chmapJson, chmap_type, arrayJson);
            json_object_object_add(decodeTlvJson, "chmap", chmapJson);
            break;
#endif
        default:
        {
            printf("unk-%i-", type);
            json_object * arrayJson = json_object_new_array();

            while (size > 0) {
                snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                size -= (unsigned int) sizeof (unsigned int);
                json_object_array_add(arrayJson, json_object_new_string(label));
            }
            break;
            json_object_object_add(decodeTlvJson, "unknown", arrayJson);
        }
    }

    return (decodeTlvJson);
}


// retreive info for one given card
STATIC  json_object* alsaCardProbe (const char *rqtSndId) {
    const char  *info, *name;
    const char *devid, *driver;
    json_object *ctlDev;
    snd_ctl_t   *handle;
    snd_ctl_card_info_t *cardinfo;
    int err;

    if ((err = snd_ctl_open(&handle, rqtSndId, 0)) < 0) {
        INFO (afbIface, "SndCard [%s] Not Found", rqtSndId);
        return NULL;
    }

    snd_ctl_card_info_alloca(&cardinfo);
    if ((err = snd_ctl_card_info(handle, cardinfo)) < 0) {
        snd_ctl_close(handle);
        WARNING (afbIface, "SndCard [%s] info error: %s", rqtSndId, snd_strerror(err));
        return NULL;
    }

    // start a new json object to store card info
    ctlDev = json_object_new_object();

    devid= snd_ctl_card_info_get_id(cardinfo);
    json_object_object_add (ctlDev, "devid"  , json_object_new_string(devid));
    name =  snd_ctl_card_info_get_name(cardinfo);
    json_object_object_add (ctlDev, "name", json_object_new_string (name));

    if (afbIface->verbosity > 1) {
        json_object_object_add (ctlDev, "devid", json_object_new_string(rqtSndId));
        driver= snd_ctl_card_info_get_driver(cardinfo);
        json_object_object_add (ctlDev, "driver"  , json_object_new_string(driver));
        info  = strdup(snd_ctl_card_info_get_longname (cardinfo));
        json_object_object_add (ctlDev, "info" , json_object_new_string (info));
        INFO (afbIface, "AJG: Soundcard Devid=%-5s devid=%-7s Name=%s\n", rqtSndId, devid, info);
    }

    // free card handle and return info
    snd_ctl_close(handle);    
    return (ctlDev);
}

// Loop on every potential Sound card and register active one
PUBLIC void alsaGetInfo (struct afb_req request) {
    int  card;
    json_object *ctlDev, *ctlDevs;
    char devid[32];
    
    const char *rqtSndId = afb_req_value(request, "devid");

    // if no specific card requested loop on all

    if (rqtSndId != NULL) {
        // only one card was requested let's probe it
        ctlDev = alsaCardProbe (rqtSndId);
        if (ctlDev != NULL) afb_req_success(request, ctlDev, NULL);
        else afb_req_fail_f (request, "sndscard-notfound", "SndCard [%s] Not Found", rqtSndId);
        
    } else {
        // return an array of ctlDev
        ctlDevs =json_object_new_array();

        // loop on potential card number
        for (card =0; card < MAX_SND_CARD; card++) {

            // build card devid and probe it
            snprintf (devid, sizeof(devid), "hw:%i", card);
            ctlDev = alsaCardProbe (devid);

            // Alsa has hole within card list [ignore them]
            if (ctlDev != NULL) {
                // add current ctlDev to ctlDevs object
                json_object_array_add (ctlDevs, ctlDev);
            }    
        }
        afb_req_success (request, ctlDevs, NULL);  
    }
}

// pack Alsa element's ACL into a JSON object
STATIC json_object *getControlAcl (snd_ctl_elem_info_t *info) {

    json_object * jsonAclCtl = json_object_new_object();

    json_object_object_add (jsonAclCtl, "read"   , json_object_new_boolean(snd_ctl_elem_info_is_readable(info)));
    json_object_object_add (jsonAclCtl, "write"  , json_object_new_boolean(snd_ctl_elem_info_is_writable(info)));
    json_object_object_add (jsonAclCtl, "inact"  , json_object_new_boolean(snd_ctl_elem_info_is_inactive(info)));
    json_object_object_add (jsonAclCtl, "volat"  , json_object_new_boolean(snd_ctl_elem_info_is_volatile(info)));
    json_object_object_add (jsonAclCtl, "lock"   , json_object_new_boolean(snd_ctl_elem_info_is_locked(info)));

    // if TLV is readable we insert its ACL
    if (!snd_ctl_elem_info_is_tlv_readable(info)) {
        json_object * jsonTlv = json_object_new_object();

        json_object_object_add (jsonTlv, "read"   , json_object_new_boolean(snd_ctl_elem_info_is_tlv_readable(info)));
        json_object_object_add (jsonTlv, "write"  , json_object_new_boolean(snd_ctl_elem_info_is_tlv_writable(info)));
        json_object_object_add (jsonTlv, "command", json_object_new_boolean(snd_ctl_elem_info_is_tlv_commandable(info)));

        json_object_object_add (jsonAclCtl, "tlv", jsonTlv);
    }
    return (jsonAclCtl);
}

// process ALSA control and store resulting value into ctlRequest
STATIC int alsaSetSingleCtl (snd_ctl_t *ctlDev, snd_ctl_elem_id_t *elemId, ctlRequestT *ctlRequest) {
    snd_ctl_elem_value_t *elemData;
    snd_ctl_elem_info_t  *elemInfo;
    int count, length, err, valueIsArray;

    // let's make sure we are processing the right control    
    if (ctlRequest->numId != snd_ctl_elem_id_get_numid (elemId)) goto OnErrorExit;
         
    // set info event ID and get value
    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_info_set_id(elemInfo, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_info(ctlDev, elemInfo) < 0) {
        NOTICE (afbIface, "Fail to load ALSA NUMID=%d Values=[%s]", ctlRequest->numId, json_object_to_json_string(ctlRequest->jValues));
        goto OnErrorExit;
    }
    
    snd_ctl_elem_info_get_id(elemInfo, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (!snd_ctl_elem_info_is_writable(elemInfo)) {
        NOTICE (afbIface, "Not Writable ALSA NUMID=%d Values=[%s]", ctlRequest->numId, json_object_to_json_string(ctlRequest->jValues));
        goto OnErrorExit;
    }
    
    count = snd_ctl_elem_info_get_count (elemInfo); 
    if (count == 0) goto OnErrorExit;
    
    enum json_type jtype= json_object_get_type(ctlRequest->jValues);
    switch (jtype) {
        case json_type_array:
            length = json_object_array_length (ctlRequest->jValues);
            valueIsArray=1;           
            break;
        case json_type_int:
            length=1;
            valueIsArray=0;
            break;                        
        default:
            count =0;
            break;
    }


    if (count == 0 || count < length) {
        NOTICE (afbIface, "Invalid values NUMID='%d' Values='%s' count='%d' wanted='%d'", ctlRequest->numId, json_object_to_json_string(ctlRequest->jValues), length, count);
        goto OnErrorExit;
    }

    snd_ctl_elem_value_alloca(&elemData);    
    snd_ctl_elem_value_set_id(elemData, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_read(ctlDev, elemData) < 0) goto OnErrorExit;   

    // Loop on every control value and push to sndcard
    for (int index=0; index < count && index < length; index++) {
        json_object *element;
        int value;
        
        if (valueIsArray) element= json_object_array_get_idx(ctlRequest->jValues, index);
        else element= ctlRequest->jValues;
        
        value= json_object_get_int (element);
        snd_ctl_elem_value_set_integer(elemData, index, value);
    }
    
    err = snd_ctl_elem_write(ctlDev, elemData);
    if (err < 0) {
        NOTICE (afbIface, "Fail to write ALSA NUMID=%d Values=[%s] Error=%s", ctlRequest->numId, json_object_to_json_string(ctlRequest->jValues), snd_strerror(err));
        goto OnErrorExit;
    }
    
    ctlRequest->used=1;
    return 0;
   
 OnErrorExit:
    ctlRequest->used=-1;
    return -1;   
}

// process ALSA control and store then into ctlRequest
STATIC int alsaGetSingleCtl (snd_ctl_t *ctlDev, snd_ctl_elem_id_t *elemId, ctlRequestT *ctlRequest, int quiet) {
    snd_ctl_elem_type_t  elemType;
    snd_ctl_elem_value_t *elemData;
    snd_ctl_elem_info_t  *elemInfo;
    int count, idx, err;

         
    // set info event ID and get value

    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_info_set_id(elemInfo, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_info(ctlDev, elemInfo) < 0) goto OnErrorExit;
    count = snd_ctl_elem_info_get_count (elemInfo); 
    if (count == 0) goto OnErrorExit;
    
    snd_ctl_elem_info_get_id(elemInfo, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (!snd_ctl_elem_info_is_readable(elemInfo)) goto OnErrorExit;
    elemType = snd_ctl_elem_info_get_type(elemInfo);    

    snd_ctl_elem_value_alloca(&elemData);    
    snd_ctl_elem_value_set_id(elemData, elemId);  // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_read(ctlDev, elemData) < 0) goto OnErrorExit;
    
    int numid= snd_ctl_elem_info_get_numid(elemInfo);

    ctlRequest->jValues= json_object_new_object();
    json_object_object_add (ctlRequest->jValues,"id" , json_object_new_int(numid));
    if (quiet < 2) json_object_object_add (ctlRequest->jValues,"name" , json_object_new_string(snd_ctl_elem_id_get_name (elemId)));
    if (quiet < 1) json_object_object_add (ctlRequest->jValues,"iface" , json_object_new_string(snd_ctl_elem_iface_name(snd_ctl_elem_id_get_interface(elemId))));
    if (quiet < 3) json_object_object_add (ctlRequest->jValues,"actif", json_object_new_boolean(!snd_ctl_elem_info_is_inactive(elemInfo)));

    json_object *jsonValuesCtl = json_object_new_array();
    for (idx = 0; idx < count; idx++) { // start from one in amixer.c !!!
        switch (elemType) {
        case SND_CTL_ELEM_TYPE_BOOLEAN: {
                json_object_array_add (jsonValuesCtl, json_object_new_boolean (snd_ctl_elem_value_get_boolean(elemData, idx)));
                break;
                }
        case SND_CTL_ELEM_TYPE_INTEGER:
                json_object_array_add (jsonValuesCtl, json_object_new_int ((int)snd_ctl_elem_value_get_integer(elemData, idx)));
                break;
        case SND_CTL_ELEM_TYPE_INTEGER64:
                json_object_array_add (jsonValuesCtl, json_object_new_int64 (snd_ctl_elem_value_get_integer64(elemData, idx)));
                break;
        case SND_CTL_ELEM_TYPE_ENUMERATED:
                json_object_array_add (jsonValuesCtl, json_object_new_int (snd_ctl_elem_value_get_enumerated(elemData, idx)));
                break;
        case SND_CTL_ELEM_TYPE_BYTES:
                json_object_array_add (jsonValuesCtl, json_object_new_int ((int)snd_ctl_elem_value_get_byte(elemData, idx)));
                break;
        case SND_CTL_ELEM_TYPE_IEC958: {
                json_object *jsonIec958Ctl = json_object_new_object();
                snd_aes_iec958_t iec958;
                snd_ctl_elem_value_get_iec958(elemData, &iec958);

                json_object_object_add (jsonIec958Ctl,"AES0",json_object_new_int(iec958.status[0]));
                json_object_object_add (jsonIec958Ctl,"AES1",json_object_new_int(iec958.status[1]));
                json_object_object_add (jsonIec958Ctl,"AES2",json_object_new_int(iec958.status[2]));
                json_object_object_add (jsonIec958Ctl,"AES3",json_object_new_int(iec958.status[3]));
                json_object_array_add  (jsonValuesCtl, jsonIec958Ctl);
                break;
                }
        default:
                json_object_array_add (jsonValuesCtl, json_object_new_string ("?unknown?"));
                break;
        }
    }
    json_object_object_add (ctlRequest->jValues,"val",jsonValuesCtl);

    if (!quiet) {  // in simple mode do not print usable values
        json_object *jsonClassCtl = json_object_new_object();
        json_object_object_add (jsonClassCtl,"type" , json_object_new_string(snd_ctl_elem_type_name(elemType)));
		json_object_object_add (jsonClassCtl,"count", json_object_new_int(count));

		switch (elemType) {
			case SND_CTL_ELEM_TYPE_INTEGER:
				json_object_object_add (jsonClassCtl,"min",  json_object_new_int((int)snd_ctl_elem_info_get_min(elemInfo)));
				json_object_object_add (jsonClassCtl,"max",  json_object_new_int((int)snd_ctl_elem_info_get_max(elemInfo)));
				json_object_object_add (jsonClassCtl,"step", json_object_new_int((int)snd_ctl_elem_info_get_step(elemInfo)));
				break;
			case SND_CTL_ELEM_TYPE_INTEGER64:
				json_object_object_add (jsonClassCtl,"min",  json_object_new_int64(snd_ctl_elem_info_get_min64(elemInfo)));
				json_object_object_add (jsonClassCtl,"max",  json_object_new_int64(snd_ctl_elem_info_get_max64(elemInfo)));
				json_object_object_add (jsonClassCtl,"step", json_object_new_int64(snd_ctl_elem_info_get_step64(elemInfo)));
				break;
			case SND_CTL_ELEM_TYPE_ENUMERATED: {
				unsigned int item, items = snd_ctl_elem_info_get_items(elemInfo);
				json_object *jsonEnum = json_object_new_array();

				for (item = 0; item < items; item++) {
					snd_ctl_elem_info_set_item(elemInfo, item);
					if ((err = snd_ctl_elem_info(ctlDev, elemInfo)) >= 0) {
						json_object_array_add (jsonEnum, json_object_new_string(snd_ctl_elem_info_get_item_name(elemInfo)));
					}
				}
				json_object_object_add (jsonClassCtl, "enums",jsonEnum);
				break;
			}
			default: break; // ignore any unknown type
			}

		// add collected class info with associated ACLs
		json_object_object_add (ctlRequest->jValues,"ctrl", jsonClassCtl);
		json_object_object_add (ctlRequest->jValues,"acl"  , getControlAcl (elemInfo));

		// check for tlv [direct port from amixer.c]
		if (snd_ctl_elem_info_is_tlv_readable(elemInfo)) {
				unsigned int *tlv;
				tlv = malloc(4096);
				if ((err = snd_ctl_elem_info(ctlDev, elemInfo)) < 0) {
					fprintf (stderr, "Control %s element TLV read error\n", snd_strerror(err));
					free(tlv);
				} else {
					json_object_object_add (ctlRequest->jValues,"tlv", decodeTlv (tlv, 4096));
		   }
		}
    }
    ctlRequest->used=1;
    return 0;
   
 OnErrorExit:
    ctlRequest->used=-1;
    return -1;   
}

// assign multiple control to the same value
PUBLIC void alsaSetGetCtls (struct afb_req request, ActionSetGetT action) {
    ctlRequestT *ctlRequest;
    const char *warmsg=NULL;
    int err=0, status=0;
    unsigned int ctlCount;
    snd_ctl_t *ctlDev;
    snd_ctl_elem_list_t *ctlList;  
    json_object *sndctls=json_object_new_array();;
    queryValuesT queryValues;
       
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;
    

    if ((err = snd_ctl_open(&ctlDev, queryValues.devid, 0)) < 0) {
        afb_req_fail_f (request, "sndcrl-notfound","devid=[%s] load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    snd_ctl_elem_list_alloca(&ctlList); 
    if ((err = snd_ctl_elem_list (ctlDev, ctlList)) < 0) {
        afb_req_fail_f (request, "listInit-failed","devid=[%s] load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    if ((err = snd_ctl_elem_list_alloc_space(ctlList, snd_ctl_elem_list_get_count(ctlList))) < 0) {
        afb_req_fail_f (request, "listAlloc-failed","devid=[%s] load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    if ((err = snd_ctl_elem_list (ctlDev, ctlList)) < 0) {
        afb_req_fail_f (request, "listOpen-failed","devid=[%s] load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    // Parse numids string (empty == all)
    ctlCount= snd_ctl_elem_list_get_used(ctlList);
    if (queryValues.count==0) {
        ctlRequest= alloca (sizeof(ctlRequestT)*(ctlCount));
    } else {
        ctlRequest= alloca (sizeof(ctlRequestT)*(queryValues.count));
        NumidsListParse (&queryValues, ctlRequest);
    }
          
    // Loop on all ctlDev controls
    for (int ctlIndex=0; ctlIndex < ctlCount; ctlIndex++) {
        unsigned int selected=0;
        int jdx;
        
        if (queryValues.count == 0 && action == ACTION_GET) {
                selected=1; // check is this numid is selected within query        
                jdx = ctlIndex;  // map all existing ctl as requested
        } else {
            int numid = snd_ctl_elem_list_get_numid(ctlList, ctlIndex); 
            if (numid < 0) {
                NOTICE (afbIface,"snd_ctl_elem_list_get_numid index=%d fail", ctlIndex);
                continue;
            }
            // check if current control was requested in query numids list
            for (jdx=0; jdx < queryValues.count; jdx++) {
                if (numid == ctlRequest[jdx].numId) {
                    selected = 1;
                    break;
                }
            }            
        }
        
        // control is selected open ctlid and get value 
        if (selected) {
            snd_ctl_elem_id_t  *elemId;
            snd_ctl_elem_id_alloca(&elemId);
           
            snd_ctl_elem_list_get_id (ctlList, ctlIndex, elemId);
            switch (action) {
                case ACTION_GET:
                    err = alsaGetSingleCtl (ctlDev, elemId, &ctlRequest[jdx], queryValues.quiet);
                break;
                
                case ACTION_SET:
                    err = alsaSetSingleCtl (ctlDev, elemId, &ctlRequest[jdx]);
                    
                default:
                    err = 1;
            }
            if (err) status++;
            else {
                json_object_array_add (sndctls, ctlRequest[jdx].jValues);
            }
        }
    }
  
    // if we had error let's add them into response message info
    json_object *warnings = json_object_new_array();
    for (int jdx=0; jdx < queryValues.count; jdx++) {
        if (ctlRequest[jdx].used <= 0) {
            json_object *failctl = json_object_new_object();
            json_object_object_add (failctl, "numid", ctlRequest[jdx].jToken);
            if (ctlRequest[jdx].jValues) 
                json_object_object_add(failctl, "values", ctlRequest[jdx].jValues);

            if (ctlRequest[jdx].numId == -1) json_object_object_add (failctl, "info", json_object_new_string ("Invalid NumID"));
            else {
               if (ctlRequest[jdx].used == 0) json_object_object_add (failctl, "info", json_object_new_string ("Does Not Exist"));
               if (ctlRequest[jdx].used == -1) json_object_object_add (failctl, "info", json_object_new_string ("Invalid Value"));
            }
            json_object_array_add (warnings, failctl);
        }
        /* WARNING!!!! Check with Jose why following put free jValues
        if (ctlRequest[jdx].jToken) json_object_put(ctlRequest[jdx].jToken);
        if (ctlRequest[jdx].jValues) json_object_put(ctlRequest[jdx].jValues);
        */
    }
    
    if (json_object_array_length(warnings)) warmsg=json_object_to_json_string_ext(warnings, JSON_C_TO_STRING_PLAIN);
    else json_object_put(warnings);
  
    // send response+warning if any
    afb_req_success (request, sndctls, warmsg);
    snd_ctl_elem_list_clear(ctlList);
    
    OnErrorExit:
        return;        
}

PUBLIC void alsaGetCtls (struct afb_req request) {
    alsaSetGetCtls (request, ACTION_GET);
}
    
PUBLIC void alsaSetCtls (struct afb_req request) {
    alsaSetGetCtls (request, ACTION_SET);
}


// This routine is called when ALSA event are fired
STATIC  int sndCtlEventCB (sd_event_source* src, int fd, uint32_t revents, void* userData) {
    int err;
    evtHandleT *evtHandle = (evtHandleT*)userData; 
    snd_ctl_event_t *eventId;
    json_object *ctlEventJ;
    unsigned int mask;
    int iface;
    int device;
    int subdev;
    const char*devname;
    ctlRequestT ctlRequest;
    snd_ctl_elem_id_t *elemId;
    
    if ((revents & EPOLLHUP) != 0) {
        NOTICE (afbIface, "SndCtl hanghup [car disconnected]");
        goto ExitOnSucess;
    }
    
    if ((revents & EPOLLIN) != 0)  {
        
        // initialise event structure on stack  
        snd_ctl_event_alloca(&eventId); 
        snd_ctl_elem_id_alloca(&elemId);
        
        err = snd_ctl_read(evtHandle->ctlDev, eventId);
        if (err < 0) goto OnErrorExit;
        
        // we only process sndctrl element
        if (snd_ctl_event_get_type(eventId) != SND_CTL_EVENT_ELEM) goto ExitOnSucess;
       
        // we only process value changed events
        mask = snd_ctl_event_elem_get_mask(eventId);
        if (!(mask & SND_CTL_EVENT_MASK_VALUE)) goto ExitOnSucess;
        
        snd_ctl_event_elem_get_id (eventId, elemId);
        
        err = alsaGetSingleCtl (evtHandle->ctlDev, elemId, &ctlRequest, evtHandle->quiet);
        if (err) goto OnErrorExit;

        iface  = snd_ctl_event_elem_get_interface(eventId);
        device = snd_ctl_event_elem_get_device(eventId);
        subdev = snd_ctl_event_elem_get_subdevice(eventId);
        devname= snd_ctl_event_elem_get_name(eventId);
        
        // proxy ctlevent as a binder event        
        ctlEventJ = json_object_new_object();
        json_object_object_add(ctlEventJ, "device" ,json_object_new_int (device));
        json_object_object_add(ctlEventJ, "subdev" ,json_object_new_int (subdev));
        if (evtHandle->quiet < 2) {
            json_object_object_add(ctlEventJ, "iface"  ,json_object_new_int (iface));
            json_object_object_add(ctlEventJ, "devname",json_object_new_string (devname));
        }
        if (ctlRequest.jValues) (json_object_object_add(ctlEventJ, "values"  ,ctlRequest.jValues));
        DEBUG(afbIface, "sndCtlEventCB=%s", json_object_get_string(ctlEventJ));
        afb_event_push(evtHandle->afbevt, ctlEventJ);

    }

    ExitOnSucess:
        return 0;
    
    OnErrorExit:
        WARNING (afbIface, "sndCtlEventCB: ignored unsupported event type");
	return (0);    
}

// Subscribe to every Alsa CtlEvent send by a given board
PUBLIC void alsaSubcribe (struct afb_req request) {
    static sndHandleT sndHandles[MAX_SND_CARD];
    evtHandleT *evtHandle;
    snd_ctl_t  *ctlDev;
    int err, idx, cardId, idxFree=-1;
    snd_ctl_card_info_t *cardinfo;
    queryValuesT queryValues;

    
    err = alsaCheckQuery (request, &queryValues);
    if (err) goto OnErrorExit;


    // open control interface for devid
    err = snd_ctl_open(&ctlDev, queryValues.devid, SND_CTL_READONLY);
    if (err < 0) {
        ctlDev=NULL;
        afb_req_fail_f (request, "devid-unknown", "SndCard devid=%s Not Found err=%s", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    snd_ctl_card_info_alloca(&cardinfo);    
    if ((err = snd_ctl_card_info(ctlDev, cardinfo)) < 0) {        
        afb_req_fail_f (request, "devid-invalid", "SndCard devid=%s Not Found err=%s", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }
    
    cardId=snd_ctl_card_info_get_card(cardinfo);
    
    // search for an existing subscription and mark 1st free slot
    for (idx= 0; idx < MAX_SND_CARD; idx ++) {
      if (sndHandles[idx].ucount > 0 && cardId == sndHandles[idx].cardId) {
         evtHandle= sndHandles[idx].evtHandle;
         break;
      } else if (idxFree == -1) idxFree= idx; 
    };
      
    // if not subscription exist for the event let's create one
    if (idx == MAX_SND_CARD) {
        
        // reach MAX_SND_CARD event registration
        if (idxFree == -1) {
            afb_req_fail_f (request, "register-toomany", "Cannot register new event Maxcard==%d", idx);
            goto OnErrorExit;            
        } 
        
        evtHandle = malloc (sizeof(evtHandleT));
        evtHandle->ctlDev = ctlDev;
        evtHandle->quiet  = queryValues.quiet;
        sndHandles[idxFree].ucount = 0;
        sndHandles[idxFree].cardId = cardId;
        sndHandles[idxFree].evtHandle = evtHandle;
        
        // subscribe for sndctl events attached to devid
        err = snd_ctl_subscribe_events(evtHandle->ctlDev, 1);
        if (err < 0) {
            afb_req_fail_f (request, "subscribe-fail", "Cannot subscribe events from devid=%s err=%d", queryValues.devid, err);
            goto OnErrorExit;
        }
            
        // get pollfd attach to this sound board
        snd_ctl_poll_descriptors(evtHandle->ctlDev, &evtHandle->pfds, 1);

        // register sound event to binder main loop
        err = sd_event_add_io(afb_daemon_get_event_loop(afbIface->daemon), &evtHandle->src, evtHandle->pfds.fd, EPOLLIN, sndCtlEventCB, evtHandle);
        if (err < 0) {
            afb_req_fail_f (request, "register-mainloop", "Cannot hook events to mainloop devid=%s err=%d", queryValues.devid, err);
            goto OnErrorExit;
        }

        // create binder event attached to devid name
        evtHandle->afbevt = afb_daemon_make_event (afbIface->daemon, queryValues.devid);
        if (!afb_event_is_valid (evtHandle->afbevt)) {
            afb_req_fail_f (request, "register-event", "Cannot register new binder event name=%s", queryValues.devid);
            goto OnErrorExit; 
        }

        // everything looks OK let's move forward 
        idx=idxFree;
    }
    
    // subscribe to binder event    
    err = afb_req_subscribe(request, evtHandle->afbevt);
    if (err != 0) {
        afb_req_fail_f (request, "register-eventname", "Cannot subscribe binder event name=%s [invalid channel]", queryValues.devid);
        goto OnErrorExit;
    }

    // increase usage count and return success
    sndHandles[idx].ucount ++;
    afb_req_success(request, NULL, NULL);
    return;
    
  OnErrorExit:
        if (ctlDev) snd_ctl_close(ctlDev);            
        return;
}

// Subscribe to every Alsa CtlEvent send by a given board
PUBLIC void alsaGetCardId (struct afb_req request) {
    char devid [10];
    const char *devname, *shortname, *longname;
    int card, err, index, idx;
    json_object *respJson;
    snd_ctl_t   *ctlDev;
    snd_ctl_card_info_t *cardinfo;
    
    const char *sndname = afb_req_value(request, "sndname");
    if (sndname == NULL) {
        afb_req_fail_f (request, "argument-missing", "sndname=SndCardName missing");
        goto OnErrorExit;
    }
    
    // loop on potential card number
    snd_ctl_card_info_alloca(&cardinfo);  
    for (card =0; card < MAX_SND_CARD; card++) {

        // build card devid and probe it
        snprintf (devid, sizeof(devid), "hw:%i", card);
        
        // open control interface for devid
        err = snd_ctl_open(&ctlDev, devid, SND_CTL_READONLY);
        if (err < 0) continue;   
        
        // extract sound card information
        snd_ctl_card_info(ctlDev, cardinfo);
        index    = snd_ctl_card_info_get_card(cardinfo);
        devname  = snd_ctl_card_info_get_id(cardinfo);
        shortname= snd_ctl_card_info_get_name(cardinfo);
        longname = snd_ctl_card_info_get_longname(cardinfo);
        
        // check if short|long name match        
        if (!strcmp (sndname, devname)) break;
        if (!strcmp (sndname, shortname)) break;
        if (!strcmp (sndname, longname)) break;
    }
    
    if (card == MAX_SND_CARD) {
        afb_req_fail_f (request, "ctlDev-notfound", "Fail to find card with name=%s", sndname);
        goto OnErrorExit;
    }
    
    // proxy ctlevent as a binder event        
    respJson = json_object_new_object();
    json_object_object_add(respJson, "index"     ,json_object_new_int (index));
    json_object_object_add(respJson, "devid"     ,json_object_new_string (devid));
    json_object_object_add(respJson, "shortname" ,json_object_new_string (shortname));
    json_object_object_add(respJson, "longname"  ,json_object_new_string (longname));
        
    // search for a HAL binder card mapping name to api prefix
    for (idx=0; idx < MAX_SND_CARD; idx++) {
       if (!strcmp (cardRegistry[idx]->shortname, shortname)) break;
    }
    // if a match if found, then we have an HAL for this board let's return its value
    if (idx < MAX_SND_CARD) json_object_object_add(respJson, "halapi",json_object_new_string (cardRegistry[idx]->apiprefix));
    
    afb_req_success(request, respJson, NULL);    
    return;
    
  OnErrorExit:
        return;  
}

// Register loaded HAL with board Name and API prefix
PUBLIC void alsaRegisterHal (struct afb_req request) {
    static int index=0;
    const char *shortname, *apiPrefix;
    
    apiPrefix = afb_req_value(request, "prefix");
    if (apiPrefix == NULL) {
        afb_req_fail_f (request, "argument-missing", "prefix=BindingApiPrefix missing");
        goto OnErrorExit;
    }
    
    shortname = afb_req_value(request, "name");
    if (shortname == NULL) {
        afb_req_fail_f (request, "argument-missing", "sndname=SndCardName missing");
        goto OnErrorExit;
    }
    
    if (index == MAX_SND_CARD) {
        afb_req_fail_f (request, "alsahal-toomany", "Fail to register sndname=[%s]", shortname);
        goto OnErrorExit;        
    }
    
    cardRegistry[index]= malloc (sizeof(cardRegistry));
    cardRegistry[index]->apiprefix=strdup(apiPrefix);
    cardRegistry[index]->shortname=strdup(shortname);
    index++;cardRegistry[index]=NULL;
 
    // when OK nothing to return
    afb_req_success(request, NULL, NULL);    
    return;
    
  OnErrorExit:
        return;  
}

