# Find Python
# ~~~~~~~~~~~
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# Find the Python interpreter and related Python directories.
#
# This file defines the following variables:
# 
# PYTHON_EXECUTABLE - The path and filename of the Python interpreter.
#
# PYTHON_SHORT_VERSION - The version of the Python interpreter found,
#     excluding the patch version number. (e.g. 2.5 and not 2.5.1))
# 
# PYTHON_LONG_VERSION - The version of the Python interpreter found as a human
#     readable string.
#
# PYTHON_SITE_PACKAGES_DIR - Location of the Python site-packages directory.
#
# PYTHON_INCLUDE_PATH - Directory holding the python.h include file.
#
# PYTHON_LIBRARY, PYTHON_LIBRARIES- Location of the Python library.

INCLUDE(CMakeFindFrameworks)

IF(PYTHON_LIBRARY)
   # Already in cache, be silent
   SET(PYTHONLIBRARY_FOUND TRUE)
ELSE(PYTHON_LIBRARY)

  FIND_PACKAGE(PythonInterp REQUIRED)
  GET_FILENAME_COMPONENT(_cmake_module_path ${CMAKE_CURRENT_LIST_FILE}  PATH)

  EXECUTE_PROCESS(COMMAND ${PYTHON_EXECUTABLE}  ${_cmake_module_path}/FindLibPython.py OUTPUT_VARIABLE python_config)
  IF(python_config)
    STRING(REGEX REPLACE ".*exec_prefix:([^\n]+).*$" "\\1" PYTHON_PREFIX ${python_config})
    STRING(REGEX REPLACE ".*\nshort_version:([^\n]+).*$" "\\1" PYTHON_SHORT_VERSION ${python_config})
    STRING(REGEX REPLACE ".*\nlong_version:([^\n]+).*$" "\\1" PYTHON_LONG_VERSION ${python_config})
    STRING(REGEX REPLACE ".*\npy_inc_dir:([^\n]+).*$" "\\1" PYTHON_INCLUDE_PATH ${python_config})
    STRING(REGEX REPLACE ".*\nsite_packages_dir:([^\n]+).*$" "\\1" PYTHON_SITE_PACKAGES_DIR ${python_config})
    FIND_LIBRARY(PYTHON_LIBRARY NAMES python${PYTHON_SHORT_VERSION} PATHS ${PYTHON_PREFIX}/lib NO_DEFAULT_PATH)
    SET(PYTHONLIBRARY_FOUND TRUE)
  ENDIF(python_config)

  # adapted from cmake's builtin FindPythonLibs
  if(APPLE)
    CMAKE_FIND_FRAMEWORKS(Python)
    SET(PYTHON_FRAMEWORK_INCLUDES)
    IF(Python_FRAMEWORKS)
      # If a framework has been selected for the include path,
      # make sure "-framework" is used to link it.
      IF("${PYTHON_INCLUDE_PATH}" MATCHES "Python\\.framework")
        SET(PYTHON_LIBRARY "")
        SET(PYTHON_DEBUG_LIBRARY "")
      ENDIF("${PYTHON_INCLUDE_PATH}" MATCHES "Python\\.framework")
      IF(NOT PYTHON_LIBRARY)
        SET (PYTHON_LIBRARY "-framework Python" CACHE FILEPATH "Python Framework" FORCE)
      ENDIF(NOT PYTHON_LIBRARY)
      SET(PYTHONLIBRARY_FOUND TRUE)
    ENDIF(Python_FRAMEWORKS)
  ENDIF(APPLE)

  IF(PYTHONLIBRARY_FOUND)
    SET(PYTHON_LIBRARIES ${PYTHON_LIBRARY})
    IF(NOT PYTHONLIBRARY_FIND_QUIETLY)
      MESSAGE(STATUS "Found Python executable: ${PYTHON_EXECUTABLE}")
      MESSAGE(STATUS "Found Python version: ${PYTHON_LONG_VERSION}")
      MESSAGE(STATUS "Found Python library: ${PYTHON_LIBRARY}")
    ENDIF(NOT PYTHONLIBRARY_FIND_QUIETLY)
  ELSE(PYTHONLIBRARY_FOUND)
    IF(PYTHONLIBRARY_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find Python")
    ENDIF(PYTHONLIBRARY_FIND_REQUIRED)
  ENDIF(PYTHONLIBRARY_FOUND)

ENDIF (PYTHON_LIBRARY)
