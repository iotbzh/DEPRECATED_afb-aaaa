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

#pragma once

#include <stdint.h>

extern int wrap_ucs_subscribe_sync();
extern int wrap_ucs_getconfig_sync(const char *config_path);
extern int wrap_ucs_initialize_sync(const char* file_name);
extern int wrap_ucs_i2cwrite_sync(uint16_t node, uint8_t *data_ptr, uint8_t data_sz);
extern int wrap_ucs_i2cwrite(uint16_t node, uint8_t *data_ptr, uint8_t data_sz);
