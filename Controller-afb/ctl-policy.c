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
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, something express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <json-c/json_object.h> 

#include "wrap-json.h"
#include "ctl-binding.h"

STATIC PolicyCtlConfigT *ctlHandle = NULL;

// List Avaliable Configuration Files
STATIC json_object* ScanForConfig (char* searchPath) {
    json_object *responseJ;
    DIR  *dirHandle;
    char *dirPath;
    char* dirList= strdup(searchPath);
    
    responseJ = json_object_new_array();
    for (dirPath= strtok(dirList, ":"); dirPath && *dirPath; dirPath=strtok(NULL,":")) {
         struct dirent *dirEnt;
         
        dirHandle = opendir (dirPath);
        if (!dirHandle) {
            AFB_NOTICE ("CONFIG-SCANNING dir=%s not readable", dirPath);
            continue;
        } 
        
        AFB_NOTICE ("CONFIG-SCANNING:ctl_listconfig scanning: %s", dirPath);
        while ((dirEnt = readdir(dirHandle)) != NULL) {
            // Unknown type is accepted to support dump filesystems
            if (dirEnt->d_type == DT_REG || dirEnt->d_type == DT_UNKNOWN) {
                struct json_object *pathJ = json_object_new_object();
                json_object_object_add(pathJ, "dirpath", json_object_new_string(dirPath));
                json_object_object_add(pathJ, "filename", json_object_new_string(dirEnt->d_name));
                json_object_array_add(responseJ, pathJ);
            }
        }
    }
    
    free (dirList);    
    return (responseJ);
}

PUBLIC void ctlapi_authorize (PolicyCtlEnumT control, afb_req request) {
    json_object*tmpJ;
    
    json_object* queryJ= afb_req_json(request);
    int done=json_object_object_get_ex(queryJ, "closing", &tmpJ);
    if (done) return;
    
}


// List Avaliable Configuration Files
PUBLIC void ctlapi_config (struct afb_req request) {
    json_object*tmpJ;
    char *dirList;
    
    json_object* queryJ = afb_req_json(request);
    if (queryJ && json_object_object_get_ex (queryJ, "cfgpath" , &tmpJ)) {
        dirList = strdup (json_object_get_string(tmpJ));
    } else {    
        dirList = strdup (CONTROL_CONFIG_PATH); 
        AFB_NOTICE ("CONFIG-MISSING: use default CONTROL_CONFIG_PATH=%s", CONTROL_CONFIG_PATH);
    }
   
    // get list of config file
    struct json_object *responseJ = ScanForConfig(dirList);
    
    if (json_object_array_length(responseJ) == 0) {
       afb_req_fail(request, "CONFIGPATH:EMPTY", "No Config Found in CONTROL_CONFIG_PATH"); 
    } else {
        afb_req_success(request, responseJ, NULL);
    }
    
    return;
}

STATIC PolicyActionT **PolicyLoadActions (json_object *actionsJ) {
    int err;
    PolicyActionT ** actions;
    
    // unpack individual action object
    int actionUnpack (json_object *actionJ,  PolicyActionT *action) {
        
        err= wrap_json_unpack(actionJ, "{ss,s?s,s?s,s?s,s?s,s?s !}"
            , "label",&action->label, "info",&action->info, "callback",&action->callback, "query",&queryJ, "api",&action->api, "verb", &action->verb);
        if (err) {
            AFB_ERROR ("POLICY-LOAD-ACTION Missing something label|info|callback|api|verb|query in %s", json_object_get_string(actionJ));
            return -1;
        }
        if (!action->callback || !(action->api && action->verb)) {
            AFB_ERROR ("POLICY-LOAD-ACTION Missing something callback|(api+verb) in %s", json_object_get_string(actionJ));            
            return -1;            
        }        
        return 0;
    };
            
    // action array is close with a nullvalue;
    if (json_object_get_type(actionsJ) == json_type_array) {
        int count = json_object_array_length(actionsJ);
        actions = calloc (count+1, sizeof(PolicyActionT));
        
        for (int idx=0; idx < count; idx++) {
            json_object *actionJ = json_object_array_get_idx(actionsJ, idx);
            err = actionUnpack (actionJ, &actions[idx]);
            if (err) goto OnErrorExit;
        }
        
    } else {
        actions = calloc (2, sizeof(PolicyActionT));  
        err = actionUnpack (actionsJ, &actions[0]);
        if (err) goto OnErrorExit;
    }
    
    return actions;
    
 OnErrorExit:
    return NULL;
    
}

