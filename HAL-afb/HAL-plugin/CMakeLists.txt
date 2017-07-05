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

# Needed to remove undefined snd_dlsym_start in plugin share object
add_compile_options(-DPIC)

# Add target to project dependency list
PROJECT_TARGET_ADD(ctl_afbhal)

    # Define targets 
    ADD_LIBRARY(${TARGET_NAME} MODULE HalPlugCtl.c)

    # Alsa Plugin properties
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX "libasound_module_"
        OUTPUT_NAME ${TARGET_NAME}
    )

    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        ${link_libraries}
    )
    install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib/alsa-lib)

PROJECT_TARGET_ADD(cb_sample)

    # Define targets 
    ADD_LIBRARY(${TARGET_NAME} MODULE HalPlugCb.c)

    # Alsa Plugin properties
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	PREFIX "afbhal_"
        OUTPUT_NAME ${TARGET_NAME}
    )

    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(${TARGET_NAME}
        ${link_libraries}
    )
    install(TARGETS ${TARGET_NAME} LIBRARY DESTINATION lib)
