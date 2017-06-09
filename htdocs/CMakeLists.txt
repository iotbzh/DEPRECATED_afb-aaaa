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



##################################################
# HTML Testing Files
##################################################
PROJECT_TARGET_ADD(html-simple-test)

    add_custom_command(OUTPUT htdocs
	DEPENDS ${TARGET_NAME}
	COMMAND mkdir -p htdocs
	COMMAND touch htdocs
	COMMAND cp -f ${CMAKE_CURRENT_SOURCE_DIR}/*.html ${CMAKE_CURRENT_SOURCE_DIR}/*.js htdocs
    )

    add_custom_target(${TARGET_NAME} ALL DEPENDS htdocs)

    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
	LABELS "HTDOCS"
	OUTPUT_NAME htdocs
    )

    # use only under native Linux when using "make install"
    install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR} DESTINATION . FILES_MATCHING PATTERN "*.html"  PATTERN "*.js" PATTERN "*.jpg")

