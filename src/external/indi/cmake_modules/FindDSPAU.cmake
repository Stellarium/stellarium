# - Try to find Digital Signal Processing for Astronomical Usage
# Once done this will define
#
#  DSPAU_FOUND - system has DSPAU
#  DSPAU_INCLUDE_DIR - the DSPAU include directory
#  DSPAU_LIBRARIES - Link these to use DSPAU

# Copyright (c) 2008, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)

  # in cache already
  set(DSPAU_FOUND TRUE)
  message(STATUS "Found libdspau: ${DSPAU_LIBRARIES}")

else (DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)

  find_path(DSPAU_INCLUDE_DIR libdspau.h
    PATH_SUFFIXES dspau
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(DSPAU_LIBRARIES NAMES dspau
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)
    set(DSPAU_FOUND TRUE)
  else (DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)
    set(DSPAU_FOUND FALSE)
  endif(DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)


  if (DSPAU_FOUND)
    if (NOT DSPAU_FIND_QUIETLY)
      message(STATUS "Found DSPAU: ${DSPAU_LIBRARIES}")
    endif (NOT DSPAU_FIND_QUIETLY)
  else (DSPAU_FOUND)
    if (DSPAU_FIND_REQUIRED)
      message(FATAL_ERROR "DSPAU not found. Please install libdspau-dev. http://www.indilib.org")
    endif (DSPAU_FIND_REQUIRED)
  endif (DSPAU_FOUND)

  mark_as_advanced(DSPAU_INCLUDE_DIR DSPAU_LIBRARIES)
  
endif (DSPAU_INCLUDE_DIR AND DSPAU_LIBRARIES)
