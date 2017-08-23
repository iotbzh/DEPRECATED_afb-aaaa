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
   https://www.kernel.org/doc/html/v4.11/sound/index.html

 */

#define _GNU_SOURCE  // needed for vasprintf
#include "Alsa-ApiHat.h"

PUBLIC void NumidsListParse(ActionSetGetT action, queryValuesT *queryValues, ctlRequestT *ctlRequest) {
    int length;

    for (int idx = 0; idx < queryValues->count; idx++) {
        json_object *jId, *valuesJ;
        ctlRequest[idx].used = 0;
        ctlRequest[idx].valuesJ = NULL;

        // when only one NUMID is provided it might not be encapsulated in a JSON array
        if (json_type_array == json_object_get_type(queryValues->numidsJ)) ctlRequest[idx].jToken = json_object_array_get_idx(queryValues->numidsJ, idx);
        else ctlRequest[idx].jToken = queryValues->numidsJ;

        enum json_type jtype = json_object_get_type(ctlRequest[idx].jToken);
        switch (jtype) {

            case json_type_int:
                // if NUMID is not an array then it should be an integer numid with no value
                ctlRequest[idx].numId = json_object_get_int(ctlRequest[idx].jToken);

                // Special SET simple short numid form [numid, [VAL1...VALX]]
                if (action == ACTION_SET && queryValues->count == 2) {
                    ctlRequest[idx].valuesJ = json_object_array_get_idx(queryValues->numidsJ, 1);
                    queryValues->count = 1; //In this form count==2 , when only one numid is to set
                    idx++;
                    continue;
                } else
                    break;

            case json_type_array:
                // NUMID is an array 1st slot should be numid, optionally values may come after
                length = json_object_array_length(ctlRequest[idx].jToken);

                // numid must be in 1st slot of numid json array
                ctlRequest[idx].numId = json_object_get_int(json_object_array_get_idx(ctlRequest[idx].jToken, 0));
                if (action == ACTION_GET) continue;

                // In Write mode second value should be the value
                if (action == ACTION_SET && length == 2) {
                    ctlRequest[idx].valuesJ = json_object_array_get_idx(ctlRequest[idx].jToken, 1);
                    continue;
                }

                // no numid value
                ctlRequest[idx].used = -1;
                break;

            case json_type_object:
                // numid+values formated as {id:xxx, val:[aa,bb...,nn]}
                if (!json_object_object_get_ex(ctlRequest[idx].jToken, "id", &jId) || !json_object_object_get_ex(ctlRequest[idx].jToken, "val", &valuesJ)) {
                    AFB_NOTICE("Invalid Json=%s missing 'id'|'val'", json_object_get_string(ctlRequest[idx].jToken));
                    ctlRequest[idx].used = -1;
                } else {
                    ctlRequest[idx].numId = json_object_get_int(jId);
                    if (action == ACTION_SET) ctlRequest[idx].valuesJ = valuesJ;
                }
                break;

            default:
                ctlRequest[idx].used = -1;
        }
    }
}

STATIC json_object *DB2StringJsonOject(long dB) {
    char label [20];
    if (dB < 0) {
        snprintf(label, sizeof (label), "-%li.%02lidB", -dB / 100, -dB % 100);
    } else {
        snprintf(label, sizeof (label), "%li.%02lidB", dB / 100, dB % 100);
    }

    // json function takes care of string copy
    return (json_object_new_string(label));
}


// Direct port from amixer TLV decode routine. This code is too complex for me.
// I hopefully did not break it when porting it.

