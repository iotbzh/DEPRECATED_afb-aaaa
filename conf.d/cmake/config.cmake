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

# Project Info
# ------------------
set(PROJECT_NAME audio-bindings)
set(PROJECT_VERSION "0.1")
set(PROJECT_PRETTY_NAME "Audio Agent")
set(PROJECT_DESCRIPTION "Expose Alsa through AGL AppFw")
set(PROJECT_URL "https://github.com/iotbzh/audio-bindings")
set(PROJECT_ICON "icon.png")
set(PROJECT_AUTHOR "Fulup, Ar Foll")
set(PROJECT_AUTHOR_MAIL "fulup@iot.bzh")
set(PROJECT_LICENCE "Apache-V2")
set(PROJECT_LANGUAGES,"C")


# Where are stored default templates files from submodule or subtree app-templates in your project tree
# relative to the root project directory
set(PROJECT_APP_TEMPLATES_DIR "conf.d/app-templates")

# Use any directory that does not start with _ as valid source rep
set(PROJECT_SRC_DIR_PATTERN "[^_]*")

# Compilation Mode (DEBUG, RELEASE)
# ----------------------------------
set(CMAKE_BUILD_TYPE "DEBUG")

# Static constante definition
# -----------------------------
add_compile_options(-DMAX_SND_CARD=16)        # should be more than enough even in luxury vehicule 
add_compile_options(-DMAX_LINEAR_DB_SCALE=24) # until 24db volume normalisation use a simple linear scale
add_compile_options(-DTLV_BYTE_SIZE=256)       # Alsa use 4096 as default but 256 should fit most sndcards 

# Compiler selection if needed. Overload the detected compiler.
# -----------------------------------------------
set (gcc_minimal_version 4.9)
#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")

# PKG_CONFIG required packages
# -----------------------------
set (PKG_REQUIRED_LIST
	alsa>=1.1.2
	libsystemd>=222
        libmicrohttpd>=0.9.55
	afb-daemon
	json-c
        libafbwsc
        lua>=5.3
)

# Define CONTROL_CDEV_NAME should match MOST driver values
# ---------------------------------------------------------
  add_compile_options(-DCONTROL_MAXPATH_LEN=255)
  add_compile_options(-DCONTROL_CONFIG_FILE="onload-control-policy.json")
  add_compile_options(-DCONTROL_CONFIG_PATH="${CMAKE_SOURCE_DIR}/conf.d:${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/config")

  add_compile_options(-DCONTROL_LUA_PATH="${CMAKE_SOURCE_DIR}/conf.d:/etc/default/${PROJECT_NAME}/lua:${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}/lua")

 
  set (CTL_PLUGIN_PRE "audio-")
  set (CTL_PLUGIN_EXT ".ctlso")
  add_compile_options(-DCTL_PLUGIN_MAGIC=2468013579)
  add_compile_options(-DCONTROL_PLUGIN_PATH="${BINDINGS_INSTALL_DIR}/controler:/usr/lib/${PROJECT_NAME}")
  
# Print a helper message when every thing is finished
# ----------------------------------------------------
set(CLOSING_MESSAGE "Debug in ./buid: afb-daemon --port=1234 --ldpaths=. --workdir=. --roothttp=../htdocs --tracereq=common --token='' --verbose")


# Optional dependencies order
# ---------------------------
#set(EXTRA_DEPENDENCIES_ORDER)

# Optional Extra global include path
# -----------------------------------
# set(EXTRA_INCLUDE_DIRS)

# Optional extra libraries
# -------------------------
# set(EXTRA_LINK_LIBRARIES)

# Optional force binding installation
# ------------------------------------
# set(BINDINGS_INSTALL_PREFIX PrefixPath )

# Optional force widget prefix generation
# ------------------------------------------------
# set(WIDGET_PREFIX DestinationPath)

# Optional Widget entry point file.
# ---------------------------------------------------------
# This is the file that will be executed, loaded,...
# at launch time by the application framework

# set(WIDGET_ENTRY_POINT EntryPoint_Path)

# Optional Widget Mimetype specification
# --------------------------------------------------
# Choose between :
# - application/x-executable
# - application/vnd.agl.url
# - application/vnd.agl.service
# - application/vnd.agl.native
# - text/vnd.qt.qml
# - application/vnd.agl.qml
# - application/vnd.agl.qml.hybrid
# - application/vnd.agl.html.hybrid
#
set(WIDGET_TYPE application/vnd.agl.service)

# Optional force binding Linking flag
# ------------------------------------
# set(BINDINGS_LINK_FLAG LinkOptions )

# This include is mandatory and MUST happens at the end
# of this file, else you expose you to unexpected behavior
# -----------------------------------------------------------
include(${PROJECT_APP_TEMPLATES_DIR}/cmake/common.cmake)
