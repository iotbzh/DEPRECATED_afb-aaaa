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


INCLUDE_DIRECTORIES(${include_dirs})

##################################################
# Inte-HDA sound card Hardware Abstraction Layer
##################################################
ADD_LIBRARY(intel-hda-hal MODULE IntelHdaHAL.c ${halsharedlib})

SET_TARGET_PROPERTIES(intel-hda-hal PROPERTIES
	PREFIX ""
	LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/export.map"
)

TARGET_LINK_LIBRARIES(intel-hda-hal ${link_libraries})
INSTALL(TARGETS intel-hda-hal
        LIBRARY DESTINATION ${binding_install_dir})


