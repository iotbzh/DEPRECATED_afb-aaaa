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
 */

#define _GNU_SOURCE  // needed for vasprintf
#include <math.h>
#include "hal-interface.h"


typedef enum {  
   DB_NORMALIZE_LINEAR,
   DB_NORMALIZE_MATH,           
} enumRandeModeDB_T;

// Return Value express from 0-100%
STATIC int dbNormalizeVal (enumRandeModeDB_T normaliseMode, const alsaHalDBscaleT *dbscale, int value) {
    double normalized, min_norm;
    
    // To get real DB from TLV DB values should be divided by 100
    switch (normaliseMode) {
        case DB_NORMALIZE_LINEAR:
            normalized = ((double)(value-dbscale->min)/(double)(dbscale->max-dbscale->min))*100;
            break;
            
        case  DB_NORMALIZE_MATH:
            normalized = exp10((double)(value - dbscale->max) / 6000.0);
            if (dbscale->min != SND_CTL_TLV_DB_GAIN_MUTE) {
                    min_norm = exp10((double)(dbscale->min - dbscale->max) / 6000.0);
                    normalized = (normalized - min_norm) / (1 - min_norm);
            }
            break;
            
        default:
            normalized=0;            
    }
    
    return (int)round(normalized*100);   
}

// HAL normalise volume values to 0-100%
PUBLIC struct json_object *GetNormaliseVolume(const alsaHalCtlMapT *halCtls,  struct json_object *valuesJ) {
    int useNormalizeDB;
    
    // If not valid db_scale let's use raw_scale
    if (!halCtls->dbscale || (halCtls->dbscale->min >= halCtls->dbscale->max)) {
       
        // dbscale is invalid let's try raw range
        if (halCtls->minval >= halCtls->maxval) goto ExitOnError;
        
        // Use Raw Scale Model
        useNormalizeDB= 0;
        
    } else { // db_scale looks OK let's use it
        if ((halCtls->dbscale->max - halCtls->dbscale->min) <= MAX_LINEAR_DB_SCALE * 100) useNormalizeDB= DB_NORMALIZE_LINEAR;
        else useNormalizeDB =  DB_NORMALIZE_MATH;
            
    }
    
    // loop on values to normalise
    int length = json_object_array_length(valuesJ);   
    json_object *normalisedJ= json_object_new_array();
    for (int idx=0; idx <length; idx++) {
        json_object *valueJ = json_object_array_get_idx (valuesJ, idx);
        int value = json_object_get_int(valueJ);
               
        // If Integer scale to 0/100
        if (halCtls->type == SND_CTL_ELEM_TYPE_INTEGER) {
            if (useNormalizeDB) value = dbNormalizeVal (useNormalizeDB, halCtls->dbscale, value);
            else value = 100* (value - halCtls->minval) / (halCtls->maxval - halCtls->minval);
        } 
        
        json_object_array_add(normalisedJ, json_object_new_int(value));  
    }
    
    return (normalisedJ);
    
  ExitOnError:
        return NULL;
}
    