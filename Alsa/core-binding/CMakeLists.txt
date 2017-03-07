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
# AlsaBinding
##################################################
ADD_LIBRARY(alsacore-afb MODULE AlsaAfbBinding.c AlsaLibMapping.c)

SET_TARGET_PROPERTIES(alsacore-afb PROPERTIES
	PREFIX ""
	LINK_FLAGS "-Wl,--version-script=${CMAKE_SOURCE_DIR}/export.map"
)

TARGET_LINK_LIBRARIES(alsacore-afb ${link_libraries})
INSTALL(TARGETS alsacore-afb
        LIBRARY DESTINATION ${binding_install_dir})


