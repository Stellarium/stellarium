# This is the cmake config file for the Sky Culture Maker plugin
# The SCM version matches the Stellarium version
SET(SCM_VERSION "${VERSION}")

# Option to manually control converter, defaults to FALSE
OPTION(SCM_SHOULD_ENABLE_CONVERTER "Attempt to enable Sky Culture Converter" FALSE)
SET(SCM_CONVERTER_ENABLED FALSE) # Default to disabled

# Detect Qt5
IF(QT_VERSION_MAJOR EQUAL "5")
  SET(IS_QT_5 TRUE)
ELSE()
  SET(IS_QT_5 FALSE)
ENDIF()

# Detect Windows ARM64
IF(WIN32 AND (CMAKE_SYSTEM_PROCESSOR MATCHES "ARM64|arm64|aarch64" OR CMAKE_VS_PLATFORM_NAME MATCHES "ARM64|arm64"))
  SET(IS_WIN_ARM64 TRUE)
ELSE()
  SET(IS_WIN_ARM64 FALSE)
ENDIF()

# Disable converter for Qt5
# Disable converter for Windows ARM64 builds due to missing ARM64 libraries
IF(SCM_SHOULD_ENABLE_CONVERTER AND NOT IS_QT_5 AND NOT IS_WIN_ARM64)
  SET(SCM_CONVERTER_ENABLED TRUE)
  MESSAGE(STATUS "Sky Culture Converter will be enabled.")

  # download https://github.com/Stellarium/stellarium-skyculture-converter
  CPMFindPackage(
    NAME StellariumSkyCultureConverter
    URL https://github.com/Stellarium/stellarium-skyculture-converter/archive/refs/tags/v0.0.1.zip
    URL_HASH SHA256=07c4f5b7f4cbf3d1d62b2eed30237d04b979f72415476c2f401a3c6198eee2ea
    EXCLUDE_FROM_ALL yes
    OPTIONS
      "SKYCULTURE_CONVERTER_BUILD_TESTS OFF"
  )

  # download https://github.com/selmf/unarr for archives
  # Using commit 1df8ab3 to allow CMake support up to 4.0
  CPMFindPackage(
    NAME unarr
    URL https://github.com/selmf/unarr/archive/1df8ab3d281409e9fe6bed8bf485976bb47f5bef.zip
    URL_HASH SHA256=f89f602184c90b01b47eb76f1f6334801c3d22eb6f5deddcff92c9ea79992903
    EXCLUDE_FROM_ALL yes
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
  IF(IS_QT_5)
    MESSAGE(STATUS "Sky Culture Converter is DISABLED because it requires Qt 6 or later.")
  ELSEIF(IS_WIN_ARM64)
    MESSAGE(STATUS "Sky Culture Converter is DISABLED for Windows ARM64 builds (missing ARM64 tidy/gettext libraries).")
  ELSE()
    MESSAGE(STATUS "Sky Culture Converter is DISABLED.")
  ENDIF()
ENDIF()

ADD_DEFINITIONS(-DSKYCULTUREMAKER_PLUGIN_VERSION="${SCM_VERSION}")
ADD_DEFINITIONS(-DSKYCULTUREMAKER_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
