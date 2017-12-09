# - Try to find QHY Library
# Once done this will define
#
#  QHY_FOUND - system has QHY
#  QHY_INCLUDE_DIR - the QHY include directory
#  QHY_LIBRARIES - Link these to use QHY

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (QHY_INCLUDE_DIR AND QHY_LIBRARIES)

  # in cache already
  set(QHY_FOUND TRUE)
  message(STATUS "Found libqhy: ${QHY_LIBRARIES}")

else (QHY_INCLUDE_DIR AND QHY_LIBRARIES)

  find_path(QHY_INCLUDE_DIR qhyccd.h
    PATH_SUFFIXES libqhy
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(QHY_LIBRARIES NAMES qhy
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(QHY_INCLUDE_DIR AND QHY_LIBRARIES)
    set(QHY_FOUND TRUE)
  else (QHY_INCLUDE_DIR AND QHY_LIBRARIES)
    set(QHY_FOUND FALSE)
  endif(QHY_INCLUDE_DIR AND QHY_LIBRARIES)


  if (QHY_FOUND)
    if (NOT QHY_FIND_QUIETLY)
      message(STATUS "Found QHY: ${QHY_LIBRARIES}")
    endif (NOT QHY_FIND_QUIETLY)
  else (QHY_FOUND)
    if (QHY_FIND_REQUIRED)
      message(FATAL_ERROR "QHY not found. Please install libqhy http://www.indilib.org")
    endif (QHY_FIND_REQUIRED)
  endif (QHY_FOUND)

  mark_as_advanced(QHY_INCLUDE_DIR QHY_LIBRARIES)
  
endif (QHY_INCLUDE_DIR AND QHY_LIBRARIES)
