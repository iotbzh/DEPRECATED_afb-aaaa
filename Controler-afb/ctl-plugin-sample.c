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

// Declare this sharelib as a Controller Plugin
CTL_PLUGIN_REGISTER("MyCtlSamplePlugin");

STATIC const char* jsonToString (json_object *valueJ) {
    const char *value;
    if (valueJ)
        value=json_object_get_string(valueJ);
    else 
        value="NULL";
    
    return value;
}

// Call at initialisation time
PUBLIC void* CtlPluginOnload(char* label, char* version, char* info) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = -1;

    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:Onload label=%s version=%s info=%s", label, info, version);
    return (void*)pluginCtx;
}

PUBLIC int SamplePolicyInit (afb_req request, DispatchActionT *action, void *context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    pluginCtx->count = 0;
    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:Init label=%s args=%s\n", action->label, jsonToString(action->argsJ));
    return 0;
}

PUBLIC int sampleControlMultimedia (afb_req request, DispatchActionT *action, void *context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLER-PLUGIN-SAMPLE:sampleControlMultimedia (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s count=%d", action->label, jsonToString(action->argsJ), pluginCtx->count);
    return 0;
}

PUBLIC int sampleControlNavigation (afb_req request, DispatchActionT *action, void *context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLER-PLUGIN-SAMPLE:sampleControlNavigation (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:sampleControlNavigation SamplePolicyCount action=%s args=%s count=%d", action->label, jsonToString(action->argsJ), pluginCtx->count);
    return 0;
}

PUBLIC int SampleControlEvent (afb_req request, DispatchActionT *action, void *context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLER-PLUGIN-SAMPLE:cousampleControlMultimediant (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s count=%d", action->label, jsonToString(action->argsJ), pluginCtx->count);
    return 0;
}
