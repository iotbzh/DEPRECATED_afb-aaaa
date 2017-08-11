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
 * Sample plugin for Controller
 */


#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>

#include "ctl-binding.h"

#define MY_PLUGIN_MAGIC 987654321

typedef struct {
  int magic;
  int count;  
} MyPluginCtxT;

// This tag is mandotory and used as sanity check when loading plugin
PUBLIC ulong CtlPluginMagic=CTL_PLUGIN_MAGIC;

// Call at initialisation time
PUBLIC void* CtlPluginOnload(char* label, char* version, char* info) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = 0;

    fprintf(stderr, "*** CONTROLER-PLUGIN-SAMPLE:Install label=%s version=%s info=%s", label, info, version);
    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:Install label=%s version=%s info=%s", label, info, version);
    return (void*)pluginCtx;
}

STATIC const char* jsonToString (json_object *valueJ) {
    const char *value;
    if (valueJ)
        value=json_object_get_string(valueJ);
    else 
        value="NULL";
    
    return value;
}


PUBLIC void SamplePolicyCount (afb_req request, DispatchActionT *action, void *context) {
   
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLER-PLUGIN-SAMPLE:count (Hoops) Invalid Sample Plugin Context");
        return;
        
    };
    
    pluginCtx->count++;

    AFB_INFO ("CONTROLER-PLUGIN-SAMPLE:Count SamplePolicyCount action=%s args=%s count=%d", action->label, jsonToString(action->argsJ), pluginCtx->count);
}
