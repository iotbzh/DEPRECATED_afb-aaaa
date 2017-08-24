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
 *
 * References:
 *  https://kernel.readthedocs.io/en/sphinx-samples/writing-an-alsa-driver.html#control-names
 *  https://01.org/linuxgraphics/gfx-docs/drm/sound/designs/control-names.html

 */
#define _GNU_SOURCE  // needed for vasprintf

#include <alsa/asoundlib.h>
#include <alsa/sound/tlv.h>
#include <systemd/sd-event.h>
#include <sys/ioctl.h>

#include "Alsa-ApiHat.h"

// Performs like a toggle switch for attenuation, because they're bool (ref:user-ctl-element-set.c)

#ifndef SNDRV_CTL_TLVD_DECLARE_DB_MINMAX
// taken from alsa-lib-1.1.3:include/sound/tlv.h

#define SNDRV_CTL_TLVD_LENGTH(...) \
    ((unsigned int)sizeof((const unsigned int[]) { __VA_ARGS__ }))

#define SNDRV_CTL_TLVD_ITEM(type, ...) \
    (type), SNDRV_CTL_TLVD_LENGTH(__VA_ARGS__), __VA_ARGS__

#define SNDRV_CTL_TLVT_DB_MINMAX 4

#define SNDRV_CTL_TLVD_DB_MINMAX_ITEM(min_dB, max_dB) \
    SNDRV_CTL_TLVD_ITEM(SNDRV_CTL_TLVT_DB_MINMAX, (min_dB), (max_dB))

#define SNDRV_CTL_TLVD_DECLARE_DB_MINMAX(name, min_dB, max_dB) \
    unsigned int name[] = { \
        SNDRV_CTL_TLVD_DB_MINMAX_ITEM(min_dB, max_dB) \
    }

#define SNDRV_CTL_TLVD_DB_SCALE_MASK    0xffff
#define SNDRV_CTL_TLVD_DB_SCALE_MUTE    0x10000

#endif
static const unsigned int *allocate_bool_fake_tlv(void) {
    static const SNDRV_CTL_TLVD_DECLARE_DB_MINMAX(range, -10000, 0);
    unsigned int *tlv = malloc(sizeof (range));
    if (tlv == NULL) return NULL;
    memcpy(tlv, range, sizeof (range));
    return tlv;
}

static const unsigned int *allocate_int_dbscale_tlv(int min, int step, int mute) {
    // SNDRV_CTL_TLVD_DECLARE_DB_SCALE(range, min, step, mute);
    size_t tlvSize = sizeof (4 * sizeof (unsigned int));
    unsigned int *tlv = malloc(tlvSize);
    tlv[0] = SNDRV_CTL_TLVT_DB_LINEAR;
    tlv[1] = (int) tlvSize;
    tlv[2] = min * 100;
    tlv[3] = ((step * 100) & SNDRV_CTL_TLVD_DB_SCALE_MASK) | ((mute * 100) ? SNDRV_CTL_TLVD_DB_SCALE_MUTE : 0);
    return tlv;
}

static const unsigned int *allocate_int_linear_tlv(int max, int min) {
    // SNDRV_CTL_TLVD_DECLARE_DB_LINEAR (range, min, max);
    size_t tlvSize = sizeof (4 * sizeof (unsigned int));
    unsigned int *tlv = malloc(tlvSize);
    tlv[0] = SNDRV_CTL_TLVT_DB_LINEAR;
    tlv[1] = (int) tlvSize;
    tlv[2] = -min * 100;
    tlv[3] = max * 100;
    return tlv;
}

