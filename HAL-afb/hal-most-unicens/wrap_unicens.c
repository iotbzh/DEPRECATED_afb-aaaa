/*
 * Copyright (C) 2017, Microchip Technology Inc. and its subsidiaries.
 * Author Tobias Jahnke
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
 */
#define _GNU_SOURCE 
#define AFB_BINDING_VERSION 2

#include <string.h>
#include <json-c/json.h>
#include <afb/afb-binding.h>

#include "wrap_unicens.h"
#include "wrap-json.h"

typedef struct async_job_ {
    wrap_ucs_result_cb_t result_fptr; 
    void *result_user_ptr;
} async_job_t; 

/* Subscribes to unicens2-binding events.
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_subscribe_sync() {
    
    json_object *j_response, *j_query = NULL;
    int err;
    
    /* Build an empty JSON object */
    err = wrap_json_pack(&j_query, "{}");
    if (err) {
        AFB_ERROR("Failed to create subscribe json object");
        goto OnErrorExit; 
    }
    
    err = afb_service_call_sync("UNICENS", "subscribe", j_query, &j_response);
    if (err) {
        AFB_ERROR("Fail subscribing to UNICENS events");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("Subscribed to UNICENS events, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    json_object_put(j_query);
    j_query = NULL;
    
OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return err;
}

/* Retrieves XML configuration path to initialize unicens2-binding.
 * \param   config_path     Directory containing XML configuration file(s).
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_getconfig_sync(const char *config_path) {
    json_object *j_response, *j_query = NULL;
    int err;
    
    /* Build JSON object to retrieve UNICENS configuration */
    if (config_path != NULL) 
        err = wrap_json_pack(&j_query, "{s:s}", "cfgpath", config_path);
    else
        err = wrap_json_pack(&j_query, "{}");
    
    if (err) {
        AFB_ERROR("Failed to create listconfig json object");
        goto OnErrorExit; 
    }
    
    err = afb_service_call_sync("UNICENS", "listconfig", j_query, &j_response);
    if (err) {
        AFB_ERROR("Failed to call listconfig");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("UNICENS listconfig result, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    json_object_put(j_query);
    j_query = NULL;
    
OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return err;
}


/* Initializes the unicens2-binding.
 * \param   file_name     Path to XML configuration file or \c NULL for
 *                        first found file in default path. 
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_initialize_sync(const char *file_name) {
    json_object *j_response, *j_query = NULL;
    int err;
    
    /* Build JSON object to initialize UNICENS */
    if (file_name != NULL) 
        err = wrap_json_pack(&j_query, "{s:s}", "filename", file_name);
    else
        err = wrap_json_pack(&j_query, "{}");
    
    if (err) {
        AFB_ERROR("Failed to create initialize json object");
        goto OnErrorExit; 
    }
    err = afb_service_call_sync("UNICENS", "initialise", j_query, &j_response);
    if (err) {
        AFB_ERROR("Failed to initialize UNICENS");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("Initialized UNICENS, res=%s", json_object_to_json_string(j_response));
        json_object_put(j_response);
    }
    
    json_object_put(j_query);
    j_query = NULL;
    
OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return err;
}

/* Write I2C command to a network node.
 * \param   node     Node address
 * \param   data_ptr Reference to command data 
 * \param   data_sz  Size of the command data. Valid values: 1..32.
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_i2cwrite_sync(uint16_t node, uint8_t *data_ptr, uint8_t data_sz) {

    json_object *j_response, *j_query, *j_array = NULL;
    int err;
    uint8_t cnt;

    j_query = json_object_new_object();
    j_array = json_object_new_array();
    
    if (!j_query || !j_array) {
        err = -1;
        AFB_ERROR("Failed to create writei2c json objects");
        goto OnErrorExit; 
    }     
    
    for (cnt = 0U; cnt < data_sz; cnt++) {
        json_object_array_add(j_array, json_object_new_int(data_ptr[cnt]));
    }
    
    json_object_object_add(j_query, "node", json_object_new_int(node));
    json_object_object_add(j_query, "data", j_array);
 
    err = afb_service_call_sync("UNICENS", "writei2c", j_query, &j_response);
    
    if (err) {
        AFB_ERROR("Failed to call writei2c");
        goto OnErrorExit;
    }
    else {
        AFB_NOTICE("Called writei2c, res=%s", "async"/*json_object_to_json_string(j_response)*/);
        json_object_put(j_response);
    }
    
    json_object_put(j_query);
    j_query = NULL;

OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return err;  
}

/* ------------------ ASYNCHRONOUS API ---------------------------------------*/

static void wrap_ucs_i2cwrite_cb(void *closure, int status, struct json_object *j_result) {
    
    AFB_NOTICE("wrap_ucs_i2cwrite_cb: closure=%p status=%d, res=%s", closure, status, json_object_to_json_string(j_result));
    
    if (closure) {
        async_job_t *job_ptr = (async_job_t *)closure;
        
        if (job_ptr->result_fptr)
            job_ptr->result_fptr(0U, job_ptr->result_user_ptr);
        
        free(closure);
    }
}

/* Write I2C command to a network node.
 * \param   node     Node address
 * \param   data_ptr Reference to command data 
 * \param   data_sz  Size of the command data. Valid values: 1..32.
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_i2cwrite(uint16_t node, uint8_t *data_ptr, uint8_t data_sz,
                             wrap_ucs_result_cb_t result_fptr, 
                             void *result_user_ptr) {

    json_object *j_query, *j_array = NULL;
    async_job_t *job_ptr = NULL;
    int err;
    uint8_t cnt;

    j_query = json_object_new_object();
    j_array = json_object_new_array();
    
    if (!j_query || !j_array) {
        err = -1;
        AFB_ERROR("Failed to create writei2c json objects");
        goto OnErrorExit; 
    }     
    
    for (cnt = 0U; cnt < data_sz; cnt++) {
        json_object_array_add(j_array, json_object_new_int(data_ptr[cnt]));
    }
    
    json_object_object_add(j_query, "node", json_object_new_int(node));
    json_object_object_add(j_query, "data", j_array);
    
    job_ptr = malloc(sizeof(async_job_t));
    
    if (!job_ptr) {
        err = -1;
        AFB_ERROR("Failed to create async job object");
        goto OnErrorExit; 
    }
    
    job_ptr->result_fptr = result_fptr;
    job_ptr->result_user_ptr = result_user_ptr;
 
    afb_service_call("UNICENS", "writei2c", j_query, wrap_ucs_i2cwrite_cb, job_ptr);
    err = 0;
    j_query = NULL;

OnErrorExit:
    if (j_query)
        json_object_put(j_query);
    return err;  
}
