# - Try to find FFTW3
# Once done this will define
#
#  FFTW3_FOUND - system has FFTW3
#  FFTW3_INCLUDE_DIR - the FFTW3 include directory
#  FFTW3_LIBRARIES - Link these to use FFTW3
#  FFTW3_VERSION_STRING - Human readable version number of fftw3
#  FFTW3_VERSION_MAJOR  - Major version number of fftw3
#  FFTW3_VERSION_MINOR  - Minor version number of fftw3

# Copyright (c) 2017, Ilia Platone, <info@iliaplatone.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FFTW3_LIBRARIES)

  # in cache already
  set(FFTW3_FOUND TRUE)
  message(STATUS "Found FFTW3: ${FFTW3_LIBRARIES}")


else (FFTW3_LIBRARIES)

  find_library(FFTW3_LIBRARIES NAMES fftw3
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
    /usr/local/lib
  )

  if(FFTW3_LIBRARIES)
    set(FFTW3_FOUND TRUE)
  else (FFTW3_LIBRARIES)
    set(FFTW3_FOUND FALSE)
  endif(FFTW3_LIBRARIES)


  if (FFTW3_FOUND)
    if (NOT FFTW3_FIND_QUIETLY)
      message(STATUS "Found FFTW3: ${FFTW3_LIBRARIES}")
    endif (NOT FFTW3_FIND_QUIETLY)
  else (FFTW3_FOUND)
    if (FFTW3_FIND_REQUIRED)
      message(FATAL_ERROR "FFTW3 not found. Please install libfftw3-dev")
    endif (FFTW3_FIND_REQUIRED)
  endif (FFTW3_FOUND)

  mark_as_advanced(FFTW3_LIBRARIES)
  
endif (FFTW3_LIBRARIES)
