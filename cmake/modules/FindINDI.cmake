# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# This module can find INDI Library
#
# Requirements:
# - CMake >= 2.8.3 (for new version of find_package_handle_standard_args)
#
# The following variables will be defined for your use:
#   - INDI_FOUND             : were all of your specified components found (include dependencies)?
#   - INDI_INCLUDE_DIR       : INDI include directory
#   - INDI_DATA_DIR          : INDI include directory
#   - INDI_LIBRARIES         : INDI libraries
#   - INDI_DRIVER_LIBRARIES  : Same as above maintained for backward compatibility
#   - INDI_VERSION           : complete version of INDI (x.y.z)
#   - INDI_MAJOR_VERSION     : major version of INDI
#   - INDI_MINOR_VERSION     : minor version of INDI
#   - INDI_RELEASE_VERSION   : release version of INDI
#   - INDI_<COMPONENT>_FOUND : were <COMPONENT> found? (FALSE for non specified component if it is not a dependency)
#
# For windows or non standard installation, define INDI_ROOT variable to point to the root installation of INDI. Two ways:
#   - run cmake with -DINDI_ROOT=<PATH>
#   - define an environment variable with the same name before running cmake
# With cmake-gui, before pressing "Configure":
#   1) Press "Add Entry" button
#   2) Add a new entry defined as:
#     - Name: INDI_ROOT
#     - Type: choose PATH in the selection list
#     - Press "..." button and select the root installation of INDI
#
# Example Usage:
#
#   1. Copy this file in the root of your project source directory
#   2. Then, tell CMake to search this non-standard module in your project directory by adding to your CMakeLists.txt:
#     set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
#   3. Finally call find_package() once, here are some examples to pick from
#
#   Require INDI 1.4 or later
#     find_package(INDI 1.4 REQUIRED)
#
#   if(INDI_FOUND)
#      include_directories(${INDI_INCLUDE_DIR})
#      add_executable(myapp myapp.cpp)
#      target_link_libraries(myapp ${INDI_LIBRARIES})
#   endif(INDI_FOUND)
#
#
# Using Components:
#
# You can search for specific components. Currently, the following components are available
# * driver
# * align
# * client
# * clientqt5
#
# By default, if you do not specify any components, driver and align components are searched.
#
# Example:
#
# To use INDI Qt5 Client library only in your application:
#
# find_package(INDI CLIENTQT5 REQUIRED)
#
#   if(INDI_FOUND)
#      include_directories(${INDI_INCLUDE_DIR})
#      add_executable(myapp myapp.cpp)
#      target_link_libraries(myapp ${INDI_CLIENTQT5_LIBRARIES})
#   endif(INDI_FOUND)
#
#=============================================================================
# Copyright (c) 2011-2013, julp
# Copyright (c) 2017 Jasem Mutlaq
#
# Distributed under the OSI-approved BSD License
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTINDILAR PURPOSE.
#=============================================================================

find_package(PkgConfig QUIET)

########## Private ##########
if(NOT DEFINED INDI_PUBLIC_VAR_NS)
    set(INDI_PUBLIC_VAR_NS "INDI")                          # Prefix for all INDI relative public variables
endif(NOT DEFINED INDI_PUBLIC_VAR_NS)
if(NOT DEFINED INDI_PRIVATE_VAR_NS)
    set(INDI_PRIVATE_VAR_NS "_${INDI_PUBLIC_VAR_NS}")       # Prefix for all INDI relative internal variables
endif(NOT DEFINED INDI_PRIVATE_VAR_NS)
if(NOT DEFINED PC_INDI_PRIVATE_VAR_NS)
    set(PC_INDI_PRIVATE_VAR_NS "_PC${INDI_PRIVATE_VAR_NS}") # Prefix for all pkg-config relative internal variables
endif(NOT DEFINED PC_INDI_PRIVATE_VAR_NS)

