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

#include "wrap_volume.h"
#include "wrap_unicens.h"

extern int wrap_volume_init(void) {
    
    /*lib_most_volume_init_t mv_init;
    mv_init.writei2c_cb = &wrap_ucs_i2cwrite;
    mv_init.service_cb = NULL;
    lib_most_volume_init(NULL);*/
    
    return -1;
}