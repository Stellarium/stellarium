# - Try to find libmodbus
# Once done this will define
#
#  MODBUS_FOUND - system has MODBUS
#  MODBUS_INCLUDE_DIR - the MODBUS include directory
#  MODBUS_LIBRARIES - Link these to use MODBUS

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)

  # in cache already
  set(MODBUS_FOUND TRUE)
  message(STATUS "Found libmodbus: ${MODBUS_LIBRARIES}")

else (MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)

  find_path(MODBUS_INCLUDE_DIR modbus.h
    PATH_SUFFIXES modbus
    ${_obIncDir}
    ${GNUWIN32_DIR}/include
  )

  find_library(MODBUS_LIBRARIES NAMES modbus
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

 set(CMAKE_REQUIRED_INCLUDES ${MODBUS_INCLUDE_DIR})
 set(CMAKE_REQUIRED_LIBRARIES ${MODBUS_LIBRARIES})

   if(MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)
    set(MODBUS_FOUND TRUE)
  else (MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)
    set(MODBUS_FOUND FALSE)
  endif(MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)

  if (MODBUS_FOUND)
    if (NOT MODBUS_FIND_QUIETLY)
      message(STATUS "Found libmodbus: ${MODBUS_LIBRARIES}")
    endif (NOT MODBUS_FIND_QUIETLY)
  else (MODBUS_FOUND)
    if (MODBUS_FIND_REQUIRED)
      message(FATAL_ERROR "libmodbus not found. Please install libmodbus-devel. https://launchpad.net/libmodbus/")
    endif (MODBUS_FIND_REQUIRED)
  endif (MODBUS_FOUND)

  mark_as_advanced(MODBUS_INCLUDE_DIR MODBUS_LIBRARIES)
  
endif (MODBUS_INCLUDE_DIR AND MODBUS_LIBRARIES)

