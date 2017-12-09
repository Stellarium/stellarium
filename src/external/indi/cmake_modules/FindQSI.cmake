# - Try to find Quantum Scientific Imaging Library
# Once done this will define
#
#  QSI_FOUND - system has QSI
#  QSI_INCLUDE_DIR - the QSI include directory
#  QSI_LIBRARIES - Link these to use QSI

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (QSI_INCLUDE_DIR AND QSI_LIBRARIES)

  # in cache already
  set(QSI_FOUND TRUE)
  message(STATUS "Found libqsiapi: ${QSI_LIBRARIES}")

else (QSI_INCLUDE_DIR AND QSI_LIBRARIES)

  find_path(QSI_INCLUDE_DIR qsiapi.h
    PATH_SUFFIXES qsiapi
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(QSI_LIBRARIES NAMES qsiapi
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(QSI_INCLUDE_DIR AND QSI_LIBRARIES)
    set(QSI_FOUND TRUE)
  else (QSI_INCLUDE_DIR AND QSI_LIBRARIES)
    set(QSI_FOUND FALSE)
  endif(QSI_INCLUDE_DIR AND QSI_LIBRARIES)


  if (QSI_FOUND)
    if (NOT QSI_FIND_QUIETLY)
      message(STATUS "Found QSI: ${QSI_LIBRARIES}")
    endif (NOT QSI_FIND_QUIETLY)
  else (QSI_FOUND)
    if (QSI_FIND_REQUIRED)
      message(FATAL_ERROR "QSI not found. Please install libqsi http://www.indilib.org")
    endif (QSI_FIND_REQUIRED)
  endif (QSI_FOUND)

  mark_as_advanced(QSI_INCLUDE_DIR QSI_LIBRARIES)
  
endif (QSI_INCLUDE_DIR AND QSI_LIBRARIES)
