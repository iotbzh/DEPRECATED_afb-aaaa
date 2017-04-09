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
PROJECT_TARGET_ADD(hal-intel-hda)

    # Define project Targets
    ADD_LIBRARY(hal-intel-hda MODULE IntelHdaHAL.c)

    # Binder exposes a unique public entry point
    SET_TARGET_PROPERTIES(hal-intel-hda PROPERTIES
	PREFIX ""
	LINK_FLAGS ${BINDINGS_LINK_FLAG}
        OUTPUT_NAME intel-hda-binding
    )

    # Library dependencies (include updates automatically)
    TARGET_LINK_LIBRARIES(hal-intel-hda 
        hal-interface
        audio-interface
    )

    # installation directory
    INSTALL(TARGETS hal-intel-hda
        LIBRARY DESTINATION ${BINDINGS_INSTALL_DIR})
