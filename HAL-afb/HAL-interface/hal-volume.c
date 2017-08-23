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
 * references:
 *   alsa-util/amixer.c + alsa-lib/simple.c
 *   snd_tlv_convert_from_dB
 *   nt snd_tlv_convert_to_dB
 *   snd_tlv_get_dB_range
 */

#define _GNU_SOURCE  // needed for vasprintf
#include <math.h>
#include "hal-interface.h"

typedef enum {
    NORMALIZE_NONE = 0,
    NORMALIZE_DB_LINEAR,
    NORMALIZE_DB_MATH,
    NORMALIZE_LINEAR,
} enumRandeModeDB_T;


// Return Value express from 0-100%

STATIC int dbNormalizeVal(enumRandeModeDB_T normaliseMode, const alsaHalDBscaleT *dbscale, int value) {
    double normalized, min_norm;

    // To get real DB from TLV DB values should be divided by 100
    switch (normaliseMode) {
        case NORMALIZE_DB_LINEAR:
            normalized = ((double) (value - dbscale->min) / (double) (dbscale->max - dbscale->min));
            break;

        case NORMALIZE_DB_MATH:
            normalized = exp10((double) (value - dbscale->max) / 6000.0);
            if (dbscale->min != SND_CTL_TLV_DB_GAIN_MUTE) {
                min_norm = exp10((double) (dbscale->min - dbscale->max) / 20);
                normalized = (normalized - min_norm) / (1 - min_norm);
            }

            break;

        default:
            normalized = 0;
    }

    return (int) round(normalized * 100);
}

// HAL normalise volume values to 0-100%

PUBLIC json_object *volumeNormalise(ActionSetGetT action, const alsaHalCtlMapT *halCtls, json_object *valuesJ) {
    enumRandeModeDB_T useNormalizeDB;
    int length;

    // If Integer look for DBscale
    if (halCtls->type == SND_CTL_ELEM_TYPE_INTEGER) {

        // If not valid db_scale let's use raw_scale
        if (!halCtls->dbscale || (halCtls->dbscale->min >= halCtls->dbscale->max)) {

            // dbscale is invalid let's try raw range
            if (halCtls->minval >= halCtls->maxval) goto ExitOnError;

            // Use Raw Scale Model
            useNormalizeDB = NORMALIZE_LINEAR;

        } else { // db_scale looks OK let's use it
            if ((halCtls->dbscale->max - halCtls->dbscale->min) <= MAX_LINEAR_DB_SCALE * 100) useNormalizeDB = NORMALIZE_DB_LINEAR;
            else useNormalizeDB = NORMALIZE_LINEAR; // Fulup not sure how to handle this useNormalizeDB=NORMALIZE_DB_MATH;
        }
    } else useNormalizeDB = NORMALIZE_NONE;


    // loop on values to normalise
    enum json_type jtype = json_object_get_type(valuesJ);
    if (jtype == json_type_array) length = json_object_array_length(valuesJ);
    else length = 1;

    json_object *normalisedJ = json_object_new_array();
    for (int idx = 0; idx < length; idx++) {
        int value;

        if (jtype == json_type_array) {
            json_object *valueJ = json_object_array_get_idx(valuesJ, idx);
            value = json_object_get_int(valueJ);
        } else {
            value = json_object_get_int(valuesJ);
        }

        // If Integer scale to 0/100
        if (halCtls->type == SND_CTL_ELEM_TYPE_INTEGER) {

            switch (action) {

                case ACTION_GET:
                    switch (useNormalizeDB) {
                        case NORMALIZE_LINEAR:
                            value = 100 * (value - halCtls->minval) / (halCtls->maxval - halCtls->minval);
                            break;
                        case NORMALIZE_DB_MATH: //ToBeDone
                            value = dbNormalizeVal(useNormalizeDB, halCtls->dbscale, value);
                            break;
                        case NORMALIZE_NONE:
                        default:
                            value = value;
                    }
                    break;

                case ACTION_SET:
                    switch (useNormalizeDB) {
                        case NORMALIZE_LINEAR:
                            value = (value * (halCtls->maxval - halCtls->minval)) / 100;
                            break;
                        case NORMALIZE_DB_MATH: //ToBeDone
                            value = dbNormalizeVal(useNormalizeDB, halCtls->dbscale, value);
                            break;
                        case NORMALIZE_NONE:
                        default:
                            value = value;
                    }
                    break;

                default:
                    AFB_NOTICE("volumeNormalise: invalid action value=%d", (int) action);
                    goto ExitOnError;
            }
        }

        json_object_array_add(normalisedJ, json_object_new_int(value));
    }

    return (normalisedJ);

ExitOnError:
    return NULL;
}
