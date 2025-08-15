# This is the cmake config file for the Sky Culture Maker plugin
SET(SCM_VERSION "0.1.0")



# Option to manually control converter, defaults to TRUE, but overridden by Qt version
OPTION(SCM_SHOULD_ENABLE_CONVERTER "Attempt to enable Sky Culture Converter" TRUE)
SET(SCM_CONVERTER_ENABLED FALSE) # Default to disabled

IF(SCM_SHOULD_ENABLE_CONVERTER AND NOT (QT_VERSION_MAJOR EQUAL "5"))
  SET(SCM_CONVERTER_ENABLED TRUE)
  MESSAGE(STATUS "Sky Culture Converter will be enabled.")

  # On macOS, Homebrew is a common way to install dependencies.
  # We should look for it in the default locations for Apple Silicon and Intel macs
  # to find the correct version of libtidy. Otherwise a system library might be used.
  IF(APPLE)
    LIST(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
  ENDIF()

  # download CPM.cmake
  FILE(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.8/CPM.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
    EXPECTED_HASH SHA256=78ba32abdf798bc616bab7c73aac32a17bbd7b06ad9e26a6add69de8f3ae4791
  )
  INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake)

  # download https://github.com/Stellarium/stellarium-skyculture-converter
  CPMAddPackage(
    NAME              StellariumSkyCultureConverter
    GITHUB_REPOSITORY Stellarium/stellarium-skyculture-converter
    GIT_TAG           master
    OPTIONS
      "SKYCULTURE_CONVERTER_BUILD_TESTS OFF"
  )

  # download https://github.com/selmf/unarr for archives
  CPMAddPackage(
    NAME              unarr
    GITHUB_REPOSITORY selmf/unarr
    GIT_TAG           v1.1.1
  )

  IF(WIN32)
    # Patch unarr's common/stream.c to include <objidl.h> after windows.h
    # Ensure this runs after unarr source is available
    IF(TARGET unarr AND EXISTS "${unarr_SOURCE_DIR}/common/stream.c")
      FILE(READ "${unarr_SOURCE_DIR}/common/stream.c" _stream_c_content)
      STRING(REPLACE "#define COBJMACROS\n#include <windows.h>\n"
                    "#define COBJMACROS\n#include <windows.h>\n#include <objidl.h>\n"
                    _stream_c_content "${_stream_c_content}")
      FILE(WRITE "${unarr_SOURCE_DIR}/common/stream.c" "${_stream_c_content}")
    ELSE()
      MESSAGE(WARNING "unarr source directory or stream.c not found for patching. This might be an issue on Windows if the converter is enabled.")
    ENDIF()
  ENDIF(WIN32)
  ADD_DEFINITIONS(-DSCM_CONVERTER_ENABLED_CPP) # Define for C++
ELSE()
  MESSAGE(STATUS "Sky Culture Converter is DISABLED (Qt version 5 or SCM_SHOULD_ENABLE_CONVERTER is OFF). Please ensure you are using Qt 6 or later to enable it.")
ENDIF()

ADD_DEFINITIONS(-DSKYCULTUREMAKER_PLUGIN_VERSION="${SCM_VERSION}")
ADD_DEFINITIONS(-DSKYCULTUREMAKER_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )