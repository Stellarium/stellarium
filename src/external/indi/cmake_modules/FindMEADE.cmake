# - Try to find Meade DSI Library.
# Once done this will define
#
#  MEADEDSI_FOUND - system has Meade DSI
#  MEADEDSI_LIBRARIES - Link these to use Meade DSI

# Copyright (c) 2006, Jasem Mutlaq <mutlaqja@ikarustech.com>
# Based on FindLibfacile by Carsten Niehaus, <cniehaus@gmx.de>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if (MEADEDSI_LIBRARIES)

  # in cache already
  set(MEADEDSI_FOUND TRUE)
  message(STATUS "Found MEADEDSI: ${MEADEDSI_LIBRARIES}")

else (MEADEDSI_LIBRARIES)

  find_library(MEADEDSI_LIBRARIES NAMES dsi
    PATHS
    ${_obLinkDir}
    ${GNUWIN32_DIR}/lib
  )

 set(CMAKE_REQUIRED_LIBRARIES ${MEADEDSI_LIBRARIES})

   if(MEADEDSI_LIBRARIES)
    set(MEADEDSI_FOUND TRUE)
  else (MEADEDSI_LIBRARIES)
    set(MEADEDSI_FOUND FALSE)
  endif(MEADEDSI_LIBRARIES)

  if (MEADEDSI_FOUND)
    if (NOT MEADEDSI_FIND_QUIETLY)
      message(STATUS "Found Meade DSI: ${MEADEDSI_LIBRARIES}")
    endif (NOT MEADEDSI_FIND_QUIETLY)
  else (MEADEDSI_FOUND)
    if (MEADEDSI_FIND_REQUIRED)
      message(FATAL_ERROR "Meade DSI not found. Please install Meade DSI library. http://linuxdsi.sourceforge.net")
    endif (MEADEDSI_FIND_REQUIRED)
  endif (MEADEDSI_FOUND)

  mark_as_advanced(MEADEDSI_LIBRARIES)

endif (MEADEDSI_LIBRARIES)
