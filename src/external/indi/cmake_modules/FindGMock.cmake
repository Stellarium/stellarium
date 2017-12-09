#.rst:
# FindGMock
# ---------
#
# Locate the Google C++ Mocking Framework.
#
# Defines the following variables:
#
# ::
#
#    GMOCK_FOUND - Found the Google Mocking framework
#    GMOCK_INCLUDE_DIRS - Include directories
#
#
#
# Also defines the library variables below as normal variables.  These
# contain debug/optimized keywords when a debugging library is found.
#
# ::
#
#    GMOCK_LIBRARIES - libgmock
#
#
#
# Accepts the following variables as input:
#
# ::
#
#    GMOCK_ROOT - (as a CMake or environment variable)
#                 The root directory of the gmock install prefix
#
#
#
# ::
#
#    GMOCK_MSVC_SEARCH - If compiling with MSVC, this variable can be set to
#                        "MD" or "MT" to enable searching a GMock build tree
#                        (defaults: "MD")
#
#
#
# Example Usage:
#
# ::
#
#     find_package(GMock REQUIRED)
#     include_directories(${GMOCK_INCLUDE_DIRS})
#
#
#
# ::
#
#     add_executable(foo foo.cc)
#     target_link_libraries(foo ${GMOCK_LIBRARIES})
#
#
#
# ::
#
#     add_test(AllMocksInFoo foo)
#
#
#
#
#
# If you would like each Google test to show up in CMock as a test you
# may use the following macro.  NOTE: It will slow down your tests by
# running an executable for each test and test fixture.  You will also
# have to rerun CMake after adding or removing tests or test fixtures.
#
# GMOCK_ADD_MOCKS(executable extra_args ARGN)
#
# ::
#
#     executable = The path to the test executable
#     extra_args = Pass a list of extra arguments to be passed to
#                  executable enclosed in quotes (or "" for none)
#     ARGN =       A list of source files to search for tests & test
#                  fixtures.
#
#
#
# ::
#
#   Example:
#      set(FooMockArgs --foo 1 --bar 2)
#      add_executable(FooMock FooUnitMock.cc)
#      GMOCK_ADD_MOCKS(FooMock "${FooMockArgs}" FooUnitMock.cc)

#=============================================================================
# Copyright 2009 Kitware, Inc.
# Copyright 2009 Philip Lowman <philip@yhbt.com>
# Copyright 2009 Daniel Blezek <blezek@gmail.com>
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)
#
# Thanks to Daniel Blezek <blezek@gmail.com> for the GMOCK_ADD_MOCKS code

function(GMOCK_ADD_MOCKS executable extra_args)
    if(NOT ARGN)
        message(FATAL_ERROR "Missing ARGN: Read the documentation for GMOCK_ADD_MOCKS")
    endif()
    foreach(source ${ARGN})
        file(READ "${source}" contents)
        string(REGEX MATCHALL "MOCK_?F?\\(([A-Za-z_0-9 ,]+)\\)" found_tests ${contents})
        foreach(hit ${found_tests})
            string(REGEX REPLACE ".*\\( *([A-Za-z_0-9]+), *([A-Za-z_0-9]+) *\\).*" "\\1.\\2" test_name ${hit})
            add_test(${test_name} ${executable} --gmock_filter=${test_name} ${extra_args})
        endforeach()
    endforeach()
endfunction()

function(_gmock_append_debugs _endvar _library)
    if(${_library} AND ${_library}_DEBUG)
        set(_output optimized ${${_library}} debug ${${_library}_DEBUG})
    else()
        set(_output ${${_library}})
    endif()
    set(${_endvar} ${_output} PARENT_SCOPE)
endfunction()

function(_gmock_find_library _name)
    find_library(${_name}
        NAMES ${ARGN}
        HINTS
            ENV GMOCK_ROOT
            ${GMOCK_ROOT}
        PATH_SUFFIXES ${_gmock_libpath_suffixes}
    )
    mark_as_advanced(${_name})
endfunction()

#

if(NOT DEFINED GMOCK_MSVC_SEARCH)
    set(GMOCK_MSVC_SEARCH MD)
endif()

set(_gmock_libpath_suffixes lib)
if(MSVC)
    if(GMOCK_MSVC_SEARCH STREQUAL "MD")
        list(APPEND _gmock_libpath_suffixes
            msvc/gmock-md/Debug
            msvc/gmock-md/Release)
    elseif(GMOCK_MSVC_SEARCH STREQUAL "MT")
        list(APPEND _gmock_libpath_suffixes
            msvc/gmock/Debug
            msvc/gmock/Release)
    endif()
endif()


find_path(GMOCK_INCLUDE_DIR gmock/gmock.h
    HINTS
        $ENV{GMOCK_ROOT}/include
        ${GMOCK_ROOT}/include
)
mark_as_advanced(GMOCK_INCLUDE_DIR)

if(MSVC AND GMOCK_MSVC_SEARCH STREQUAL "MD")
    # The provided /MD project files for Google Mock add -md suffixes to the
    # library names.
    _gmock_find_library(GMOCK_LIBRARY            gmock-md  gmock)
    _gmock_find_library(GMOCK_LIBRARY_DEBUG      gmock-mdd gmockd)
else()
    _gmock_find_library(GMOCK_LIBRARY            gmock)
    _gmock_find_library(GMOCK_LIBRARY_DEBUG      gmockd)
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(GMock DEFAULT_MSG GMOCK_LIBRARY GMOCK_INCLUDE_DIR)

if(GMOCK_FOUND)
    set(GMOCK_INCLUDE_DIRS ${GMOCK_INCLUDE_DIR})
    _gmock_append_debugs(GMOCK_LIBRARIES      GMOCK_LIBRARY)
endif()

