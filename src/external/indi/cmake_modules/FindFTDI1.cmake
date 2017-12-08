# - Try to find FTDI1
# Once done this will define
#
#  FTDI1_FOUND - system has FTDI
#  FTDI1_INCLUDE_DIR - the FTDI include directory
#  FTDI1_LIBRARIES - Link these to use FTDI
#
# N.B. You must include the file as following:
#
#include <libftdi1/ftdi.h>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)

  # in cache already
  set(FTDI1_FOUND TRUE)
  message(STATUS "Found libftdi1: ${FTDI1_LIBRARIES}")

else (FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)

  find_path(FTDI1_INCLUDE_DIR ftdi.h
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
    /usr/local/include
  )

  find_library(FTDI1_LIBRARIES NAMES ftdi1
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
    /usr/local/lib
  )

  if(FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)
    set(FTDI1_FOUND TRUE)
  else (FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)
    set(FTDI1_FOUND FALSE)
  endif(FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)


  if (FTDI1_FOUND)
    if (NOT FTDI1_FIND_QUIETLY)
      message(STATUS "Found FTDI1: ${FTDI1_LIBRARIES}")
    endif (NOT FTDI1_FIND_QUIETLY)
  else (FTDI1_FOUND)
    if (FTDI1_FIND_REQUIRED)
      message(FATAL_ERROR "FTDI not found. Please install libftdi1-dev")
    endif (FTDI1_FIND_REQUIRED)
  endif (FTDI1_FOUND)

  mark_as_advanced(FTDI1_INCLUDE_DIR FTDI1_LIBRARIES)
  
endif (FTDI1_INCLUDE_DIR AND FTDI1_LIBRARIES)
