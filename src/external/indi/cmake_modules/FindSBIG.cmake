# - Try to find SBIG Universal Library
# Once done this will define
#
#  SBIG_FOUND - system has SBIG
#  SBIG_INCLUDE_DIR - the SBIG include directory
#  SBIG_LIBRARIES - Link these to use SBIG

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)

  # in cache already
  set(SBIG_FOUND TRUE)
  message(STATUS "Found libsbig: ${SBIG_LIBRARIES}")

else (SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)

  find_path(SBIG_INCLUDE_DIR sbigudrv.h
    PATH_SUFFIXES libsbig
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(SBIG_LIBRARIES NAMES sbigudrv
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

  if(SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)
    set(SBIG_FOUND TRUE)
  else (SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)
    set(SBIG_FOUND FALSE)
  endif(SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)


  if (SBIG_FOUND)
    if (NOT SBIG_FIND_QUIETLY)
      message(STATUS "Found SBIG: ${SBIG_LIBRARIES}")
    endif (NOT SBIG_FIND_QUIETLY)
  else (SBIG_FOUND)
    if (SBIG_FIND_REQUIRED)
      message(FATAL_ERROR "SBIG not found. Please install SBIG Library http://www.indilib.org")
    endif (SBIG_FIND_REQUIRED)
  endif (SBIG_FOUND)

  mark_as_advanced(SBIG_INCLUDE_DIR SBIG_LIBRARIES)
  
endif (SBIG_INCLUDE_DIR AND SBIG_LIBRARIES)