STATIC json_object * addOneSndCtl(afb_req request, snd_ctl_t *ctlDev, json_object *ctlJ, halQueryMode queryMode) {
    int err, done, ctlNumid, ctlValue=0, shouldCreate;
    json_object *tmpJ;
    const char *ctlName;
    ctlRequestT ctlRequest;
    int ctlMax=100, ctlMin=0, ctlStep, ctlCount, ctlSubDev=0, ctlSndDev=0;
    snd_ctl_elem_type_t ctlType=SND_CTL_ELEM_TYPE_NONE;
    snd_ctl_elem_info_t *elemInfo;
    snd_ctl_elem_id_t *elemId;
    snd_ctl_elem_value_t *elemValue;
    const unsigned int *elemTlv = NULL;

    // parse json ctl object
    json_object_object_get_ex(ctlJ, "name", &tmpJ);
    ctlName = json_object_get_string(tmpJ);

    json_object_object_get_ex(ctlJ, "ctl", &tmpJ);
    ctlNumid = json_object_get_int(tmpJ);

    if (!ctlNumid && !ctlName) {
        afb_req_fail_f(request, "ctl-invalid", "crl=%s name or numid missing", json_object_get_string(ctlJ));
        goto OnErrorExit;
    }


    // We are facing a software ctl
    if (ctlNumid == CTL_AUTO) {
        done = json_object_object_get_ex(ctlJ, "type", &tmpJ);
        if (done) ctlType = json_object_get_int(tmpJ);
        else ctlType = SND_CTL_ELEM_TYPE_INTEGER;

        json_object_object_get_ex(ctlJ, "val", &tmpJ);
        ctlValue = json_object_get_int(tmpJ);

        // default for json_object_get_int is zero
        json_object_object_get_ex(ctlJ, "min", &tmpJ);
        ctlMin = json_object_get_int(tmpJ);

        done = json_object_object_get_ex(ctlJ, "max", &tmpJ);
        if (done) ctlMax = json_object_get_int(tmpJ);
        else {
            if (ctlType == SND_CTL_ELEM_TYPE_BOOLEAN) ctlMax = 1;
            else ctlMax = 100;
        }

        done = json_object_object_get_ex(ctlJ, "step", &tmpJ);
        if (!done) ctlStep = 1;
        else ctlStep = json_object_get_int(tmpJ);

        done = json_object_object_get_ex(ctlJ, "count", &tmpJ);
        if (!done) ctlCount = 1;
        else ctlCount = json_object_get_int(tmpJ);

        json_object_object_get_ex(ctlJ, "snddev", &tmpJ);
        ctlSndDev = json_object_get_int(tmpJ);

        json_object_object_get_ex(ctlJ, "subdev", &tmpJ);
        ctlSubDev = json_object_get_int(tmpJ);
    }

    // Assert that this ctls is not used
    snd_ctl_elem_info_alloca(&elemInfo);
    if (ctlName) snd_ctl_elem_info_set_name(elemInfo, ctlName);
    if (ctlNumid > 0)snd_ctl_elem_info_set_numid(elemInfo, ctlNumid);
    snd_ctl_elem_info_set_interface(elemInfo, SND_CTL_ELEM_IFACE_MIXER);

    // If control exist try to reuse it
    err = snd_ctl_elem_info(ctlDev, elemInfo);
    if (err) {
        shouldCreate = 1;
    } else {
        int count, min, max, sndDev, subDev;
        snd_ctl_elem_type_t type;

        // ctl exit let's get associated elemID
        snd_ctl_elem_id_alloca(&elemId);
        snd_ctl_elem_info_get_id(elemInfo, elemId);

        // If this is a hardware ctl only update value
        if (ctlNumid != CTL_AUTO) {
            json_object_object_get_ex(ctlJ, "val", &tmpJ);
            ctlValue = json_object_get_int(tmpJ);
            goto UpdateDefaultVal;
        }

        count = snd_ctl_elem_info_get_count(elemInfo);
        min = (int) snd_ctl_elem_info_get_min(elemInfo);
        max = (int) snd_ctl_elem_info_get_max(elemInfo);
        type = snd_ctl_elem_info_get_type(elemInfo);
        sndDev = (int) snd_ctl_elem_info_get_device(elemInfo);
        subDev = (int) snd_ctl_elem_info_get_subdevice(elemInfo);

        if (count == ctlCount && min == ctlMin && max == ctlMax && type == ctlType && sndDev == ctlSndDev && subDev == ctlSubDev) {
            // Adjust value to best fit request
            shouldCreate = 0;
        } else {
            err = snd_ctl_elem_remove(ctlDev, elemId);
            shouldCreate = 1;
            if (err < 0) {
                AFB_ERROR("addOneSndCtl: ctlName=%s numid=%d fail to reset", snd_ctl_elem_info_get_name(elemInfo), snd_ctl_elem_info_get_numid(elemInfo));
                goto DoNotUpdate;
            };

            // Control was deleted need to refresh elemInfo
            snd_ctl_elem_info_alloca(&elemInfo);
            if (ctlName) snd_ctl_elem_info_set_name(elemInfo, ctlName);
            snd_ctl_elem_info_set_interface(elemInfo, SND_CTL_ELEM_IFACE_MIXER);
            snd_ctl_elem_info(ctlDev, elemInfo);
        }
    }

    // Add requested ID into elemInfo
    snd_ctl_elem_info_set_device(elemInfo, ctlSndDev);
    snd_ctl_elem_info_set_subdevice(elemInfo, ctlSubDev);

    switch (ctlType) {
        case SND_CTL_ELEM_TYPE_BOOLEAN:
            if (shouldCreate) {
                err = snd_ctl_add_boolean_elem_set(ctlDev, elemInfo, 1, ctlCount);
                if (err) {
                    AFB_ERROR("AddOntSndCtl Boolean: devid='%s' name='%s' count=%d", snd_ctl_name(ctlDev), ctlName, ctlCount);
                    afb_req_fail_f(request, "ctl-invalid-bool", "devid='%s' name='%s' count=%d", snd_ctl_name(ctlDev), ctlName, ctlCount);
                    goto OnErrorExit;
                }
            }

            // Create a dummy TLV
            elemTlv = allocate_bool_fake_tlv();

            break;

        case SND_CTL_ELEM_TYPE_INTEGER:
            if (shouldCreate) {
                err = snd_ctl_add_integer_elem_set(ctlDev, elemInfo, 1, ctlCount, ctlMin, ctlMax, ctlStep);
                if (err) {
                    AFB_ERROR("AddOntSndCtl Integer: devid='%s' name='%s' count=%d min=%d max=%d step=%d", snd_ctl_name(ctlDev), ctlName, ctlCount, ctlMin, ctlMax, ctlStep);
                    afb_req_fail_f(request, "ctl-invalid-bool", "devid='%s' name='%s' count=%d min=%d max=%d step=%d", snd_ctl_name(ctlDev), ctlName, ctlCount, ctlMin, ctlMax, ctlStep);
                    goto OnErrorExit;
                }
            }

            // Fulup needed to be tested with some dB expert !!!
            json_object *dbscaleJ;
            if (json_object_object_get_ex(ctlJ, "dbscale", &dbscaleJ)) {
                int min, max;

                if (json_object_get_type(dbscaleJ) != json_type_object) {
                    afb_req_fail_f(request, "ctl-invalid-dbscale", "devid=%s crl=%s invalid json in integer control", snd_ctl_name(ctlDev), json_object_get_string(ctlJ));
                    goto OnErrorExit;

                    json_object_object_get_ex(ctlJ, "min", &tmpJ);
                    min = json_object_get_int(tmpJ);
                    if (min >= 0) {
                        afb_req_fail_f(request, "ctl-invalid-dbscale", "devid=%s crl=%s min should be a negative number", snd_ctl_name(ctlDev), json_object_get_string(ctlJ));
                        goto OnErrorExit;
                    }

                    // default value 0=mute
                    json_object_object_get_ex(ctlJ, "max", &tmpJ);
                    max = json_object_get_int(tmpJ);

                    // default value 1dB
                    json_object_object_get_ex(ctlJ, "step", &tmpJ);
                    int step = json_object_get_int(tmpJ);
                    if (step <= 0) step = 1;

                    elemTlv = allocate_int_dbscale_tlv(min, max, step);

                }
            } else {
                // provide a fake linear TLV
                elemTlv = allocate_int_linear_tlv(ctlMin, ctlMax);
            }
            break;

        case SND_CTL_ELEM_TYPE_ENUMERATED:

            if (shouldCreate) {
                json_object *enumsJ;
                json_object_object_get_ex(ctlJ, "enums", &enumsJ);
                if (json_object_get_type(enumsJ) != json_type_array) {
                    afb_req_fail_f(request, "ctl-missing-enums", "devid=%s crl=%s mandatory enum=xxx missing in enumerated control", snd_ctl_name(ctlDev), json_object_get_string(ctlJ));
                    goto OnErrorExit;
                }

                int length = json_object_array_length(enumsJ);
                const char **enumlabels = malloc(length * sizeof (char*));

                for (int jdx = 0; jdx < length; jdx++) {
                    tmpJ = json_object_array_get_idx(enumsJ, jdx);
                    enumlabels[jdx] = json_object_get_string(tmpJ);
                }

                err = snd_ctl_add_enumerated_elem_set(ctlDev, elemInfo, 1, ctlCount, length, enumlabels);
                if (err) {
                    afb_req_fail_f(request, "ctl-invalid-bool", "devid=%s crl=%s invalid enumerated control", snd_ctl_name(ctlDev), json_object_get_string(ctlJ));
                    goto OnErrorExit;
                }

                // Fulup should be an empty container
                elemTlv = allocate_bool_fake_tlv();
            }
            break;

            // Long Term Waiting ToDoList
        case SND_CTL_ELEM_TYPE_INTEGER64:
        case SND_CTL_ELEM_TYPE_BYTES:
        default:
            afb_req_fail_f(request, "ctl-invalid-type", "crl=%s unsupported type type=%d numid=%d", json_object_get_string(ctlJ), ctlType, ctlNumid);
            goto OnErrorExit;
    }

UpdateDefaultVal:

    // Set Value to default
    snd_ctl_elem_value_alloca(&elemValue);
    for (int idx = 0; idx < snd_ctl_elem_info_get_count(elemInfo); idx++) {
        snd_ctl_elem_value_set_integer(elemValue, idx, ctlValue);
    }

    // write default values in newly created control
    snd_ctl_elem_id_alloca(&elemId);
    snd_ctl_elem_info(ctlDev, elemInfo);
    snd_ctl_elem_info_get_id(elemInfo, elemId);
    snd_ctl_elem_value_set_id(elemValue, elemId);
    err = snd_ctl_elem_write(ctlDev, elemValue);
    if (err < 0) {
        AFB_WARNING("addOneSndCtl: crl=%s numid=%d fail to write Values error=%s", json_object_get_string(ctlJ), snd_ctl_elem_info_get_numid(elemInfo), snd_strerror(err));
        goto DoNotUpdate;
    }

    // write a default null TLV (if usefull should be implemented for every ctl type)
    if (elemTlv) {
        err = snd_ctl_elem_tlv_write(ctlDev, elemId, elemTlv);
        if (err < 0) {
            AFB_WARNING("addOneSndCtl: crl=%s numid=%d fail to write TLV error=%s", json_object_get_string(ctlJ), snd_ctl_elem_info_get_numid(elemInfo), snd_strerror(err));
            goto DoNotUpdate;
        }
    }

DoNotUpdate:
    // return newly created as a JSON object
    alsaGetSingleCtl(ctlDev, elemId, &ctlRequest, queryMode);
    if (ctlRequest.used < 0) {
        AFB_WARNING("addOneSndCtl: crl=%s numid=%d Fail to get value", json_object_get_string(ctlJ), snd_ctl_elem_info_get_numid(elemInfo));
    }
    return ctlRequest.valuesJ;

OnErrorExit:
    return NULL;
}

