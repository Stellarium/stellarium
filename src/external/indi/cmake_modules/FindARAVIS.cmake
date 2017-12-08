# - Find the native sqlite3 includes and library
#
# This module defines
#  ARV_INCLUDE_DIR, where to find libgphoto2 header files
#  ARV_LIBRARIES, the libraries to link against to use libgphoto2
#  ARV_FOUND, If false, do not try to use libgphoto2.
#  ARV_VERSION_STRING, e.g. 2.4.14
#  ARV_VERSION_MAJOR, e.g. 2
#  ARV_VERSION_MINOR, e.g. 4
#  ARV_VERSION_PATCH, e.g. 14
#
# also defined, but not for general use are
#  ARV_LIBRARY, where to find the sqlite3 library.


#=============================================================================
# Copyright 2010 henrik andersson
#=============================================================================

SET(ARV_FIND_REQUIRED ${Arv_FIND_REQUIRED})

find_path(ARV_INCLUDE_DIR aravis-0.6/arv.h)
mark_as_advanced(ARV_INCLUDE_DIR)

set(ARV_NAMES ${ARV_NAMES} aravis-0.6)
find_library(ARV_LIBRARY NAMES ${ARV_NAMES} )
mark_as_advanced(ARV_LIBRARY)

set(ARV_VERSION_MAJOR "0")
set(ARV_VERSION_MINOR "6")
set(ARV_VERSION_STRING "${ARV_VERSION_MAJOR}.${ARV_VERSION_MINOR}")

# handle the QUIETLY and REQUIRED arguments and set ARV_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ARV DEFAULT_MSG ARV_LIBRARY ARV_INCLUDE_DIR)

IF(ARV_FOUND)
    #SET(Arv_LIBRARIES ${ARV_LIBRARY})
    SET(Arv_LIBRARIES "aravis-0.6")
    SET(Arv_INCLUDE_DIRS "${ARV_INCLUDE_DIR}/aravis-0.6")
    MESSAGE (STATUS "Found aravis: ${Arv_LIBRARIES} ${Arv_INCLUDE_DIRS}")
ENDIF(ARV_FOUND)
