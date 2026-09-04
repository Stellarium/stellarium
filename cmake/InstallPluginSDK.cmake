# Configure and install the minimal Qt6/MSVC SDK for external dynamic plug-ins.
INCLUDE(GNUInstallDirs)
INCLUDE(CMakePackageConfigHelpers)
INCLUDE("${PROJECT_SOURCE_DIR}/cmake/StellariumPluginSDKHeaders.cmake")

IF(NOT WIN32 OR NOT MSVC OR NOT Qt6_FOUND OR NOT GENERATE_STELMAINLIB)
     MESSAGE(FATAL_ERROR "The Windows plug-in SDK requires Qt6, MSVC, and GENERATE_STELMAINLIB")
ENDIF()

SET(_plugin_sdk_seen_headers "")
FOREACH(_plugin_sdk_header IN LISTS STELLARIUM_PLUGIN_SDK_HEADERS)
     IF(IS_ABSOLUTE "${_plugin_sdk_header}" OR
          _plugin_sdk_header MATCHES "(^|/)\\.\\.(/|$)" OR
          _plugin_sdk_header MATCHES "\\\\" OR
          NOT _plugin_sdk_header MATCHES "^[A-Za-z0-9_./+-]+\\.hpp$")
          MESSAGE(FATAL_ERROR "Invalid plug-in SDK header path: ${_plugin_sdk_header}")
     ENDIF()
     IF(_plugin_sdk_header IN_LIST _plugin_sdk_seen_headers)
          MESSAGE(FATAL_ERROR "Duplicate plug-in SDK header: ${_plugin_sdk_header}")
     ENDIF()
     IF(NOT EXISTS "${PROJECT_SOURCE_DIR}/src/${_plugin_sdk_header}")
          MESSAGE(FATAL_ERROR "Plug-in SDK header does not exist: ${_plugin_sdk_header}")
     ENDIF()
     LIST(APPEND _plugin_sdk_seen_headers "${_plugin_sdk_header}")
ENDFOREACH()
UNSET(_plugin_sdk_seen_headers)

IF(STELLARIUM_BUILD_ARM64)
     SET(STELLARIUM_PLUGIN_ABI_ARCHITECTURE "arm64")
ELSEIF(CMAKE_SIZEOF_VOID_P EQUAL 8)
     SET(STELLARIUM_PLUGIN_ABI_ARCHITECTURE "x64")
ELSE()
     SET(STELLARIUM_PLUGIN_ABI_ARCHITECTURE "x86")
ENDIF()

