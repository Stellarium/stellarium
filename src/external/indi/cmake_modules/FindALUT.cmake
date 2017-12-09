# - Locate ALUT
# This module defines
# ALUT_LIBRARY
# ALUT_FOUND, if false, do not try to link to OpenAL
# ALUT_INCLUDE_DIR, where to find the headers
#
# $OPENALDIR is an environment variable that would
# correspond to the ./configure --prefix=$OPENALDIR
# used in building OpenAL.
#
# Created by Bryan Donlan, based on the FindOpenAL.cmake module by Eric Wang.
 
FIND_PATH(ALUT_INCLUDE_DIR alut.h
  $ENV{OPENALDIR}/include
  ~/Library/Frameworks/OpenAL.framework/Headers
  /Library/Frameworks/OpenAL.framework/Headers
  /System/Library/Frameworks/OpenAL.framework/Headers # Tiger
  /usr/local/include/AL
  /usr/local/include/OpenAL
  /usr/local/include
  /usr/include/AL
  /usr/include/OpenAL
  /usr/include
  /sw/include/AL # Fink
  /sw/include/OpenAL
  /sw/include
  /opt/local/include/AL # DarwinPorts
  /opt/local/include/OpenAL
  /opt/local/include
  /opt/csw/include/AL # Blastwave
  /opt/csw/include/OpenAL
  /opt/csw/include
  /opt/include/AL
  /opt/include/OpenAL
  /opt/include
  )
# I'm not sure if I should do a special casing for Apple. It is
# unlikely that other Unix systems will find the framework path.
# But if they do ([Next|Open|GNU]Step?),
# do they want the -framework option also?
IF(${ALUT_INCLUDE_DIR} MATCHES ".framework")
  STRING(REGEX REPLACE "(.*)/.*\\.framework/.*" "\\1" ALUT_FRAMEWORK_PATH_TMP ${ALUT_INCLUDE_DIR})
  IF("${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
      OR "${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
      )
    # String is in default search path, don't need to use -F
    SET (ALUT_LIBRARY "-framework OpenAL" CACHE STRING "OpenAL framework for OSX")
  ELSE("${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
      OR "${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
      )
    # String is not /Library/Frameworks, need to use -F
    SET(ALUT_LIBRARY "-F${ALUT_FRAMEWORK_PATH_TMP} -framework OpenAL" CACHE STRING "OpenAL framework for OSX")
  ENDIF("${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/Library/Frameworks"
    OR "${ALUT_FRAMEWORK_PATH_TMP}" STREQUAL "/System/Library/Frameworks"
    )
  # Clear the temp variable so nobody can see it
  SET(ALUT_FRAMEWORK_PATH_TMP "" CACHE INTERNAL "")
 
ELSE(${ALUT_INCLUDE_DIR} MATCHES ".framework")
  FIND_LIBRARY(ALUT_LIBRARY
    NAMES alut
    PATHS
    $ENV{OPENALDIR}/lib
    $ENV{OPENALDIR}/libs
    /usr/local/lib
    /usr/lib
    /sw/lib
    /opt/local/lib
    /opt/csw/lib
    /opt/lib
    )
ENDIF(${ALUT_INCLUDE_DIR} MATCHES ".framework")
 
SET(ALUT_FOUND "NO")
IF(ALUT_LIBRARY)
  SET(ALUT_FOUND "YES")
ENDIF(ALUT_LIBRARY)
