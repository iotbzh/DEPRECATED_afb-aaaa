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

typedef struct parse_result_ {
    int done;
    char *str_result;
} parse_result_t;

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

/* Checks if name ends with a given letter. */
static int wrap_ucs_string_ends_with(const char *name, char letter) {

    int result = 0;
    size_t len = strlen(name);

    if (len > 0) {

        if (name[len] == letter) {
            result = 1;
        }
    }

    return result;
}

/* Callback for iteration through found files. Marks search as "done" as
 * soon as the search pattern "kit.xml" matches.
 * \param closure   User reference. Points to parse_result_t.
 * \param j_obj     Points to json object within array.
 */
static void wrap_ucs_find_xml_cb(void *closure, struct json_object *j_obj) {

    const char *dir, *name;
    parse_result_t *result;

    if (!closure)
        return;

    result = (parse_result_t *)closure;
    if (result->done)
        return;

    wrap_json_unpack(j_obj, "{s:s, s:s}", "dirpath", &dir, "basename", &name);
    AFB_DEBUG("Found file: %s", name);

    if(strstr(name, "kit.xml") != NULL) {
        size_t sz = strlen(dir)+strlen(name)+10;
        char * full_path = malloc(sz);

        strncpy(full_path, dir, sz);

        if (!wrap_ucs_string_ends_with(dir, '/'))
            strncat(full_path, "/", sz);

        strncat(full_path, name, sz);
        AFB_DEBUG("Found XML file: %s", full_path);

        result->done = 1;
        result->str_result = full_path;
    }
}

/* Retrieves XML configuration path to initialize unicens2-binding.
 * \param   config_path Directory containing XML configuration file(s).
 * \param   file_found  Points to found file_path or NULL if not found.
 *          Memory must be released by calling free().
 * \return  Returns 0 if successful, otherwise != 0".
 */
extern int wrap_ucs_getconfig_sync(const char *config_path, char **file_found) {

    int err = 0;
    json_object *j_response, *j_query, *j_paths = NULL;
    parse_result_t result = {.done = 0, .str_result = NULL};

    *file_found = NULL;

    /* Build JSON object to retrieve UNICENS configuration */
    if ((config_path == NULL) || (strcmp(config_path, "") == 0))
        err = wrap_json_pack(&j_query, "{}");
    else
        err = wrap_json_pack(&j_query, "{s:s}", "cfgpath", config_path);

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
        AFB_DEBUG("UNICENS listconfig result, res=%s", json_object_to_json_string(j_response));

        if (json_object_object_get_ex(j_response, "response", &j_paths)) {

            AFB_DEBUG("UNICENS listconfig result, paths=%s", json_object_to_json_string(j_paths));

            wrap_json_optarray_for_all(j_paths, &wrap_ucs_find_xml_cb, &result);

            if (result.done) {
                *file_found = strdup(result.str_result);
                free(result.str_result);
                result.str_result = NULL;
            }
            else {
                AFB_NOTICE("wrap_ucs_getconfig_sync, search pattern does not match for paths=%s", json_object_to_json_string(j_paths));
                err = -1;
            }
        }

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
        AFB_ERROR("Failed to call writei2c_sync");
        goto OnErrorExit;
    }
    else {
        AFB_INFO("Called writei2c_sync, res=%s", json_object_to_json_string(j_response));
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

    AFB_INFO("wrap_ucs_i2cwrite_cb: closure=%p status=%d, res=%s", closure, status, json_object_to_json_string(j_result));

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
