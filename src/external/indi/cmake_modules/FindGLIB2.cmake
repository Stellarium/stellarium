# - Try to find GLib2
# Once done this will define
#
#  GLIB2_FOUND - system has GLib2
#  GLIB2_INCLUDE_DIRS - the GLib2 include directory
#  GLIB2_LIBRARIES - Link these to use GLib2
#
#  HAVE_GLIB_GREGEX_H  glib has gregex.h header and 
#                      supports g_regex_match_simple
#
#  Copyright (c) 2006 Andreas Schneider <mail@cynapses.org>
#  Copyright (c) 2006 Philippe Bernery <philippe.bernery@gmail.com>
#  Copyright (c) 2007 Daniel Gollub <dgollub@suse.de>
#  Copyright (c) 2007 Alban Browaeys <prahal@yahoo.com>
#  Copyright (c) 2008 Michael Bell <michael.bell@web.de>
#  Copyright (c) 2008 Bjoern Ricks <bjoern.ricks@googlemail.com>
#
#  Redistribution and use is allowed according to the terms of the New
#  BSD license.
#  For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


IF (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )
  # in cache already
  SET(GLIB2_FOUND TRUE)
ELSE (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS )

  INCLUDE(FindPkgConfig)

  ## Glib
  IF ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "REQUIRED" )
  ELSE ( GLIB2_FIND_REQUIRED )
    SET( _pkgconfig_REQUIRED "" )
  ENDIF ( GLIB2_FIND_REQUIRED )

  IF ( GLIB2_MIN_VERSION )
    PKG_SEARCH_MODULE( GLIB2 ${_pkgconfig_REQUIRED} glib-2.0>=${GLIB2_MIN_VERSION} )
  ELSE ( GLIB2_MIN_VERSION )
    PKG_SEARCH_MODULE( GLIB2 ${_pkgconfig_REQUIRED} glib-2.0 )
  ENDIF ( GLIB2_MIN_VERSION )
  IF ( PKG_CONFIG_FOUND )
    IF ( GLIB2_FOUND )
      SET ( GLIB2_CORE_FOUND TRUE )
    ELSE ( GLIB2_FOUND )
      SET ( GLIB2_CORE_FOUND FALSE )
    ENDIF ( GLIB2_FOUND )
  ENDIF ( PKG_CONFIG_FOUND )

  # Look for glib2 include dir and libraries w/o pkgconfig
  IF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
    FIND_PATH(
      _glibconfig_include_DIR
    NAMES
      glibconfig.h
    PATHS
      /opt/gnome/lib64
      /opt/gnome/lib
      /opt/lib/
      /opt/local/lib
      /sw/lib/
      /usr/lib64
      /usr/lib
      /usr/local/include
      ${CMAKE_LIBRARY_PATH}
    PATH_SUFFIXES
      glib-2.0/include
    )

    FIND_PATH(
      _glib2_include_DIR
    NAMES
      glib.h
    PATHS
      /opt/gnome/include
      /opt/local/include
      /sw/include
      /usr/include
      /usr/local/include
    PATH_SUFFIXES
      glib-2.0
    )

    #MESSAGE(STATUS "Glib headers: ${_glib2_include_DIR}")

    FIND_LIBRARY(
      _glib2_link_DIR
    NAMES
      glib-2.0
      glib
    PATHS
      /opt/gnome/lib
      /opt/local/lib
      /sw/lib
      /usr/lib
      /usr/local/lib
    )
    IF ( _glib2_include_DIR AND _glib2_link_DIR )
        SET ( _glib2_FOUND TRUE )
    ENDIF ( _glib2_include_DIR AND _glib2_link_DIR )


    IF ( _glib2_FOUND )
        SET ( GLIB2_INCLUDE_DIRS ${_glib2_include_DIR} ${_glibconfig_include_DIR} )
        SET ( GLIB2_LIBRARIES ${_glib2_link_DIR} )
        SET ( GLIB2_CORE_FOUND TRUE )
    ELSE ( _glib2_FOUND )
        SET ( GLIB2_CORE_FOUND FALSE )
    ENDIF ( _glib2_FOUND )

    # Handle dependencies
    # libintl
    IF ( NOT LIBINTL_FOUND )
      FIND_PATH(LIBINTL_INCLUDE_DIR
      NAMES
        libintl.h
      PATHS
        /opt/gnome/include
        /opt/local/include
        /sw/include
        /usr/include
        /usr/local/include
      )

      FIND_LIBRARY(LIBINTL_LIBRARY
      NAMES
        intl
      PATHS
        /opt/gnome/lib
        /opt/local/lib
        /sw/lib
        /usr/local/lib
        /usr/lib
      )

      IF (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
        SET (LIBINTL_FOUND TRUE)
      ENDIF (LIBINTL_LIBRARY AND LIBINTL_INCLUDE_DIR)
    ENDIF ( NOT LIBINTL_FOUND )

    # libiconv
    IF ( NOT LIBICONV_FOUND )
      FIND_PATH(LIBICONV_INCLUDE_DIR
      NAMES
        iconv.h
      PATHS
        /opt/gnome/include
        /opt/local/include
        /opt/local/include
        /sw/include
        /sw/include
        /usr/local/include
        /usr/include
      PATH_SUFFIXES
        glib-2.0
      )

      FIND_LIBRARY(LIBICONV_LIBRARY
      NAMES
        iconv
      PATHS
        /opt/gnome/lib
        /opt/local/lib
        /sw/lib
        /usr/lib
        /usr/local/lib
      )

      IF (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
        SET (LIBICONV_FOUND TRUE)
      ENDIF (LIBICONV_LIBRARY AND LIBICONV_INCLUDE_DIR)
    ENDIF ( NOT LIBICONV_FOUND )

    IF (LIBINTL_FOUND)
      SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBINTL_LIBRARY})
      SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBINTL_INCLUDE_DIR})
    ENDIF (LIBINTL_FOUND)

    IF (LIBICONV_FOUND)
      SET (GLIB2_LIBRARIES ${GLIB2_LIBRARIES} ${LIBICONV_LIBRARY})
      SET (GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIRS} ${LIBICONV_INCLUDE_DIR})
    ENDIF (LIBICONV_FOUND)

  ENDIF ( NOT GLIB2_FOUND AND NOT PKG_CONFIG_FOUND )
  ##

  IF (GLIB2_CORE_FOUND AND GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)
    SET (GLIB2_FOUND TRUE)
  ENDIF (GLIB2_CORE_FOUND AND GLIB2_INCLUDE_DIRS AND GLIB2_LIBRARIES)

  IF (GLIB2_FOUND)
    IF (NOT GLIB2_FIND_QUIETLY)
      MESSAGE (STATUS "Found GLib2: ${GLIB2_LIBRARIES} ${GLIB2_INCLUDE_DIRS}")
    ENDIF (NOT GLIB2_FIND_QUIETLY)
  ELSE (GLIB2_FOUND)
    IF (GLIB2_FIND_REQUIRED)
      MESSAGE (SEND_ERROR "Could not find GLib2")
    ENDIF (GLIB2_FIND_REQUIRED)
  ENDIF (GLIB2_FOUND)

  # show the GLIB2_INCLUDE_DIRS and GLIB2_LIBRARIES variables only in the advanced view
  MARK_AS_ADVANCED(GLIB2_INCLUDE_DIRS GLIB2_LIBRARIES)
  MARK_AS_ADVANCED(LIBICONV_INCLUDE_DIR LIBICONV_LIBRARY)
  MARK_AS_ADVANCED(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARY)

ENDIF (GLIB2_LIBRARIES AND GLIB2_INCLUDE_DIRS)

IF ( GLIB2_FOUND )
	# Check if system has a newer version of glib
	# which supports g_regex_match_simple
	INCLUDE( CheckIncludeFiles )
	SET( CMAKE_REQUIRED_INCLUDES ${GLIB2_INCLUDE_DIRS} )
	CHECK_INCLUDE_FILES ( glib/gregex.h HAVE_GLIB_GREGEX_H )
	# Reset CMAKE_REQUIRED_INCLUDES
	SET( CMAKE_REQUIRED_INCLUDES "" )
ENDIF( GLIB2_FOUND )

