# This is the cmake config file for the Sky Culture Maker plugin
SET(SCM_VERSION "1.0.0")

# Option to manually control converter, defaults to TRUE, but overridden by Qt version
OPTION(SCM_SHOULD_ENABLE_CONVERTER "Attempt to enable Sky Culture Converter" TRUE)
SET(SCM_CONVERTER_ENABLED FALSE) # Default to disabled

IF(SCM_SHOULD_ENABLE_CONVERTER AND NOT (QT_VERSION_MAJOR EQUAL "5"))
  SET(SCM_CONVERTER_ENABLED TRUE)
  MESSAGE(STATUS "Sky Culture Converter will be enabled.")

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
    OPTIONS
      "USE_SYSTEM_BZ2 OFF"
      "USE_SYSTEM_LZMA OFF"
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