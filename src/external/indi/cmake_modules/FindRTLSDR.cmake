# - Try to find RTLSDR
# Once done this will define
#
#  RTLSDR_FOUND - system has RTLSDR
#  RTLSDR_INCLUDE_DIR - the RTLSDR include directory
#  RTLSDR_LIBRARIES - Link these to use RTLSDR
#  RTLSDR_VERSION_STRING - Human readable version number of rtlsdr
#  RTLSDR_VERSION_MAJOR  - Major version number of rtlsdr
#  RTLSDR_VERSION_MINOR  - Minor version number of rtlsdr

# Copyright (c) 2017, Ilia Platone, <info@iliaplatone.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (RTLSDR_LIBRARIES)

  # in cache already
  set(RTLSDR_FOUND TRUE)
  message(STATUS "Found RTLSDR: ${RTLSDR_LIBRARIES}")


else (RTLSDR_LIBRARIES)

  find_library(RTLSDR_LIBRARIES NAMES rtlsdr rtlsdr0
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
    /usr/local/lib
  )

  if(RTLSDR_LIBRARIES)
    set(RTLSDR_FOUND TRUE)
  else (RTLSDR_LIBRARIES)
    set(RTLSDR_FOUND FALSE)
  endif(RTLSDR_LIBRARIES)


  if (RTLSDR_FOUND)
    if (NOT RTLSDR_FIND_QUIETLY)
      message(STATUS "Found RTLSDR: ${RTLSDR_LIBRARIES}")
    endif (NOT RTLSDR_FIND_QUIETLY)
  else (RTLSDR_FOUND)
    if (RTLSDR_FIND_REQUIRED)
      message(FATAL_ERROR "RTLSDR not found. Please install librtlsdr-dev")
    endif (RTLSDR_FIND_REQUIRED)
  endif (RTLSDR_FOUND)

  mark_as_advanced(RTLSDR_LIBRARIES)
  
endif (RTLSDR_LIBRARIES)
