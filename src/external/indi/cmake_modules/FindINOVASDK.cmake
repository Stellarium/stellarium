# - Try to find INOVASDK Universal Library
# Once done this will define
#
#  INOVASDK_FOUND - system has INOVASDK
#  INOVASDK_INCLUDE_DIR - the INOVASDK include directory
#  INOVASDK_LIBRARIES - Link these to use INOVASDK

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)

  # in cache already
  set(INOVASDK_FOUND TRUE)
  message(STATUS "Found libinovasdk: ${INOVASDK_LIBRARIES}")

else (INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)

  find_path(INOVASDK_INCLUDE_DIR inovasdk.h
    PATH_SUFFIXES inovasdk
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(INOVASDK_LIBRARIES NAMES inovasdk
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)
    set(INOVASDK_FOUND TRUE)
  else (INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)
    set(INOVASDK_FOUND FALSE)
  endif(INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)


  if (INOVASDK_FOUND)
    if (NOT INOVASDK_FIND_QUIETLY)
      message(STATUS "Found INOVASDK: ${INOVASDK_LIBRARIES}")
    endif (NOT INOVASDK_FIND_QUIETLY)
  else (INOVASDK_FOUND)
    if (INOVASDK_FIND_REQUIRED)
      message(FATAL_ERROR "INOVASDK not found. Please install INOVASDK Library http://www.indilib.org")
    endif (INOVASDK_FIND_REQUIRED)
  endif (INOVASDK_FOUND)

  mark_as_advanced(INOVASDK_INCLUDE_DIR INOVASDK_LIBRARIES)
  
endif (INOVASDK_INCLUDE_DIR AND INOVASDK_LIBRARIES)
