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
#include <time.h>
#include <json-c/json.h>
#include <afb/afb-binding.h>

#include "wrap_volume.h"
#include "wrap_unicens.h"
#include "libmostvolume.h"

static int wrap_volume_service_timeout_cb(sd_event_source* source,
        uint64_t timer __attribute__((__unused__)),
        void *userdata __attribute__((__unused__))) {
    
    uint8_t ret;

    sd_event_source_unref(source);
    ret = lib_most_volume_service();
    
    if (ret != 0U) {
        AFB_ERROR("lib_most_volume_service returns %d", ret);
    }
    
    return 0;
}

static void wrap_volume_service_cb(uint16_t timeout) {
    uint64_t usec;
    sd_event_now(afb_daemon_get_event_loop(), CLOCK_BOOTTIME, &usec);
    sd_event_add_time(  afb_daemon_get_event_loop(), NULL, CLOCK_MONOTONIC, 
                        usec + (timeout*1000), 
                        250, 
                        wrap_volume_service_timeout_cb, 
                        NULL);
}

extern int wrap_volume_init(void) {
    
    uint8_t ret = 0U;
    lib_most_volume_init_t mv_init;
    
    mv_init.writei2c_cb = &wrap_ucs_i2cwrite;
    mv_init.service_cb = wrap_volume_service_cb;
    
    ret = lib_most_volume_init(&mv_init);
    
    return ret * (-1);
}