PUBLIC void alsaAddCustomCtls(afb_req request) {
    int err;
    json_object *ctlsJ, *ctlsValues, *ctlValues;
    enum json_type;
    snd_ctl_t *ctlDev = NULL;
    const char *devid, *mode;

    devid = afb_req_value(request, "devid");
    if (devid == NULL) {
        afb_req_fail_f(request, "devid-missing", "devid MUST be defined for alsaAddCustomCtls");
        goto OnErrorExit;
    }

    // open control interface for devid
    err = snd_ctl_open(&ctlDev, devid, 0);
    if (err < 0) {
        afb_req_fail_f(request, "devid-unknown", "SndCard devid=[%s] Not Found err=%s", devid, snd_strerror(err));
        goto OnErrorExit;
    }

    // get verbosity level
    halQueryMode queryMode = QUERY_QUIET;
    mode = afb_req_value(request, "mode");
    if (mode != NULL) {
        sscanf(mode, "%i", (int*) &queryMode);
    }

    // extract sound controls and parse json
    ctlsJ = json_tokener_parse(afb_req_value(request, "ctl"));
    if (!ctlsJ) {
        afb_req_fail_f(request, "ctls-missing", "ctls MUST be defined as a JSON array for alsaAddCustomCtls");
        goto OnErrorExit;
    }

    switch (json_object_get_type(ctlsJ)) {
        case json_type_object:
            ctlsValues = addOneSndCtl(request, ctlDev, ctlsJ, queryMode);
            if (!ctlsValues) goto OnErrorExit;
            break;

        case json_type_array:
            ctlsValues = json_object_new_array();
            for (int idx = 0; idx < json_object_array_length(ctlsJ); idx++) {
                json_object *ctlJ = json_object_array_get_idx(ctlsJ, idx);
                ctlValues = addOneSndCtl(request, ctlDev, ctlJ, queryMode);
                if (ctlValues) json_object_array_add(ctlsValues, ctlValues);
                else goto OnErrorExit;
            }
            break;

        default:
            afb_req_fail_f(request, "ctls-invalid", "ctls=%s not valid JSON array", json_object_get_string(ctlsJ));
            goto OnErrorExit;
    }

    // get ctl as a json response
    afb_req_success(request, ctlsValues, NULL);

OnErrorExit:
    if (ctlDev) snd_ctl_close(ctlDev);
    return;
}
