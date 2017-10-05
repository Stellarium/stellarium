# - Try to find Iconv 
# Once done this will define 
# 
#  ICONV_FOUND - system has Iconv 
#  ICONV_INCLUDE_DIR - the Iconv include directory 
#  ICONV_LIBRARIES - Link these to use Iconv 
#  ICONV_SECOND_ARGUMENT_IS_CONST - the second argument for iconv() is const
# 
# As of 7/27/2012 the version of libiconv.2.dyld in /usr/lib is incompatible with libintl.8.dylib in /opt/local/lib
# HINTS are used before looking in the usual places...
#
# =====================================================================
# Copyright 2007 Fabien Chereau et. al.
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# * Redistributions of source code must retain the above copyright
#   notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
#
# * Neither the names of Kitware, Inc., the Insight Software Consortium,
#   nor the names of their contributors may be used to endorse or promote
#   products derived from this software without specific prior written
#   permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

include(CheckCXXSourceCompiles)

IF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)
  # Already in cache, be silent
  SET(ICONV_FIND_QUIETLY TRUE)
ENDIF (ICONV_INCLUDE_DIR AND ICONV_LIBRARIES)

IF(APPLE)
    FIND_PATH(ICONV_INCLUDE_DIR iconv.h 
        HINTS
        /opt/local/include
        /sw/include
    )
ELSE(NOT APPLE)
    FIND_PATH(ICONV_INCLUDE_DIR iconv.h)
ENDIF(APPLE)

IF(APPLE)
    FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv
        HINTS
        /opt/local/lib
        /sw/include
    ) 
ELSE(NOT APPLE)
    FIND_LIBRARY(ICONV_LIBRARIES NAMES iconv c)
ENDIF(APPLE)

IF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 
   SET(ICONV_FOUND TRUE) 
ENDIF(ICONV_INCLUDE_DIR AND ICONV_LIBRARIES) 

set(CMAKE_REQUIRED_INCLUDES ${ICONV_INCLUDE_DIR})
set(CMAKE_REQUIRED_LIBRARIES ${ICONV_LIBRARIES})
IF(ICONV_FOUND)
  check_cxx_source_compiles("
  #include <iconv.h>
  int main(){
    iconv_t conv = 0;
    const char* in = 0;
    size_t ilen = 0;
    char* out = 0;
    size_t olen = 0;
    iconv(conv, &in, &ilen, &out, &olen);
    return 0;
  }
" ICONV_SECOND_ARGUMENT_IS_CONST )
ENDIF(ICONV_FOUND)
set(CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_LIBRARIES)

IF(ICONV_FOUND) 
  IF(NOT ICONV_FIND_QUIETLY) 
    MESSAGE(STATUS "Found Iconv: ${ICONV_LIBRARIES}") 
  ENDIF(NOT ICONV_FIND_QUIETLY) 
ELSE(ICONV_FOUND) 
  IF(Iconv_FIND_REQUIRED) 
    MESSAGE(FATAL_ERROR "Could not find Iconv") 
  ENDIF(Iconv_FIND_REQUIRED) 
ENDIF(ICONV_FOUND) 

MARK_AS_ADVANCED(
  ICONV_INCLUDE_DIR
  ICONV_LIBRARIES
  ICONV_SECOND_ARGUMENT_IS_CONST
)