STATIC json_object *decodeTlv(unsigned int *tlv, unsigned int tlv_size, int mode) {
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
                embedJson = decodeTlv(tlv + idx, tlv[idx + 1] + 8, mode);
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
                    if (mode >= QUERY_VERBOSE) {
                        snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                        json_object_array_add(arrayJson, json_object_new_string(label));
                    } else {
                        json_object_array_add(arrayJson, json_object_new_int(tlv[idx++]));
                    }
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbscaleJson, "array", arrayJson);
            } else {
                if (mode >= QUERY_VERBOSE) {
                    json_object_object_add(dbscaleJson, "min", DB2StringJsonOject((int) tlv[2]));
                    json_object_object_add(dbscaleJson, "step", DB2StringJsonOject(tlv[3] & 0xffff));
                    json_object_object_add(dbscaleJson, "mute", DB2StringJsonOject((tlv[3] >> 16) & 1));
                } else {
                    json_object_object_add(dbscaleJson, "min", json_object_new_int((int) tlv[2]));
                    json_object_object_add(dbscaleJson, "step", json_object_new_int(tlv[3] & 0xffff));
                    json_object_object_add(dbscaleJson, "mute", json_object_new_int((tlv[3] >> 16) & 1));
                }
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
                    if (mode >= QUERY_VERBOSE) {
                        snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                        json_object_array_add(arrayJson, json_object_new_string(label));
                    } else {
                        json_object_array_add(arrayJson, json_object_new_int(tlv[idx++]));
                    }
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbLinearJson, "offset", arrayJson);
            } else {
                if (mode >= QUERY_VERBOSE) {
                    json_object_object_add(dbLinearJson, "min", DB2StringJsonOject((int) tlv[2]));
                    json_object_object_add(dbLinearJson, "max", DB2StringJsonOject((int) tlv[3]));
                } else {
                    json_object_object_add(dbLinearJson, "min", json_object_new_int((int) tlv[2]));
                    json_object_object_add(dbLinearJson, "max", json_object_new_int((int) tlv[3]));
                }
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
                    if (mode >= QUERY_VERBOSE) {
                        snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                        json_object_array_add(arrayJson, json_object_new_string(label));
                    } else {
                        json_object_array_add(arrayJson, json_object_new_int(tlv[idx++]));
                    }
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbRangeJson, "dbrange", arrayJson);
                break;
            }
            while (size > 0) {
                json_object * embedJson = json_object_new_object();
                json_object_object_add(embedJson, "rangemin", json_object_new_int(tlv[idx++]));
                json_object_object_add(embedJson, "rangemax", json_object_new_int(tlv[idx++]));
                embedJson = decodeTlv(tlv + idx, 4 * sizeof (unsigned int), mode);
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
                    if (mode >= QUERY_VERBOSE) {
                        snprintf(label, sizeof (label), "0x%08x,", tlv[idx++]);
                        json_object_array_add(arrayJson, json_object_new_string(label));
                    } else {
                        json_object_array_add(arrayJson, json_object_new_int(tlv[idx++]));
                    }
                    size -= (unsigned int) sizeof (unsigned int);
                }
                json_object_object_add(dbMinMaxJson, "array", arrayJson);

            } else {
                if (mode >= QUERY_VERBOSE) {
                    json_object_object_add(dbMinMaxJson, "min", DB2StringJsonOject((int) tlv[2]));
                    json_object_object_add(dbMinMaxJson, "max", DB2StringJsonOject((int) tlv[3]));
                } else {
                    json_object_object_add(dbMinMaxJson, "min", json_object_new_int((int) tlv[2]));
                    json_object_object_add(dbMinMaxJson, "max", json_object_new_int((int) tlv[3]));
                }
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

STATIC json_object* alsaCardProbe(const char *rqtSndId) {
    const char *info, *name;
    const char *devid, *driver;
    json_object *ctlDev;
    snd_ctl_t *handle;
    snd_ctl_card_info_t *cardinfo;
    int err;

    if ((err = snd_ctl_open(&handle, rqtSndId, 0)) < 0) {
        AFB_INFO("alsaCardProbe '%s' Not Found", rqtSndId);
        return NULL;
    }

    snd_ctl_card_info_alloca(&cardinfo);
    if ((err = snd_ctl_card_info(handle, cardinfo)) < 0) {
        snd_ctl_close(handle);
        AFB_WARNING("SndCard '%s' info error: %s", rqtSndId, snd_strerror(err));
        return NULL;
    }

    // start a new json object to store card info
    ctlDev = json_object_new_object();

    devid = snd_ctl_card_info_get_id(cardinfo);
    json_object_object_add(ctlDev, "devid", json_object_new_string(devid));
    name = snd_ctl_card_info_get_name(cardinfo);
    json_object_object_add(ctlDev, "name", json_object_new_string(name));

    if (AFB_GET_VERBOSITY > 1) {
        json_object_object_add(ctlDev, "devid", json_object_new_string(rqtSndId));
        driver = snd_ctl_card_info_get_driver(cardinfo);
        json_object_object_add(ctlDev, "driver", json_object_new_string(driver));
        info = strdup(snd_ctl_card_info_get_longname(cardinfo));
        json_object_object_add(ctlDev, "info", json_object_new_string(info));
        AFB_INFO("AJG: Soundcard Devid=%-5s devid=%-7s Name=%s\n", rqtSndId, devid, info);
    }

    // free card handle and return info
    snd_ctl_close(handle);
    return (ctlDev);
}

// Loop on every potential Sound card and register active one

PUBLIC void alsaGetInfo(afb_req request) {
    int card;
    json_object *ctlDev, *ctlDevs;
    char devid[32];

    const char *rqtSndId = afb_req_value(request, "devid");

    // if no specific card requested loop on all

    if (rqtSndId != NULL) {
        // only one card was requested let's probe it
        ctlDev = alsaCardProbe(rqtSndId);
        if (ctlDev != NULL) afb_req_success(request, ctlDev, NULL);
        else afb_req_fail_f(request, "sndscard-notfound", "SndCard '%s' Not Found", rqtSndId);

    } else {
        // return an array of ctlDev
        ctlDevs = json_object_new_array();

        // loop on potential card number
        for (card = 0; card < MAX_SND_CARD; card++) {

            // build card devid and probe it
            snprintf(devid, sizeof (devid), "hw:%i", card);
            ctlDev = alsaCardProbe(devid);

            // Alsa has hole within card list [ignore them]
            if (ctlDev != NULL) {
                // add current ctlDev to ctlDevs object
                json_object_array_add(ctlDevs, ctlDev);
            }
        }
        afb_req_success(request, ctlDevs, NULL);
    }
}

// pack Alsa element's ACL into a JSON object

STATIC json_object *getControlAcl(snd_ctl_elem_info_t *info) {

    json_object * jsonAclCtl = json_object_new_object();

    json_object_object_add(jsonAclCtl, "read", json_object_new_boolean(snd_ctl_elem_info_is_readable(info)));
    json_object_object_add(jsonAclCtl, "write", json_object_new_boolean(snd_ctl_elem_info_is_writable(info)));
    json_object_object_add(jsonAclCtl, "inact", json_object_new_boolean(snd_ctl_elem_info_is_inactive(info)));
    json_object_object_add(jsonAclCtl, "volat", json_object_new_boolean(snd_ctl_elem_info_is_volatile(info)));
    json_object_object_add(jsonAclCtl, "lock", json_object_new_boolean(snd_ctl_elem_info_is_locked(info)));

    // if TLV is readable we insert its ACL
    if (!snd_ctl_elem_info_is_tlv_readable(info)) {
        json_object * jsonTlv = json_object_new_object();

        json_object_object_add(jsonTlv, "read", json_object_new_boolean(snd_ctl_elem_info_is_tlv_readable(info)));
        json_object_object_add(jsonTlv, "write", json_object_new_boolean(snd_ctl_elem_info_is_tlv_writable(info)));
        json_object_object_add(jsonTlv, "command", json_object_new_boolean(snd_ctl_elem_info_is_tlv_commandable(info)));

        json_object_object_add(jsonAclCtl, "tlv", jsonTlv);
    }
    return (jsonAclCtl);
}

// process ALSA control and store resulting value into ctlRequest

PUBLIC int alsaSetSingleCtl(snd_ctl_t *ctlDev, snd_ctl_elem_id_t *elemId, ctlRequestT *ctlRequest) {
    snd_ctl_elem_value_t *elemData;
    snd_ctl_elem_info_t *elemInfo;
    int count, length, err, valueIsArray = 0;

    // let's make sure we are processing the right control
    if (ctlRequest->numId != snd_ctl_elem_id_get_numid(elemId)) goto OnErrorExit;

    // set info event ID and get value
    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_info_set_id(elemInfo, elemId); // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_info(ctlDev, elemInfo) < 0) {
        AFB_NOTICE("Fail to load ALSA NUMID=%d Values='%s'", ctlRequest->numId, json_object_get_string(ctlRequest->valuesJ));
        goto OnErrorExit;
    }

    if (!snd_ctl_elem_info_is_writable(elemInfo)) {
        AFB_NOTICE("Not Writable ALSA NUMID=%d Values='%s'", ctlRequest->numId, json_object_get_string(ctlRequest->valuesJ));
        goto OnErrorExit;
    }

    count = snd_ctl_elem_info_get_count(elemInfo);
    if (count == 0) goto OnErrorExit;

    enum json_type jtype = json_object_get_type(ctlRequest->valuesJ);
    switch (jtype) {
        case json_type_array:
            length = json_object_array_length(ctlRequest->valuesJ);
            valueIsArray = 1;
            break;
        case json_type_int:
            length = 1;
            valueIsArray = 0;
            break;
        default:
            length = 0;
            break;
    }


    if (length == 0) {
        AFB_NOTICE("Invalid values NUMID='%d' Values='%s' count='%d' wanted='%d'", ctlRequest->numId, json_object_get_string(ctlRequest->valuesJ), length, count);
        goto OnErrorExit;
    }

    snd_ctl_elem_value_alloca(&elemData);
    snd_ctl_elem_value_set_id(elemData, elemId); // map ctlInfo to ctlId elemInfo is updated !!!
    if (snd_ctl_elem_read(ctlDev, elemData) < 0) goto OnErrorExit;

    // Loop on every control value and push to sndcard
    for (int index = 0; index < count; index++) {
        json_object *element;
        int value;

        // when not enough value duplicate last provided one
        if (!valueIsArray) element = ctlRequest->valuesJ;
        else {
            if (index < length) element = json_object_array_get_idx(ctlRequest->valuesJ, index);
            else element = json_object_array_get_idx(ctlRequest->valuesJ, length - 1);
        }

        value = json_object_get_int(element);
        snd_ctl_elem_value_set_integer(elemData, index, value);
    }

    err = snd_ctl_elem_write(ctlDev, elemData);
    if (err < 0) {
        AFB_NOTICE("Fail to write ALSA NUMID=%d Values='%s' Error=%s", ctlRequest->numId, json_object_get_string(ctlRequest->valuesJ), snd_strerror(err));
        goto OnErrorExit;
    }

    ctlRequest->used = 1;
    return 0;

OnErrorExit:
    ctlRequest->used = -1;
    return -1;
}