SET(STELLARIUM_PLUGIN_ABI_CONFIGURATION "Release")
SET(STELLARIUM_PLUGIN_ABI_MSVC_RUNTIME "MultiThreadedDLL")
SET(STELLARIUM_PLUGIN_SDK_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/stellarium")
SET(STELLARIUM_PLUGIN_SDK_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/Stellarium")
SET(STELLARIUM_PLUGIN_SDK_DATADIR "${CMAKE_INSTALL_DATADIR}/stellarium/plugin-sdk")

SET(STELLARIUM_PLUGIN_SDK_SOURCE_REVISION "" CACHE STRING
     "Source revision recorded in the Windows plug-in SDK")
SET(STELLARIUM_PLUGIN_SDK_BUILD_ID "local" CACHE STRING
     "Package build identifier recorded in the Windows plug-in SDK")
IF(NOT STELLARIUM_PLUGIN_SDK_SOURCE_REVISION)
     FIND_PACKAGE(Git QUIET)
     SET(_plugin_sdk_git_result 1)
     IF(GIT_EXECUTABLE)
          EXECUTE_PROCESS(
               COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
               WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}"
               RESULT_VARIABLE _plugin_sdk_git_result
               OUTPUT_VARIABLE STELLARIUM_PLUGIN_SDK_SOURCE_REVISION
               OUTPUT_STRIP_TRAILING_WHITESPACE
               ERROR_QUIET)
     ENDIF()
     IF(NOT _plugin_sdk_git_result EQUAL 0 OR
          NOT STELLARIUM_PLUGIN_SDK_SOURCE_REVISION)
          SET(STELLARIUM_PLUGIN_SDK_SOURCE_REVISION "unknown")
     ENDIF()
ENDIF()
UNSET(_plugin_sdk_git_result)

FUNCTION(_stellarium_plugin_sdk_json_escape _input _output)
     SET(_value "${_input}")
     STRING(REPLACE "\\" "\\\\" _value "${_value}")
     STRING(REPLACE "\"" "\\\"" _value "${_value}")
     STRING(REPLACE "\n" "\\n" _value "${_value}")
     STRING(REPLACE "\r" "\\r" _value "${_value}")
     STRING(REPLACE "\t" "\\t" _value "${_value}")
     SET(${_output} "${_value}" PARENT_SCOPE)
ENDFUNCTION()
_stellarium_plugin_sdk_json_escape(
     "${STELLARIUM_PLUGIN_SDK_SOURCE_REVISION}"
     STELLARIUM_PLUGIN_SDK_SOURCE_REVISION_JSON)
_stellarium_plugin_sdk_json_escape(
     "${STELLARIUM_PLUGIN_SDK_BUILD_ID}"
     STELLARIUM_PLUGIN_SDK_BUILD_ID_JSON)

SET(STELLARIUM_PLUGIN_SDK_HEADERS_CMAKE "")
SET(STELLARIUM_PLUGIN_SDK_HEADERS_JSON "")
FOREACH(_plugin_sdk_header IN LISTS STELLARIUM_PLUGIN_SDK_HEADERS)
     STRING(APPEND STELLARIUM_PLUGIN_SDK_HEADERS_CMAKE
          "\n\t\"${_plugin_sdk_header}\"")
     IF(STELLARIUM_PLUGIN_SDK_HEADERS_JSON)
          STRING(APPEND STELLARIUM_PLUGIN_SDK_HEADERS_JSON ",\n")
     ENDIF()
     STRING(APPEND STELLARIUM_PLUGIN_SDK_HEADERS_JSON
          "    \"${_plugin_sdk_header}\"")
ENDFOREACH()

SET(STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_CMAKE "")
SET(STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_JSON "")
LIST(SORT STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS)
FOREACH(_plugin_sdk_definition IN LISTS
     STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS)
     STRING(APPEND STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_CMAKE
          "\n\t\"${_plugin_sdk_definition}\"")
     IF(STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_JSON)
          STRING(APPEND STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_JSON ",\n")
     ENDIF()
     STRING(APPEND STELLARIUM_PLUGIN_SDK_COMPILE_DEFINITIONS_JSON
          "    \"${_plugin_sdk_definition}\"")
ENDFOREACH()

STRING(TOLOWER "${STELLARIUM_PLUGIN_ABI_CONFIGURATION}"
     STELLARIUM_PLUGIN_SDK_CONFIGURATION_LABEL)
SET(STELLARIUM_PLUGIN_SDK_ARCHIVE_BASENAME
     "stellarium-${VERSION}-plugin-sdk-windows-${STELLARIUM_PLUGIN_ABI_ARCHITECTURE}-msvc${MSVC_TOOLSET_VERSION}-qt${QT_VERSION}-${STELLARIUM_PLUGIN_SDK_CONFIGURATION_LABEL}-experimental")
SET(STELLARIUM_PLUGIN_SDK_ARCHIVE
     "${PROJECT_BINARY_DIR}/packages/${STELLARIUM_PLUGIN_SDK_ARCHIVE_BASENAME}.zip"
     CACHE INTERNAL "Path to the Windows plug-in SDK archive" FORCE)

CONFIGURE_PACKAGE_CONFIG_FILE(
     "${PROJECT_SOURCE_DIR}/cmake/StellariumConfig.cmake.in"
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumConfig.cmake"
     INSTALL_DESTINATION "${STELLARIUM_PLUGIN_SDK_CMAKEDIR}"
     PATH_VARS
          CMAKE_INSTALL_BINDIR
          CMAKE_INSTALL_LIBDIR
          STELLARIUM_PLUGIN_SDK_INCLUDEDIR
          STELLARIUM_PLUGIN_SDK_DATADIR)
WRITE_BASIC_PACKAGE_VERSION_FILE(
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumConfigVersion.cmake"
     VERSION "${VERSION}"
     COMPATIBILITY ExactVersion)
CONFIGURE_FILE(
     "${PROJECT_SOURCE_DIR}/cmake/StellariumPluginBuildInfo.json.in"
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumPluginBuildInfo.json"
     @ONLY)
CONFIGURE_FILE(
     "${PROJECT_SOURCE_DIR}/cmake/StellariumPluginSDK-README.md.in"
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumPluginSDK-README.md"
     @ONLY)

FOREACH(_plugin_sdk_header IN LISTS STELLARIUM_PLUGIN_SDK_HEADERS)
     GET_FILENAME_COMPONENT(_plugin_sdk_header_directory
          "${_plugin_sdk_header}" DIRECTORY)
     INSTALL(FILES "${PROJECT_SOURCE_DIR}/src/${_plugin_sdk_header}"
          DESTINATION "${STELLARIUM_PLUGIN_SDK_INCLUDEDIR}/${_plugin_sdk_header_directory}"
          CONFIGURATIONS Release
          COMPONENT ${STELLARIUM_PLUGIN_SDK_COMPONENT})
ENDFOREACH()
INSTALL(FILES
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumConfig.cmake"
     "${CMAKE_CURRENT_BINARY_DIR}/StellariumConfigVersion.cmake"
     DESTINATION "${STELLARIUM_PLUGIN_SDK_CMAKEDIR}"
     CONFIGURATIONS Release
     COMPONENT ${STELLARIUM_PLUGIN_SDK_COMPONENT})
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/StellariumPluginBuildInfo.json"
     DESTINATION "${STELLARIUM_PLUGIN_SDK_DATADIR}"
     CONFIGURATIONS Release
     COMPONENT ${STELLARIUM_PLUGIN_SDK_COMPONENT})
