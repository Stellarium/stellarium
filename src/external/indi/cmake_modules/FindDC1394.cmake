# - Try to find dc1394 library (version 2) and include files
# Once done this will define
#
#  DC1394_FOUND - system has DC1394
#  DC1394_INCLUDE_DIR - the DC1394 include directory
#  DC1394_LIBRARIES - Link these to use DC1394

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)

  # in cache already
  set(DC1394_FOUND TRUE)
  message(STATUS "Found libdc1394: ${DC1394_LIBRARIES}")

else (DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)

  find_path(DC1394_INCLUDE_DIR control.h
    PATH_SUFFIXES dc1394
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(DC1394_LIBRARIES NAMES dc1394
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)
    set(DC1394_FOUND TRUE)
  else (DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)
    set(DC1394_FOUND FALSE)
  endif(DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)


  if (DC1394_FOUND)
    if (NOT DC1394_FIND_QUIETLY)
      message(STATUS "Found DC1394: ${DC1394_LIBRARIES}")
    endif (NOT DC1394_FIND_QUIETLY)
  else (DC1394_FOUND)
    if (DC1394_FIND_REQUIRED)
      message(FATAL_ERROR "DC1394 not found. Please install libdc1395")
    endif (DC1394_FIND_REQUIRED)
  endif (DC1394_FOUND)

  mark_as_advanced(DC1394_INCLUDE_DIR DC1394_LIBRARIES)

endif (DC1394_INCLUDE_DIR AND DC1394_LIBRARIES)
