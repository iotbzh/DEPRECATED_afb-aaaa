###########################################################################
# Copyright 2015, 2016, 2017 IoT.bzh
#
# author: Fulup Ar Foll <fulup@iot.bzh>
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
###########################################################################

PROJECT(hadware-abstraction-layer C)

# Fulup NOTE: this might have to be change for a formal static/dynamic lib
set(halsharedlib ${CMAKE_CURRENT_SOURCE_DIR}/Shared/SharedHalLib.c)

ADD_SUBDIRECTORY(IntelHda)
ADD_SUBDIRECTORY(Shared)

