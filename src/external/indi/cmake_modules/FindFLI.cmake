# - Try to find Finger Lakes Instruments Library
# Once done this will define
#
#  FLI_FOUND - system has FLI
#  FLI_INCLUDE_DIR - the FLI include directory
#  FLI_LIBRARIES - Link these to use FLI

# Copyright (c) 2008, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FLI_INCLUDE_DIR AND FLI_LIBRARIES)

  # in cache already
  set(FLI_FOUND TRUE)
  message(STATUS "Found libfli: ${FLI_LIBRARIES}")

else (FLI_INCLUDE_DIR AND FLI_LIBRARIES)

  find_path(FLI_INCLUDE_DIR libfli.h
    PATH_SUFFIXES fli
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(FLI_LIBRARIES NAMES fli
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(FLI_INCLUDE_DIR AND FLI_LIBRARIES)
    set(FLI_FOUND TRUE)
  else (FLI_INCLUDE_DIR AND FLI_LIBRARIES)
    set(FLI_FOUND FALSE)
  endif(FLI_INCLUDE_DIR AND FLI_LIBRARIES)


  if (FLI_FOUND)
    if (NOT FLI_FIND_QUIETLY)
      message(STATUS "Found FLI: ${FLI_LIBRARIES}")
    endif (NOT FLI_FIND_QUIETLY)
  else (FLI_FOUND)
    if (FLI_FIND_REQUIRED)
      message(FATAL_ERROR "FLI not found. Please install libfli-dev. http://www.indilib.org")
    endif (FLI_FIND_REQUIRED)
  endif (FLI_FOUND)

  mark_as_advanced(FLI_INCLUDE_DIR FLI_LIBRARIES)
  
endif (FLI_INCLUDE_DIR AND FLI_LIBRARIES)