INSTALL(FILES "${CMAKE_CURRENT_BINARY_DIR}/StellariumPluginSDK-README.md"
     DESTINATION "."
     RENAME README.md
     CONFIGURATIONS Release
     COMPONENT ${STELLARIUM_PLUGIN_SDK_COMPONENT})
INSTALL(FILES "${PROJECT_SOURCE_DIR}/COPYING"
     DESTINATION "."
     CONFIGURATIONS Release
     COMPONENT ${STELLARIUM_PLUGIN_SDK_COMPONENT})

ADD_CUSTOM_TARGET(stellarium-plugin-sdk-archive
     COMMAND "${CMAKE_COMMAND}"
          "-DPLUGIN_SDK_BUILD_DIR=${PROJECT_BINARY_DIR}"
          "-DPLUGIN_SDK_STAGE_PARENT=${PROJECT_BINARY_DIR}/plugin-sdk-stage/$<CONFIG>"
          "-DPLUGIN_SDK_ARCHIVE_DIR=${PROJECT_BINARY_DIR}/packages"
          "-DPLUGIN_SDK_BASENAME=${STELLARIUM_PLUGIN_SDK_ARCHIVE_BASENAME}"
          "-DPLUGIN_SDK_CONFIGURATION=$<CONFIG>"
          "-DPLUGIN_SDK_COMPONENT=${STELLARIUM_PLUGIN_SDK_COMPONENT}"
          "-DPLUGIN_SDK_SOURCE_DIR=${PROJECT_SOURCE_DIR}"
          -P "${PROJECT_SOURCE_DIR}/cmake/BuildPluginSDKArchive.cmake"
     COMMENT "Creating the relocatable Stellarium plug-in SDK archive"
     VERBATIM
     USES_TERMINAL)
ADD_DEPENDENCIES(stellarium-plugin-sdk-archive stelMain)
SET_TARGET_PROPERTIES(stellarium-plugin-sdk-archive PROPERTIES
     FOLDER "packaging")
