# - Try to find FreeType2
# Once done this will define:
#  FreeType2_FOUND        - system has FreeType2
#  FreeType2_INCLUDE_DIR  - incude paths to use FreeType2
#  FreeType2_LIBRARIES    - Link these to use FreeType2
#
# TODO: This script should use the program 

# Copyright (c) 2006, Stefan Kebekus, <kebekus@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.


  FIND_PATH(FreeType2_INCLUDE_DIR 
    freetype/config/ftheader.h
    /usr/include/freetype2
    /usr/X11R6/include/freetype
    /usr/nekoware/include/freetype2
    /opt/local/include/freetype2
    )

  FIND_LIBRARY(FreeType2_LIBRARIES
    freetype
    /usr/X11R6/lib
    /usr/local/lib
    /usr/openwin/lib
    /usr/lib
    /usr/nekoware/lib
    /opt/local/lib
    )

if (FreeType2_INCLUDE_DIR AND FreeType2_LIBRARIES)
   set(FreeType2_FOUND TRUE)
else (FreeType2_INCLUDE_DIR AND FreeType2_LIBRARIES)
   set(FreeType2_FOUND FALSE)
endif (FreeType2_INCLUDE_DIR AND FreeType2_LIBRARIES)

if (FreeType2_FOUND)	
   if (NOT FreeType2_FIND_QUIETLY)
      message(STATUS "Found FreeType2: library: ${FreeType2_LIBRARIES}, include path: ${FreeType2_INCLUDE_DIR}")
   endif (NOT FreeType2_FIND_QUIETLY)
else (FreeType2_FOUND)
   if (FreeType2_FIND_REQUIRED)	
      MESSAGE(FATAL_ERROR "Could not find the FreeType2 library and header files.")
   ENDIF (FreeType2_FIND_REQUIRED)
endif (FreeType2_FOUND)

MARK_AS_ADVANCED(
  FreeType2_INCLUDE_DIR
  FreeType2_LIBRARIES
)
