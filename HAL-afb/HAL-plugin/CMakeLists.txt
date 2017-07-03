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


# Add target to project dependency list
PROJECT_TARGET_ADD(hal-ctl-plugin)

    # Define targets 
    ADD_LIBRARY(hal-ctl-plugin MODULE HalPlugCtl.c)

    # Alsa Plugin properties
    SET_TARGET_PROPERTIES(hal-ctl-plugin PROPERTIES
	PREFIX "alsa-"
        OUTPUT_NAME ${TARGET_NAME}
    )

PROJECT_TARGET_ADD(hal-pcm-plugin)

    # Define targets 
    ADD_LIBRARY(hal-pcm-plugin MODULE HalPlugPcm.c)

    # Alsa Plugin properties
    SET_TARGET_PROPERTIES(hal-pcm-plugin PROPERTIES
	PREFIX "alsa-"
        OUTPUT_NAME ${TARGET_NAME}
    )
