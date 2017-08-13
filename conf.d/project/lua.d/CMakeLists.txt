###########################################################################
# Copyright 2017 IoT.bzh
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


##################################################
# Control Policy Config file
##################################################
PROJECT_TARGET_ADD(ctl-lua.d)

    file(GLOB XML_FILES "*.lua")

    add_custom_target(${TARGET_NAME}
       DEPENDS  ${PROJECT_PKG_BUILD_DIR}/${TARGET_NAME}
    )

    # check XML schema before pushing config
    add_custom_command(
       DEPENDS  ${XML_FILES}
       OUTPUT ${PROJECT_PKG_BUILD_DIR}/${TARGET_NAME}
       # COMMAND xmllint -schema ${XML_SCHEMA} ${XML_FILES} --noout (Fulup we miss this for JSON)
       COMMAND mkdir -p ${PROJECT_PKG_BUILD_DIR}/${TARGET_NAME}
       COMMAND touch ${PROJECT_PKG_BUILD_DIR}/${TARGET_NAME}      
       COMMAND cp -r ${XML_FILES} ${PROJECT_PKG_BUILD_DIR}/${TARGET_NAME}
    )