// load control policy from file using json_unpack https://jansson.readthedocs.io/en/2.9/apiref.html#parsing-and-validating-values
STATIC PolicyCtlConfigT *PolicyLoadConfig (const char* filepath) {
    json_object *policyConfigJ, *ignoreJ, *actionsJ;
    PolicyCtlConfigT *policyConfig = calloc (1, sizeof(PolicyCtlConfigT));
    int err;
    
    // Load JSON file
    policyConfigJ= json_object_from_file(filepath);
    if (!policyConfigJ) goto OnErrorExit;
    
    json_object *metadataJ, *onloadJ, *controlsJ, *eventsJ;
    err= wrap_json_unpack(policyConfigJ, "{s?o,so,s?o,so,so !}", "$schema", &ignoreJ, "metadata",&metadataJ, "onload",&onloadJ, "controls",&controlsJ, "events",&eventsJ);
    if (err) {
        AFB_ERROR ("POLICY-LOAD-ERRROR Missing something metadata|onload|controls|events in %s", json_object_get_string(policyConfigJ));
        goto OnErrorExit;
    }
    
    PolicyHandleT *policyHandle = calloc (1, sizeof(PolicyHandleT));
    err= wrap_json_unpack(metadataJ, "{so,s?s,s?s !}", "label", &policyHandle->label, "info",&policyHandle->info, "version",&policyHandle->version);
    if (err) {
        AFB_ERROR ("POLICY-LOAD-CONFIG Missing something label|info|version in %s", json_object_get_string(metadataJ));
        goto OnErrorExit;
    }
    
    if (onloadJ) {
        err= wrap_json_unpack(onloadJ, "{s?o,s?s,s?s !}", "info",&ignoreJ, "label",&ignoreJ, "actions",&actionsJ);
        if (err) {
            AFB_ERROR ("POLICY-LOAD-CONFIG Missing something label|info|plugin|actions in %s", json_object_get_string(metadataJ));
            goto OnErrorExit;
        }
        policyConfig->onload = PolicyLoadActions (actionsJ);
    }
    
    return policyControl;
    
OnErrorExit:
    return NULL;
}

// Load default config file at init
PUBLIC int PolicyInit () {
    int index, err;
    
    
    // search for default policy config file
    json_object* responseJ = ScanForConfig(CONTROL_CONFIG_PATH);
    
    for (index=0; index < json_object_array_length(responseJ); index++) {
        json_object *entryJ=json_object_array_get_idx(responseJ, index);
        
        char *filename; char*dirpath;
        err= wrap_json_unpack (entryJ, "{s:s, s:s !}", "dirpath",  &dirpath,"filename", &filename);
        if (err) {
            AFB_ERROR ("POLICY-INIT HOOPs invalid config file path = %s", json_object_get_string(entryJ));
            goto OnErrorExit;
        }
        
        if (strcasestr(filename, CONTROL_CONFIG_FILE)) {
            char filepath[255];
            strncpy(filepath, dirpath, sizeof(filepath)); 
            strncat(filepath, "/", sizeof(filepath)); 
            strncat(filepath, filename, sizeof(filepath)); 
            ctlHandle = PolicyLoadConfig (filepath);
            if (!ctlHandle) goto OnErrorExit;
            break;
        }
    }
    
    // no policy config found remove control API from binder
    if (index == 0)  goto OnErrorExit;
    
    AFB_NOTICE ("SUCCES: Audio Control Policy Init");
    return 0;
    
OnErrorExit:
    AFB_NOTICE ("ERROR: Audio Control Policy Init");
    return 1;    
}
 

 
