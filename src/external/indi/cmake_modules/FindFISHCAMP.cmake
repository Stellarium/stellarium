# - Try to find FISHCAMP CCD 
# Once done this will define
#
#  FISHCAMP_FOUND - system has FISHCAMP
#  FISHCAMP_LIBRARIES - Link these to use FISHCAMP
#  FISHCAMP_INCLUDE_DIR - Fishcamp include directory

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)

  # in cache already
  set(FISHCAMP_FOUND TRUE)
  message(STATUS "Found FISHCAMP: ${FISHCAMP_LIBRARIES}")

else (FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)

  find_library(FISHCAMP_LIBRARIES NAMES fishcamp    
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )
  
  find_path(FISHCAMP_INCLUDE_DIR fishcamp.h
    PATH_SUFFIXES libfishcamp
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

 set(CMAKE_REQUIRED_LIBRARIES ${FISHCAMP_LIBRARIES})

   if(FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)
    set(FISHCAMP_FOUND TRUE)
  else (FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)
    set(FISHCAMP_FOUND FALSE)
  endif(FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)

  if (FISHCAMP_FOUND)
    if (NOT FISHCAMP_FIND_QUIETLY)
      message(STATUS "Found FISHCAMP: ${FISHCAMP_LIBRARIES}")
    endif (NOT FISHCAMP_FIND_QUIETLY)
  else (FISHCAMP_FOUND)
    if (FISHCAMP_FIND_REQUIRED)
      message(FATAL_ERROR "FISHCAMP not found. Please install FISHCAMP library. http://www.indilib.org")
    endif (FISHCAMP_FIND_REQUIRED)
  endif (FISHCAMP_FOUND)

  mark_as_advanced(FISHCAMP_LIBRARIES FISHCAMP_INCLUDE_DIR)
  
endif (FISHCAMP_LIBRARIES AND FISHCAMP_INCLUDE_DIR)
