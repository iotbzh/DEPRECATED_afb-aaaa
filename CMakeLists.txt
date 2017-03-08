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


# Select GCC5 is avaliable
###########################################################################
execute_process(COMMAND gcc-5 -dumpversion RESULT_VARIABLE GCC5RC OUTPUT_QUIET ERROR_QUIET)
if(GCC5RC EQUAL 0)
message(STATUS "GCC version-5 selected")
set(CMAKE_C_COMPILER "gcc-5")
set(CMAKE_CXX_COMPILER "g++-5")
endif(GCC5RC EQUAL 0)


# Compiler should be selected before projet()
project(audio-binding)
SET(PROJECT_NAME "AFB Audio Binding")
SET(PROJECT_PRETTY_NAME "Audio Binding")
SET(PROJECT_DESCRIPTION "Expose Audio API through AGL Framework")
SET(PROJECT_VERSION "1.0")
SET(PROJECT_URL "https://github.com/iotbzh/afb-audio")

CMAKE_MINIMUM_REQUIRED(VERSION 3.3)
SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_POSITION_INDEPENDENT_CODE ON)

INCLUDE(FindPkgConfig)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckLibraryExists)
INCLUDE(GNUInstallDirs)


SET(binding_install_dir ${CMAKE_INSTALL_FULL_LIBDIR}/audio)

# Generic useful macro
###########################################################
macro(defstr name value)
	add_definitions(-D${name}=${value})
endmacro(defstr)

macro(setc name value)
	if(NOT DEFINED ${name})
		set(${name} ${value})
	endif(NOT DEFINED ${name})
endmacro(setc)

# Default compilation options
############################################################################
link_libraries(-Wl,--as-needed -Wl,--gc-sections)
add_compile_options(-Wall -Wextra -Wconversion)
add_compile_options(-Wno-unused-parameter) # frankly not using a parameter does it care?
add_compile_options(-Wno-sign-compare -Wno-sign-conversion)
add_compile_options(-Werror=maybe-uninitialized)
add_compile_options(-Werror=implicit-function-declaration)
add_compile_options(-ffunction-sections -fdata-sections)
add_compile_options(-fPIC)
add_compile_options(-g)

setc(CMAKE_C_FLAGS_PROFILING   "-g -O2 -pg -Wp,-U_FORTIFY_SOURCE")
setc(CMAKE_C_FLAGS_DEBUG       "-g -O2 -ggdb -Wp,-U_FORTIFY_SOURCE")
setc(CMAKE_C_FLAGS_RELEASE     "-g -O2")
setc(CMAKE_C_FLAGS_CCOV        "-g -O2 --coverage")
setc(CMAKE_INSTALL_PREFIX      "${CMAKE_SOURCE_DIR}/Install")

# Warning as PKG_CONFIG_PATH does not work [should be en env variable]
set(PKG_CONFIG_USE_CMAKE_PREFIX_PATH 1)
set(CMAKE_PREFIX_PATH ${HOME}/opt/lib64/pkgconfig)


# Generic Package dependencies
############################################################################
PKG_CHECK_MODULES(json-c REQUIRED json-c)
PKG_CHECK_MODULES(afb-daemon REQUIRED afb-daemon)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  CHECK_LIBRARY_EXISTS(efence malloc "" HAVE_LIBEFENCE)
  IF(HAVE_LIBEFENCE)
    MESSAGE(STATUS "Linking with ElectricFence for debugging purposes...")
    SET(libefence_LIBRARIES "-lefence")
  ENDIF(HAVE_LIBEFENCE)
ENDIF(CMAKE_BUILD_TYPE MATCHES Debug)


SET(include_dirs
	${INCLUDE_DIRS}
	${CMAKE_SOURCE_DIR}/AlsaSound/include
	${json-c_INCLUDE_DIRS}
	${afb-daemon_INCLUDE_DIRS}
	)

SET(link_libraries
	${libefence_LIBRARIES}
	${json-c_LIBRARIES}
	)

# Bindings to compile
# --------------------
add_subdirectory(AlsaSound)
add_subdirectory(AudioLogic)
add_subdirectory(htdocs)
