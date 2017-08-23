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

# Compiler selection if needed. Overload the detected compiler.
# -----------------------------------------------
set (gcc_minimal_version 4.9)
#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")

# When Present LUA is used by the controller
# ---------------------------------------------------------------
set(CONTROL_SUPPORT_LUA 1 CACHE BOOL "Active or not LUA Support")

# PKG_CONFIG required packages
# -----------------------------
set (PKG_REQUIRED_LIST
	alsa>=1.1.2
	libsystemd>=222
	libmicrohttpd>=0.9.55
	afb-daemon
	json-c
	libafbwsc
)

# Compilation options definition
# Use CMake generator expressions to specify only for a specific language
# Values are prefilled with default options that is currently used.
# Either separate options with ";", or each options must be quoted separately
# DO NOT PUT ALL OPTION QUOTED AT ONCE , COMPILATION COULD FAILED !
# ----------------------------------------------------------------------------
set(COMPILE_OPTIONS
-Wall
-Wextra
-Wconversion
-Wno-unused-parameter
-Wno-sign-compare
-Wno-sign-conversion
-Werror=maybe-uninitialized
-Werror=implicit-function-declaration
-ffunction-sections
-fdata-sections
-fPIC
# Personal compilation options
-DMAX_SND_CARD=16        # should be more than enough even in luxury vehicule
-DMAX_LINEAR_DB_SCALE=24 # until 24db volume normalisation use a simple linear scale
-DTLV_BYTE_SIZE=256      # Alsa use 4096 as default but 256 should fit most sndcards
-DCONTROL_MAXPATH_LEN=255
 CACHE STRING "Compilation flags")
#set(C_COMPILE_OPTIONS "" CACHE STRING "Compilation flags for C language.")
#set(CXX_COMPILE_OPTIONS "" CACHE STRING "Compilation flags for C++ language.")
#set(PROFILING_COMPILE_OPTIONS -g -O0 -pg -Wp,-U_FORTIFY_SOURCE CACHE STRING "Compilation flags for PROFILING build type.")
#set(DEBUG_COMPILE_OPTIONS -g -ggdb -Wp,-U_FORTIFY_SOURCE CACHE STRING "Compilation flags for DEBUG build type.")
#set(CCOV_COMPILE_OPTIONS -g -O2 --coverage CACHE STRING "Compilation flags for CCOV build type.")
#set(RELEASE_COMPILE_OPTIONS -g -O2 CACHE STRING "Compilation flags for RELEASE build type.")

# Print a helper message when every thing is finished
# ----------------------------------------------------
if(IS_DIRECTORY $ENV{HOME}/opt/afb-monitoring)
set(MONITORING_ALIAS "--alias=/monitoring:$ENV{HOME}/opt/afb-monitoring")
endif()
set(CLOSING_MESSAGE "Debug from afb-daemon --port=1234 ${MONITORING_ALIAS} --ldpaths=. --workdir=. --roothttp=../htdocs --tracereq=common --token= --verbose ")
set(PACKAGE_MESSAGE "Install widget file using in the target : afm-util install ${PROJECT_NAME}.wgt")

# Optional location for config.xml.in
# -----------------------------------
#set(WIDGET_ICON conf.d/wgt/${PROJECT_ICON} CACHE PATH "Path to the widget icon")
#set(WIDGET_CONFIG_TEMPLATE ${CMAKE_CURRENT_SOURCE_DIR}/conf.d/wgt/config.xml.in CACHE PATH "Path to widget config file template (config.xml.in)")

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
