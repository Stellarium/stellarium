# - Try to find NOVA
# Once done this will define
#
#  NOVA_FOUND - system has NOVA
#  NOVA_INCLUDE_DIR - the NOVA include directory
#  NOVA_LIBRARIES - Link these to use NOVA

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)

  # in cache already
  set(NOVA_FOUND TRUE)
  message(STATUS "Found libnova: ${NOVA_LIBRARIES}")

else (NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)

  find_path(NOVA_INCLUDE_DIR libnova.h
    PATH_SUFFIXES libnova
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(NOVA_LIBRARIES NAMES nova libnova libnovad
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

 set(CMAKE_REQUIRED_INCLUDES ${NOVA_INCLUDE_DIR})
 set(CMAKE_REQUIRED_LIBRARIES ${NOVA_LIBRARIES})

   if(NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)
    set(NOVA_FOUND TRUE)
  else (NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)
    set(NOVA_FOUND FALSE)
  endif(NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)

  if (NOVA_FOUND)
    if (NOT Nova_FIND_QUIETLY)
      message(STATUS "Found NOVA: ${NOVA_LIBRARIES}")
    endif (NOT Nova_FIND_QUIETLY)
  else (NOVA_FOUND)
    if (Nova_FIND_REQUIRED)
      message(FATAL_ERROR "libnova not found. Please install libnova development package.")
    endif (Nova_FIND_REQUIRED)
  endif (NOVA_FOUND)

  mark_as_advanced(NOVA_INCLUDE_DIR NOVA_LIBRARIES)
  
endif (NOVA_INCLUDE_DIR AND NOVA_LIBRARIES)
