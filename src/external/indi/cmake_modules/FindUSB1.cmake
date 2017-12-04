# - Try to find libusb-1.0
# Once done this will define
#
#  USB1_FOUND - system has libusb-1.0
#  USB1_INCLUDE_DIRS - the libusb-1.0 include directories
#  USB1_LIBRARIES - Link these to use libusb-1.0
#  USB1_DEFINITIONS - Compiler switches required for using libusb-1.0
#
#  USB1_HAS_LIBUSB_ERROR_NAME - defined when libusb-1.0 has libusb_error_name()

#=============================================================================
# Copyright (c) 2017 Pino Toscano <toscano.pino@tiscali.it>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#=============================================================================

find_package(PkgConfig)
pkg_check_modules(PC_LIBUSB1 QUIET libusb-1.0)

find_path(USB1_INCLUDE_DIR
  NAMES
    libusb.h
  HINTS
    ${PC_LIBUSB1_INCLUDE_DIRS}
  PATH_SUFFIXES
    libusb-1.0
)

find_library(USB1_LIBRARY
  NAMES
    ${PC_LIBUSB1_LIBRARIES}
    usb-1.0
  HINTS
    ${PC_LIBUSB1_LIBRARY_DIRS}
)

set(USB1_INCLUDE_DIRS ${USB1_INCLUDE_DIR})
set(USB1_LIBRARIES ${USB1_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(USB1
  FOUND_VAR
    USB1_FOUND
  REQUIRED_VARS
    USB1_LIBRARY
    USB1_INCLUDE_DIR
  VERSION_VAR
    PC_LIBUSB1_VERSION
)

mark_as_advanced(USB1_INCLUDE_DIRS USB1_LIBRARIES)

if(USB1_FOUND)
  include(CheckCXXSourceCompiles)
  include(CMakePushCheckState)
  cmake_push_check_state(RESET)
  set(CMAKE_REQUIRED_INCLUDES ${USB1_INCLUDE_DIRS})
  set(CMAKE_REQUIRED_LIBRARIES ${USB1_LIBRARIES})
  check_cxx_source_compiles("#include <libusb.h> 
    int main() { libusb_error_name(0); return 0; }" USB1_HAS_LIBUSB_ERROR_NAME)
  cmake_pop_check_state()
endif()