// process ALSA control and store then into ctlRequest

PUBLIC int alsaGetSingleCtl(snd_ctl_t *ctlDev, snd_ctl_elem_id_t *elemId, ctlRequestT *ctlRequest, halQueryMode queryMode) {
    snd_ctl_elem_type_t elemType;
    snd_ctl_elem_value_t *elemData;
    snd_ctl_elem_info_t *elemInfo;
    int count, idx, err;


    // set info event ID and get value

    snd_ctl_elem_info_alloca(&elemInfo);
    snd_ctl_elem_info_set_id(elemInfo, elemId);
    if (snd_ctl_elem_info(ctlDev, elemInfo) < 0) goto OnErrorExit;
    count = snd_ctl_elem_info_get_count(elemInfo);
    if (count == 0) goto OnErrorExit;

    if (!snd_ctl_elem_info_is_readable(elemInfo)) goto OnErrorExit;
    elemType = snd_ctl_elem_info_get_type(elemInfo);

    snd_ctl_elem_value_alloca(&elemData);
    snd_ctl_elem_value_set_id(elemData, elemId);
    if (snd_ctl_elem_read(ctlDev, elemData) < 0) goto OnErrorExit;

    int numid = snd_ctl_elem_info_get_numid(elemInfo);

    ctlRequest->valuesJ = json_object_new_object();
    json_object_object_add(ctlRequest->valuesJ, "id", json_object_new_int(numid));
    if (queryMode >= 1) json_object_object_add(ctlRequest->valuesJ, "name", json_object_new_string(snd_ctl_elem_id_get_name(elemId)));
    if (queryMode >= 2) json_object_object_add(ctlRequest->valuesJ, "iface", json_object_new_string(snd_ctl_elem_iface_name(snd_ctl_elem_id_get_interface(elemId))));
    if (queryMode >= 3) json_object_object_add(ctlRequest->valuesJ, "actif", json_object_new_boolean(!snd_ctl_elem_info_is_inactive(elemInfo)));

    json_object *jsonValuesCtl = json_object_new_array();
    for (idx = 0; idx < count; idx++) { // start from one in amixer.c !!!
        switch (elemType) {
            case SND_CTL_ELEM_TYPE_BOOLEAN:
            {
                json_object_array_add(jsonValuesCtl, json_object_new_boolean(snd_ctl_elem_value_get_boolean(elemData, idx)));
                break;
            }
            case SND_CTL_ELEM_TYPE_INTEGER:
                json_object_array_add(jsonValuesCtl, json_object_new_int((int) snd_ctl_elem_value_get_integer(elemData, idx)));
                break;
            case SND_CTL_ELEM_TYPE_INTEGER64:
                json_object_array_add(jsonValuesCtl, json_object_new_int64(snd_ctl_elem_value_get_integer64(elemData, idx)));
                break;
            case SND_CTL_ELEM_TYPE_ENUMERATED:
                json_object_array_add(jsonValuesCtl, json_object_new_int(snd_ctl_elem_value_get_enumerated(elemData, idx)));
                break;
            case SND_CTL_ELEM_TYPE_BYTES:
                json_object_array_add(jsonValuesCtl, json_object_new_int((int) snd_ctl_elem_value_get_byte(elemData, idx)));
                break;
            case SND_CTL_ELEM_TYPE_IEC958:
            {
                json_object *jsonIec958Ctl = json_object_new_object();
                snd_aes_iec958_t iec958;
                snd_ctl_elem_value_get_iec958(elemData, &iec958);

                json_object_object_add(jsonIec958Ctl, "AES0", json_object_new_int(iec958.status[0]));
                json_object_object_add(jsonIec958Ctl, "AES1", json_object_new_int(iec958.status[1]));
                json_object_object_add(jsonIec958Ctl, "AES2", json_object_new_int(iec958.status[2]));
                json_object_object_add(jsonIec958Ctl, "AES3", json_object_new_int(iec958.status[3]));
                json_object_array_add(jsonValuesCtl, jsonIec958Ctl);
                break;
            }
            default:
                json_object_array_add(jsonValuesCtl, json_object_new_string("?unknown?"));
                break;
        }
    }
    json_object_object_add(ctlRequest->valuesJ, "val", jsonValuesCtl);

    if (queryMode >= 1) { // in simple mode do not print usable values
        json_object *jsonClassCtl = json_object_new_object();
        json_object_object_add(jsonClassCtl, "type", json_object_new_int(elemType));
        json_object_object_add(jsonClassCtl, "count", json_object_new_int(count));

        switch (elemType) {
            case SND_CTL_ELEM_TYPE_INTEGER:
                json_object_object_add(jsonClassCtl, "min", json_object_new_int((int) snd_ctl_elem_info_get_min(elemInfo)));
                json_object_object_add(jsonClassCtl, "max", json_object_new_int((int) snd_ctl_elem_info_get_max(elemInfo)));
                json_object_object_add(jsonClassCtl, "step", json_object_new_int((int) snd_ctl_elem_info_get_step(elemInfo)));
                break;
            case SND_CTL_ELEM_TYPE_INTEGER64:
                json_object_object_add(jsonClassCtl, "min", json_object_new_int64(snd_ctl_elem_info_get_min64(elemInfo)));
                json_object_object_add(jsonClassCtl, "max", json_object_new_int64(snd_ctl_elem_info_get_max64(elemInfo)));
                json_object_object_add(jsonClassCtl, "step", json_object_new_int64(snd_ctl_elem_info_get_step64(elemInfo)));
                break;
            case SND_CTL_ELEM_TYPE_ENUMERATED:
            {
                unsigned int item, items = snd_ctl_elem_info_get_items(elemInfo);
                json_object *jsonEnum = json_object_new_array();

                for (item = 0; item < items; item++) {
                    snd_ctl_elem_info_set_item(elemInfo, item);
                    if ((err = snd_ctl_elem_info(ctlDev, elemInfo)) >= 0) {
                        json_object_array_add(jsonEnum, json_object_new_string(snd_ctl_elem_info_get_item_name(elemInfo)));
                    }
                }
                json_object_object_add(jsonClassCtl, "enums", jsonEnum);
                break;
            }
            default: break; // ignore any unknown type
        }

        // add collected class info with associated ACLs
        json_object_object_add(ctlRequest->valuesJ, "ctl", jsonClassCtl);

        if (queryMode >= QUERY_FULL) json_object_object_add(ctlRequest->valuesJ, "acl", getControlAcl(elemInfo));

        // check for tlv [direct port from amixer.c]
        if (snd_ctl_elem_info_is_tlv_readable(elemInfo)) {
            unsigned int *tlv = alloca(TLV_BYTE_SIZE);
            if ((err = snd_ctl_elem_tlv_read(ctlDev, elemId, tlv, 4096)) < 0) {
                AFB_NOTICE("Control numid=%d err=%s element TLV read error\n", numid, snd_strerror(err));
                goto OnErrorExit;
            } else {
                json_object_object_add(ctlRequest->valuesJ, "tlv", decodeTlv(tlv, TLV_BYTE_SIZE, queryMode));
            }
        }
    }

    ctlRequest->used = 1;
    return 0;

OnErrorExit:
    ctlRequest->used = -1;
    return -1;
}

