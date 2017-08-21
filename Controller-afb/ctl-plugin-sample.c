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

STATIC const char* jsonToString (json_object *valueJ) {
    const char *value;
    if (valueJ)
        value=json_object_get_string(valueJ);
    else 
        value="NULL";
    
    return value;
}

// Declare this sharelib as a Controller Plugin
CTLP_REGISTER("MyCtlSamplePlugin");

// Call at initialisation time
PUBLIC CTLP_ONLOAD(label, version, info) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = -1;

    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Onload label=%s version=%s info=%s", label, info, version);
    return (void*)pluginCtx;
}

PUBLIC CTLP_CAPI (SamplePolicyInit, source, label, argsJ, queryJ, context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:SamplePolicyInit (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    
    pluginCtx->count = 0;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Init label=%s args=%s\n", label, jsonToString(argsJ));
    return 0;
}

PUBLIC CTLP_CAPI (sampleControlMultimedia, source, label, argsJ,queryJ,context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s query=%s count=%d"
               , label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}

PUBLIC  CTLP_CAPI (sampleControlNavigation, source, label, argsJ, queryJ, context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:sampleControlNavigation (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlNavigation SamplePolicyCount action=%s args=%s query=%s count=%d"
               ,label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}

PUBLIC  CTLP_CAPI (SampleControlEvent, source, label, argsJ, queryJ, context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:cousampleControlMultimediant (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:sampleControlMultimedia SamplePolicyCount action=%s args=%s query=%s count=%d"
               ,label, jsonToString(argsJ), jsonToString(queryJ), pluginCtx->count);
    return 0;
}

// This function is a LUA function. Lua2CHelloWorld label should be declare in the "onload" section of JSON config file
PUBLIC CTLP_LUA2C (Lua2cHelloWorld1, label, argsJ, context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld1 (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld1 SamplePolicyCount action=%s args=%s count=%d"
               ,label, jsonToString(argsJ), pluginCtx->count);
    return 0;
}

// This function is a LUA function. Lua2CHelloWorld label should be declare in the "onload" section of JSON config file
PUBLIC CTLP_LUA2C (Lua2cHelloWorld2, label, argsJ, context) {
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    
    if (!context || pluginCtx->magic != MY_PLUGIN_MAGIC) {
        AFB_ERROR("CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld2 (Hoops) Invalid Sample Plugin Context");
        return -1;
    };
    pluginCtx->count++;
    AFB_NOTICE ("CONTROLLER-PLUGIN-SAMPLE:Lua2cHelloWorld2 SamplePolicyCount action=%s args=%s count=%d"
               ,label, jsonToString(argsJ), pluginCtx->count);
    return 0;
}