function(indidebug _VARNAME)
    if(${INDI_PUBLIC_VAR_NS}_DEBUG)
        if(DEFINED ${INDI_PUBLIC_VAR_NS}_${_VARNAME})
            message("${INDI_PUBLIC_VAR_NS}_${_VARNAME} = ${${INDI_PUBLIC_VAR_NS}_${_VARNAME}}")
        else(DEFINED ${INDI_PUBLIC_VAR_NS}_${_VARNAME})
            message("${INDI_PUBLIC_VAR_NS}_${_VARNAME} = <UNDEFINED>")
        endif(DEFINED ${INDI_PUBLIC_VAR_NS}_${_VARNAME})
    endif(${INDI_PUBLIC_VAR_NS}_DEBUG)
endfunction(indidebug)

set(${INDI_PRIVATE_VAR_NS}_ROOT "")
if(DEFINED ENV{INDI_ROOT})
    set(${INDI_PRIVATE_VAR_NS}_ROOT "$ENV{INDI_ROOT}")
endif(DEFINED ENV{INDI_ROOT})
if (DEFINED INDI_ROOT)
    set(${INDI_PRIVATE_VAR_NS}_ROOT "${INDI_ROOT}")
endif(DEFINED INDI_ROOT)

set(${INDI_PRIVATE_VAR_NS}_BIN_SUFFIXES )
set(${INDI_PRIVATE_VAR_NS}_LIB_SUFFIXES )
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    list(APPEND ${INDI_PRIVATE_VAR_NS}_BIN_SUFFIXES "bin64")
    list(APPEND ${INDI_PRIVATE_VAR_NS}_LIB_SUFFIXES "lib64")
endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
list(APPEND ${INDI_PRIVATE_VAR_NS}_BIN_SUFFIXES "bin")
list(APPEND ${INDI_PRIVATE_VAR_NS}_LIB_SUFFIXES "lib")

set(${INDI_PRIVATE_VAR_NS}_COMPONENTS )
# <INDI component name> <library name 1> ... <library name N>
macro(INDI_declare_component _NAME)
    list(APPEND ${INDI_PRIVATE_VAR_NS}_COMPONENTS ${_NAME})
    set("${INDI_PRIVATE_VAR_NS}_COMPONENTS_${_NAME}" ${ARGN})
endmacro(INDI_declare_component)

INDI_declare_component(driver  indidriver)
INDI_declare_component(align   indiAlignmentDriver)
INDI_declare_component(client  indiclient)
INDI_declare_component(clientqt5 indiclientqt5)

