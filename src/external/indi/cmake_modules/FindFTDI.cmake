# - Try to find FTDI
# This finds libFTDI that is compatible with old libusb v 0.1
# For newer libusb > 1.0, use FindFTDI1.cmake
# Once done this will define
#
#  FTDI_FOUND - system has FTDI
#  FTDI_INCLUDE_DIR - the FTDI include directory
#  FTDI_LIBRARIES - Link these to use FTDI

# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)

  # in cache already
  set(FTDI_FOUND TRUE)
  message(STATUS "Found libftdi: ${FTDI_LIBRARIES}")

else (FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)

  find_path(FTDI_INCLUDE_DIR ftdi.h
    PATH_SUFFIXES libftdi1
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
    /usr/local/include
  )

  find_library(FTDI_LIBRARIES NAMES ftdi ftdi1
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
    /usr/local/lib
  )

  if(FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)
    set(FTDI_FOUND TRUE)
  else (FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)
    set(FTDI_FOUND FALSE)
  endif(FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)


  if (FTDI_FOUND)
    if (NOT FTDI_FIND_QUIETLY)
      message(STATUS "Found FTDI: ${FTDI_LIBRARIES}")
    endif (NOT FTDI_FIND_QUIETLY)
  else (FTDI_FOUND)
    if (FTDI_FIND_REQUIRED)
      message(FATAL_ERROR "FTDI not found. Please install libftdi-dev")
    endif (FTDI_FIND_REQUIRED)
  endif (FTDI_FOUND)

  mark_as_advanced(FTDI_INCLUDE_DIR FTDI_LIBRARIES)
  
endif (FTDI_INCLUDE_DIR AND FTDI_LIBRARIES)
