# - Try to find libaiousb
# Once done this will define
#
#  AIOUSB_FOUND - system has AIOUSB
#  AIOUSB_INCLUDE_DIR - the AIOUSB include directory
#  AIOUSB_LIBRARIES - Link these to use AIOUSB (C)
#  AIOUSB_CPP_LIBRARIES - Link these to use AIOUSB (C++)

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)

  # in cache already
  set(AIOUSB_FOUND TRUE)
  message(STATUS "Found libaiusb: ${AIOUSB_LIBRARIES}")
  message(STATUS "Found libaiusbcpp: ${AIOUSB_CPP_LIBRARIES}")

else (AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)

  find_path(AIOUSB_INCLUDE_DIR aiousb.h
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(AIOUSB_LIBRARIES NAMES aiousb
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

    find_library(AIOUSB_CPP_LIBRARIES NAMES aiousbcpp
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )
  
   if(AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)
    set(AIOUSB_FOUND TRUE)
  else (AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)
    set(AIOUSB_FOUND FALSE)
  endif(AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)

  if (AIOUSB_FOUND)
    if (NOT AIOUSB_FIND_QUIETLY)
      message(STATUS "Found libaiousb: ${AIOUSB_LIBRARIES}")
      message(STATUS "Found libaiusbcpp: ${AIOUSB_CPP_LIBRARIES}")
    endif (NOT AIOUSB_FIND_QUIETLY)
  else (AIOUSB_FOUND)
    if (AIOUSB_FIND_REQUIRED)
      message(FATAL_ERROR "libaiousb not found. Please install libaiousb. https://www.accesio.com")
    endif (AIOUSB_FIND_REQUIRED)
  endif (AIOUSB_FOUND)

  mark_as_advanced(AIOUSB_INCLUDE_DIR AIOUSB_LIBRARIES AIOUSB_CPP_LIBRARIES)
  
endif (AIOUSB_INCLUDE_DIR AND AIOUSB_LIBRARIES AND AIOUSB_CPP_LIBRARIES)

