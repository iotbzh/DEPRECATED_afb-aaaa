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

PUBLIC int SamplePolicyInstall (DispatchActionT *action, json_object *response, void *context) {
   
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)calloc (1, sizeof(MyPluginCtxT));
    pluginCtx->magic = MY_PLUGIN_MAGIC;
    pluginCtx->count = 0;

    AFB_INFO ("CONTROLER-PLUGIN-SAMPLE SamplePolicyInstall action=%s args=%s", action->label, jsonToString(action->argsJ));
    
    return 0;
}

PUBLIC int SamplePolicyCount (DispatchActionT *action, json_object *response, void *context) {
   
    MyPluginCtxT *pluginCtx= (MyPluginCtxT*)context;
    //pluginCtx->magic = MY_PLUGIN_MAGIC;
    //pluginCtx->count = 0;

    AFB_INFO ("CONTROLER-PLUGIN-SAMPLE SamplePolicyCount action=%s args=%s count=%d", action->label, jsonToString(action->argsJ), pluginCtx->count);
    
    return 0;
}
