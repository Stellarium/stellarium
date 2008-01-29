# Python macros
# ~~~~~~~~~~~~~
# Copyright (c) 2007, Simon Edwards <simon@simonzone.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
# This file defines the following macros:
#
# PYTHON_INSTALL (SOURCE_FILE DESINATION_DIR)
#     Install the SOURCE_FILE, which is a Python .py file, into the
#     destination directory during install. The file will be byte compiled
#     and both the .py file and .pyc file will be installed.

GET_FILENAME_COMPONENT(PYTHON_MACROS_MODULE_PATH ${CMAKE_CURRENT_LIST_FILE}  PATH)

MACRO(PYTHON_INSTALL SOURCE_FILE DESINATION_DIR)
  ADD_CUSTOM_TARGET(compile_python_files ALL)

  # Install the source file.
  INSTALL(FILES ${SOURCE_FILE} DESTINATION ${DESINATION_DIR})

  # Byte compile and install the .pyc file.        
  GET_FILENAME_COMPONENT(_absfilename ${SOURCE_FILE} ABSOLUTE)
  GET_FILENAME_COMPONENT(_filename ${SOURCE_FILE} NAME)
  GET_FILENAME_COMPONENT(_filenamebase ${SOURCE_FILE} NAME_WE)
  GET_FILENAME_COMPONENT(_basepath ${SOURCE_FILE} PATH)
  SET(_bin_py ${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/${_filename})
  SET(_bin_pyc ${CMAKE_CURRENT_BINARY_DIR}/${_basepath}/${_filenamebase}.pyc)

  FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${_basepath})

  SET(_message "-DMESSAGE=Byte-compiling ${_bin_py}")

  GET_FILENAME_COMPONENT(_abs_bin_py ${_bin_py} ABSOLUTE)
  IF(_abs_bin_py STREQUAL ${_absfilename})    # Don't copy the file onto itself.
    ADD_CUSTOM_COMMAND(
      TARGET compile_python_files
      COMMAND ${CMAKE_COMMAND} ${_message} -P ${PYTHON_MACROS_MODULE_PATH}/print_status.cmake
      COMMAND ${PYTHON_EXECUTABLE} ${PYTHON_MACROS_MODULE_PATH}/PythonCompile.py ${_bin_py}
      DEPENDS ${_absfilename}
    )
  ELSE(_abs_bin_py STREQUAL ${_absfilename})
    ADD_CUSTOM_COMMAND(
      TARGET compile_python_files
      COMMAND ${CMAKE_COMMAND} ${_message} -P ${PYTHON_MACROS_MODULE_PATH}/print_status.cmake
      COMMAND ${CMAKE_COMMAND} -E copy ${_absfilename} ${_bin_py}
      COMMAND ${PYTHON_EXECUTABLE} ${PYTHON_MACROS_MODULE_PATH}/PythonCompile.py ${_bin_py}
      DEPENDS ${_absfilename}
    )
  ENDIF(_abs_bin_py STREQUAL ${_absfilename})

  INSTALL(FILES ${_bin_pyc} DESTINATION ${DESINATION_DIR})
ENDMACRO(PYTHON_INSTALL)
