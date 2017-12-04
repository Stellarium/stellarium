# - Try to find ASI Library
# Once done this will define
#
#  ASI_FOUND - system has ASI
#  ASI_INCLUDE_DIR - the ASI include directory
#  ASI_LIBRARIES - Link these to use ASI

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (ASI_INCLUDE_DIR AND ASI_LIBRARIES)

  # in cache already
  set(ASI_FOUND TRUE)
  message(STATUS "Found libasi: ${ASI_LIBRARIES}")

else (ASI_INCLUDE_DIR AND ASI_LIBRARIES)

  find_path(ASI_INCLUDE_DIR ASICamera2.h
    PATH_SUFFIXES libasi
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(ASI_LIBRARIES NAMES ASICamera2
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(ASI_INCLUDE_DIR AND ASI_LIBRARIES)
    set(ASI_FOUND TRUE)
  else (ASI_INCLUDE_DIR AND ASI_LIBRARIES)
    set(ASI_FOUND FALSE)
  endif(ASI_INCLUDE_DIR AND ASI_LIBRARIES)


  if (ASI_FOUND)
    if (NOT ASI_FIND_QUIETLY)
      message(STATUS "Found ASI: ${ASI_LIBRARIES}")
    endif (NOT ASI_FIND_QUIETLY)
  else (ASI_FOUND)
    if (ASI_FIND_REQUIRED)
      message(FATAL_ERROR "ASI not found. Please install libasi http://www.indilib.org")
    endif (ASI_FIND_REQUIRED)
  endif (ASI_FOUND)

  mark_as_advanced(ASI_INCLUDE_DIR ASI_LIBRARIES)
  
endif (ASI_INCLUDE_DIR AND ASI_LIBRARIES)