// assign multiple control to the same value

STATIC void alsaSetGetCtls(ActionSetGetT action, afb_req request) {
    ctlRequestT *ctlRequest;
    const char *warmsg = NULL;
    int err = 0, status = 0, done;
    unsigned int ctlCount;
    snd_ctl_t *ctlDev;
    snd_ctl_elem_list_t *ctlList;
    queryValuesT queryValues;
    json_object *queryJ, *numidsJ, *sndctls;

    queryJ = alsaCheckQuery(request, &queryValues);
    if (!queryJ) goto OnErrorExit;

    // Prase Numids + optional values
    done = json_object_object_get_ex(queryJ, "ctl", &numidsJ);
    if (!done) queryValues.count = 0;
    else {
        enum json_type jtype = json_object_get_type(numidsJ);
        switch (jtype) {
            case json_type_array:
                queryValues.numidsJ = numidsJ;
                queryValues.count = json_object_array_length(numidsJ);
                break;

            case json_type_int:
            case json_type_object:
                queryValues.count = 1;
                queryValues.numidsJ = numidsJ;
                break;

            default:
                afb_req_fail_f(request, "numid-notarray", "NumId=%s NumId not valid JSON array", json_object_get_string(numidsJ));
                goto OnErrorExit;
        }
    }

    if ((err = snd_ctl_open(&ctlDev, queryValues.devid, 0)) < 0) {
        afb_req_fail_f(request, "sndcrl-notfound", "devid='%s' load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    snd_ctl_elem_list_alloca(&ctlList);
    if ((err = snd_ctl_elem_list(ctlDev, ctlList)) < 0) {
        afb_req_fail_f(request, "listInit-failed", "devid='%s' load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    if ((err = snd_ctl_elem_list_alloc_space(ctlList, snd_ctl_elem_list_get_count(ctlList))) < 0) {
        afb_req_fail_f(request, "listAlloc-failed", "devid='%s' load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    if ((err = snd_ctl_elem_list(ctlDev, ctlList)) < 0) {
        afb_req_fail_f(request, "listOpen-failed", "devid='%s' load fail error=%s\n", queryValues.devid, snd_strerror(err));
        goto OnErrorExit;
    }

    // Parse numids string (empty == all)
    ctlCount = snd_ctl_elem_list_get_used(ctlList);
    if (queryValues.count == 0) {
        ctlRequest = alloca(sizeof (ctlRequestT)*(ctlCount));
    } else {
        ctlRequest = alloca(sizeof (ctlRequestT)*(queryValues.count));
        NumidsListParse(action, &queryValues, ctlRequest);
    }

    // if more than one crl requested prepare an array for response
    if (queryValues.count != 1 && action == ACTION_GET) sndctls = json_object_new_array();
    else sndctls = NULL;

    // Loop on all ctlDev controls
    for (int ctlIndex = 0; ctlIndex < ctlCount; ctlIndex++) {
        unsigned int selected = 0;
        int jdx;

        if (queryValues.count == 0 && action == ACTION_GET) {
            selected = 1; // check is this numid is selected within query
            jdx = ctlIndex; // map all existing ctl as requested
        } else {
            int numid = snd_ctl_elem_list_get_numid(ctlList, ctlIndex);
            if (numid < 0) {
                AFB_NOTICE("snd_ctl_elem_list_get_numid index=%d fail", ctlIndex);
                continue;
            }
            // check if current control was requested in query numids list
            for (jdx = 0; jdx < queryValues.count; jdx++) {
                if (numid == ctlRequest[jdx].numId) {
                    selected = 1;
                    break;
                }
            }
        }

        // control is selected open ctlid and get value
        if (selected) {
            snd_ctl_elem_id_t *elemId;
            snd_ctl_elem_id_alloca(&elemId);

            snd_ctl_elem_list_get_id(ctlList, ctlIndex, elemId);
            switch (action) {
                case ACTION_GET:
                    err = alsaGetSingleCtl(ctlDev, elemId, &ctlRequest[jdx], queryValues.mode);
                    break;

                case ACTION_SET:
                    err = alsaSetSingleCtl(ctlDev, elemId, &ctlRequest[jdx]);
                    break;

                default:
                    err = 1;
            }
            if (err) status++;
            else {
                // Do not embed response in an array when only one ctl was requested
                if (action == ACTION_GET) {
                    if (queryValues.count == 1) sndctls = ctlRequest[jdx].valuesJ;
                    else json_object_array_add(sndctls, ctlRequest[jdx].valuesJ);
                }
            }
        }
    }

    // if we had error let's add them into response message info
    json_object *warningsJ = json_object_new_array();
    for (int jdx = 0; jdx < queryValues.count; jdx++) {
        if (ctlRequest[jdx].used <= 0) {
            json_object *failctl = json_object_new_object();
            if (ctlRequest[jdx].numId == -1) json_object_object_add(failctl, "warning", json_object_new_string("Numid Invalid"));
            else {
                if (ctlRequest[jdx].used == 0) json_object_object_add(failctl, "warning", json_object_new_string("Numid Does Not Exist"));
                if (ctlRequest[jdx].used == -1) json_object_object_add(failctl, "warning", json_object_new_string("Value Refused"));
            }

            json_object_object_add(failctl, "ctl", ctlRequest[jdx].jToken);

            json_object_array_add(warningsJ, failctl);
        }
        /* WARNING!!!! Check with Jose why following put free valuesJ
        if (ctlRequest[jdx].jToken) json_object_put(ctlRequest[jdx].jToken);
        if (ctlRequest[jdx].valuesJ) json_object_put(ctlRequest[jdx].valuesJ);
         */
    }

    if (json_object_array_length(warningsJ) > 0) warmsg = json_object_get_string(warningsJ);
    else json_object_put(warningsJ);

    // send response+warning if any
    afb_req_success(request, sndctls, warmsg);
    snd_ctl_elem_list_clear(ctlList);

OnErrorExit:
    return;
}

PUBLIC void alsaGetCtls(afb_req request) {
    alsaSetGetCtls(ACTION_GET, request);
}

PUBLIC void alsaSetCtls(afb_req request) {
    alsaSetGetCtls(ACTION_SET, request);
}