########## Public ##########
set(${INDI_PUBLIC_VAR_NS}_FOUND TRUE)
set(${INDI_PUBLIC_VAR_NS}_LIBRARIES )
set(${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR )
foreach(${INDI_PRIVATE_VAR_NS}_COMPONENT ${${INDI_PRIVATE_VAR_NS}_COMPONENTS})
    string(TOUPPER "${${INDI_PRIVATE_VAR_NS}_COMPONENT}" ${INDI_PRIVATE_VAR_NS}_UPPER_COMPONENT)
    set("${INDI_PUBLIC_VAR_NS}_${${INDI_PRIVATE_VAR_NS}_UPPER_COMPONENT}_FOUND" FALSE) # may be done in the INDI_declare_component macro
endforeach(${INDI_PRIVATE_VAR_NS}_COMPONENT)

# Check components
if(NOT ${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS) # driver and posix client by default
    set(${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS driver align)
else(NOT ${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS)
    #list(APPEND ${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS uc)
    list(REMOVE_DUPLICATES ${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS)
    foreach(${INDI_PRIVATE_VAR_NS}_COMPONENT ${${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS})
        if(NOT DEFINED ${INDI_PRIVATE_VAR_NS}_COMPONENTS_${${INDI_PRIVATE_VAR_NS}_COMPONENT})
            message(FATAL_ERROR "Unknown INDI component: ${${INDI_PRIVATE_VAR_NS}_COMPONENT}")
        endif(NOT DEFINED ${INDI_PRIVATE_VAR_NS}_COMPONENTS_${${INDI_PRIVATE_VAR_NS}_COMPONENT})
    endforeach(${INDI_PRIVATE_VAR_NS}_COMPONENT)
endif(NOT ${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS)

# Includes
find_path(
    ${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR
    indidevapi.h
    PATH_SUFFIXES libindi
    ${PC_INDI_INCLUDE_DIR}
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
    HINTS ${${INDI_PRIVATE_VAR_NS}_ROOT}
    DOC "Include directory for INDI"
)

find_path(${INDI_PUBLIC_VAR_NS}_DATA_DIR
    drivers.xml
    PATH_SUFFIXES share/indi
    DOC "Data directory for INDI"
    )

if(${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR)
    if(EXISTS "${${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR}/indiversion.h") # INDI >= 1.4
        file(READ "${${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR}/indiversion.h" ${INDI_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS)
    else()
        message(FATAL_ERROR "INDI version header not found")
    endif()

    if(${INDI_PRIVATE_VAR_NS}_VERSION_HEADER_CONTENTS MATCHES ".*INDI_VERSION ([0-9]+).([0-9]+).([0-9]+)")
            set(${INDI_PUBLIC_VAR_NS}_MAJOR_VERSION "${CMAKE_MATCH_1}")
            set(${INDI_PUBLIC_VAR_NS}_MINOR_VERSION "${CMAKE_MATCH_2}")
            set(${INDI_PUBLIC_VAR_NS}_RELEASE_VERSION "${CMAKE_MATCH_3}")
    else()
        message(FATAL_ERROR "failed to detect INDI version")
    endif()
    set(${INDI_PUBLIC_VAR_NS}_VERSION "${${INDI_PUBLIC_VAR_NS}_MAJOR_VERSION}.${${INDI_PUBLIC_VAR_NS}_MINOR_VERSION}.${${INDI_PUBLIC_VAR_NS}_RELEASE_VERSION}")

    # Check libraries
    foreach(${INDI_PRIVATE_VAR_NS}_COMPONENT ${${INDI_PUBLIC_VAR_NS}_FIND_COMPONENTS})
        set(${INDI_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES )
        set(${INDI_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES )
        foreach(${INDI_PRIVATE_VAR_NS}_BASE_NAME ${${INDI_PRIVATE_VAR_NS}_COMPONENTS_${${INDI_PRIVATE_VAR_NS}_COMPONENT}})
            list(APPEND ${INDI_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES "${${INDI_PRIVATE_VAR_NS}_BASE_NAME}")
            list(APPEND ${INDI_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES "${${INDI_PRIVATE_VAR_NS}_BASE_NAME}d")
            list(APPEND ${INDI_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES "${${INDI_PRIVATE_VAR_NS}_BASE_NAME}${INDI_MAJOR_VERSION}${INDI_MINOR_VERSION}")
            list(APPEND ${INDI_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES "${${INDI_PRIVATE_VAR_NS}_BASE_NAME}${INDI_MAJOR_VERSION}${INDI_MINOR_VERSION}d")
        endforeach(${INDI_PRIVATE_VAR_NS}_BASE_NAME)

        find_library(
            ${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT}
            NAMES ${${INDI_PRIVATE_VAR_NS}_POSSIBLE_RELEASE_NAMES}
            HINTS ${${INDI_PRIVATE_VAR_NS}_ROOT}
            PATH_SUFFIXES ${_INDI_LIB_SUFFIXES}
            DOC "Release libraries for INDI"
        )
        find_library(
            ${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}
            NAMES ${${INDI_PRIVATE_VAR_NS}_POSSIBLE_DEBUG_NAMES}
            HINTS ${${INDI_PRIVATE_VAR_NS}_ROOT}
            PATH_SUFFIXES ${_INDI_LIB_SUFFIXES}
            DOC "Debug libraries for INDI"
        )

        string(TOUPPER "${${INDI_PRIVATE_VAR_NS}_COMPONENT}" ${INDI_PRIVATE_VAR_NS}_UPPER_COMPONENT)
        if(NOT ${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT} AND NOT ${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}) # both not found
            set("${INDI_PUBLIC_VAR_NS}_${${INDI_PRIVATE_VAR_NS}_UPPER_COMPONENT}_FOUND" FALSE)
            set("${INDI_PUBLIC_VAR_NS}_FOUND" FALSE)
        else(NOT ${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT} AND NOT ${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}) # one or both found
            set("${INDI_PUBLIC_VAR_NS}_${${INDI_PRIVATE_VAR_NS}_UPPER_COMPONENT}_FOUND" TRUE)
            if(NOT ${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT}) # release not found => we are in debug
                set(${INDI_PRIVATE_VAR_NS}_LIB_${${INDI_PRIVATE_VAR_NS}_COMPONENT} "${${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}}")
            elseif(NOT ${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}) # debug not found => we are in release
                set(${INDI_PRIVATE_VAR_NS}_LIB_${${INDI_PRIVATE_VAR_NS}_COMPONENT} "${${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT}}")
            else() # both found
                set(
                    ${INDI_PRIVATE_VAR_NS}_LIB_${${INDI_PRIVATE_VAR_NS}_COMPONENT}
                    optimized ${${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT}}
                    debug ${${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT}}
                )
            endif()
            list(APPEND ${INDI_PUBLIC_VAR_NS}_LIBRARIES ${${INDI_PRIVATE_VAR_NS}_LIB_${${INDI_PRIVATE_VAR_NS}_COMPONENT}})
        endif(NOT ${INDI_PRIVATE_VAR_NS}_LIB_RELEASE_${${INDI_PRIVATE_VAR_NS}_COMPONENT} AND NOT ${INDI_PRIVATE_VAR_NS}_LIB_DEBUG_${${INDI_PRIVATE_VAR_NS}_COMPONENT})
    endforeach(${INDI_PRIVATE_VAR_NS}_COMPONENT)

    # Check find_package arguments
    include(FindPackageHandleStandardArgs)
    if(${INDI_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${INDI_PUBLIC_VAR_NS}_FIND_QUIETLY)
        find_package_handle_standard_args(
            ${INDI_PUBLIC_VAR_NS}
            REQUIRED_VARS ${INDI_PUBLIC_VAR_NS}_LIBRARIES ${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR
            VERSION_VAR ${INDI_PUBLIC_VAR_NS}_VERSION
        )
    else(${INDI_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${INDI_PUBLIC_VAR_NS}_FIND_QUIETLY)
        find_package_handle_standard_args(${INDI_PUBLIC_VAR_NS} "INDI not found" ${INDI_PUBLIC_VAR_NS}_LIBRARIES ${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR)
    endif(${INDI_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${INDI_PUBLIC_VAR_NS}_FIND_QUIETLY)
else(${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR)
    set("${INDI_PUBLIC_VAR_NS}_FOUND" FALSE)
    if(${INDI_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${INDI_PUBLIC_VAR_NS}_FIND_QUIETLY)
        message(FATAL_ERROR "Could not find INDI include directory")
    endif(${INDI_PUBLIC_VAR_NS}_FIND_REQUIRED AND NOT ${INDI_PUBLIC_VAR_NS}_FIND_QUIETLY)
endif(${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR)

mark_as_advanced(
    ${INDI_PUBLIC_VAR_NS}_INCLUDE_DIR
    ${INDI_PUBLIC_VAR_NS}_LIBRARIES
)

# IN (args)
indidebug("FIND_COMPONENTS")
indidebug("FIND_REQUIRED")
indidebug("FIND_QUIETLY")
indidebug("FIND_VERSION")
# OUT
# Found
indidebug("FOUND")
indidebug("SERVER_FOUND")
indidebug("DRIVERS_FOUND")
indidebug("CLIENT_FOUND")
indidebug("QT5CLIENT_FOUND")

# Linking
indidebug("INCLUDE_DIR")
indidebug("DATA_DIR")
indidebug("LIBRARIES")
# Backward compatibility
set(${INDI_PUBLIC_VAR_NS}_DRIVER_LIBRARIES ${${INDI_PUBLIC_VAR_NS}_LIBRARIES})
indidebug("DRIVER_LIBRARIES")
# Version
indidebug("MAJOR_VERSION")
indidebug("MINOR_VERSION")
indidebug("RELEASE_VERSION")
indidebug("VERSION")
