CMAKE_MINIMUM_REQUIRED(VERSION 3.18)

FOREACH(_required_variable
     PLUGIN_SDK_BUILD_DIR
     PLUGIN_SDK_STAGE_PARENT
     PLUGIN_SDK_ARCHIVE_DIR
     PLUGIN_SDK_BASENAME
     PLUGIN_SDK_CONFIGURATION
     PLUGIN_SDK_COMPONENT
     PLUGIN_SDK_SOURCE_DIR)
     IF(NOT DEFINED ${_required_variable} OR
          "${${_required_variable}}" STREQUAL "")
          MESSAGE(FATAL_ERROR
               "BuildPluginSDKArchive.cmake requires ${_required_variable}")
     ENDIF()
ENDFOREACH()

IF(NOT PLUGIN_SDK_CONFIGURATION STREQUAL "Release")
     MESSAGE(FATAL_ERROR
          "Build stellarium-plugin-sdk-archive with --config Release")
ENDIF()
IF(NOT PLUGIN_SDK_BASENAME MATCHES "^[A-Za-z0-9][A-Za-z0-9._+-]*$")
     MESSAGE(FATAL_ERROR "PLUGIN_SDK_BASENAME contains an unsupported character")
ENDIF()
STRING(FIND "${PLUGIN_SDK_BASENAME}" ".." _parent_reference)
IF(PLUGIN_SDK_BASENAME STREQUAL "." OR NOT _parent_reference EQUAL -1)
     MESSAGE(FATAL_ERROR "PLUGIN_SDK_BASENAME must not contain a parent reference")
ENDIF()

GET_FILENAME_COMPONENT(_build_dir "${PLUGIN_SDK_BUILD_DIR}" ABSOLUTE)
GET_FILENAME_COMPONENT(_stage_parent "${PLUGIN_SDK_STAGE_PARENT}" ABSOLUTE)
GET_FILENAME_COMPONENT(_archive_dir "${PLUGIN_SDK_ARCHIVE_DIR}" ABSOLUTE)
GET_FILENAME_COMPONENT(_source_dir "${PLUGIN_SDK_SOURCE_DIR}" ABSOLUTE)
FILE(TO_CMAKE_PATH "${_build_dir}" _build_dir)
FILE(TO_CMAKE_PATH "${_stage_parent}" _stage_parent)
FILE(TO_CMAKE_PATH "${_archive_dir}" _archive_dir)
FILE(TO_CMAKE_PATH "${_source_dir}" _source_dir)

SET(_build_prefix "${_build_dir}/")
STRING(FIND "${_stage_parent}/" "${_build_prefix}" _stage_prefix)
STRING(FIND "${_archive_dir}/" "${_build_prefix}" _archive_prefix)
IF(_stage_parent STREQUAL _build_dir OR NOT _stage_prefix EQUAL 0)
     MESSAGE(FATAL_ERROR
          "PLUGIN_SDK_STAGE_PARENT must be below PLUGIN_SDK_BUILD_DIR")
ENDIF()
IF(_archive_dir STREQUAL _build_dir OR NOT _archive_prefix EQUAL 0)
     MESSAGE(FATAL_ERROR
          "PLUGIN_SDK_ARCHIVE_DIR must be below PLUGIN_SDK_BUILD_DIR")
ENDIF()

INCLUDE("${_source_dir}/cmake/StellariumPluginSDKHeaders.cmake")
IF(NOT STELLARIUM_PLUGIN_SDK_HEADERS)
     MESSAGE(FATAL_ERROR "The plug-in SDK header manifest is empty")
ENDIF()
SET(_seen_headers "")
FOREACH(_header IN LISTS STELLARIUM_PLUGIN_SDK_HEADERS)
     IF(IS_ABSOLUTE "${_header}" OR
          _header MATCHES "(^|/)\\.\\.(/|$)" OR
          _header MATCHES "\\\\" OR
          NOT _header MATCHES "^[A-Za-z0-9_./+-]+\\.hpp$")
          MESSAGE(FATAL_ERROR "Invalid plug-in SDK header path: ${_header}")
     ENDIF()
     IF(_header IN_LIST _seen_headers)
          MESSAGE(FATAL_ERROR "Duplicate plug-in SDK header: ${_header}")
     ENDIF()
     LIST(APPEND _seen_headers "${_header}")
ENDFOREACH()
UNSET(_seen_headers)

SET(_sdk_root "${_stage_parent}/${PLUGIN_SDK_BASENAME}")
SET(_sdk_archive "${_archive_dir}/${PLUGIN_SDK_BASENAME}.zip")
SET(_sdk_archive_checksum "${_sdk_archive}.sha256")
SET(_payload_files
     "COPYING"
     "README.md"
     "bin/stelMain.dll"
     "lib/cmake/Stellarium/StellariumConfig.cmake"
     "lib/cmake/Stellarium/StellariumConfigVersion.cmake"
     "lib/stelMain.lib"
     "share/stellarium/plugin-sdk/StellariumPluginBuildInfo.json")
FOREACH(_header IN LISTS STELLARIUM_PLUGIN_SDK_HEADERS)
     LIST(APPEND _payload_files "include/stellarium/${_header}")
ENDFOREACH()
LIST(SORT _payload_files)

FILE(REMOVE_RECURSE "${_sdk_root}")
FILE(REMOVE "${_sdk_archive}" "${_sdk_archive_checksum}")
FILE(MAKE_DIRECTORY "${_sdk_root}" "${_archive_dir}")

EXECUTE_PROCESS(
     COMMAND "${CMAKE_COMMAND}" --install "${_build_dir}"
          --config "${PLUGIN_SDK_CONFIGURATION}"
          --component "${PLUGIN_SDK_COMPONENT}"
          --prefix "${_sdk_root}"
     RESULT_VARIABLE _install_result)
IF(NOT _install_result EQUAL 0)
     MESSAGE(FATAL_ERROR
          "Plug-in SDK component install failed: ${_install_result}")
ENDIF()

FOREACH(_relative_path IN LISTS _payload_files)
     SET(_payload_path "${_sdk_root}/${_relative_path}")
     IF(NOT EXISTS "${_payload_path}" OR IS_DIRECTORY "${_payload_path}")
          MESSAGE(FATAL_ERROR
               "Plug-in SDK payload file is missing: ${_relative_path}")
     ENDIF()
     FILE(SIZE "${_payload_path}" _payload_size)
     IF(_payload_size EQUAL 0)
          MESSAGE(FATAL_ERROR
               "Plug-in SDK payload file is empty: ${_relative_path}")
     ENDIF()
ENDFOREACH()

EXECUTE_PROCESS(
     COMMAND "${CMAKE_COMMAND}" -E sha256sum ${_payload_files}
     WORKING_DIRECTORY "${_sdk_root}"
     OUTPUT_FILE "${_sdk_root}/MANIFEST.sha256"
     RESULT_VARIABLE _manifest_result)
IF(NOT _manifest_result EQUAL 0)
     MESSAGE(FATAL_ERROR
          "Plug-in SDK checksum manifest failed: ${_manifest_result}")
ENDIF()
FILE(SIZE "${_sdk_root}/MANIFEST.sha256" _manifest_size)
IF(_manifest_size EQUAL 0)
     MESSAGE(FATAL_ERROR "Plug-in SDK checksum manifest is empty")
ENDIF()

SET(_expected_files ${_payload_files} "MANIFEST.sha256")
FILE(GLOB_RECURSE _actual_files
     LIST_DIRECTORIES FALSE
     RELATIVE "${_sdk_root}"
     "${_sdk_root}/*")
LIST(SORT _expected_files)
LIST(SORT _actual_files)
IF(NOT "${_actual_files}" STREQUAL "${_expected_files}")
     MESSAGE(FATAL_ERROR
          "Unexpected plug-in SDK payload. Expected: ${_expected_files}; actual: ${_actual_files}")
ENDIF()

EXECUTE_PROCESS(
     COMMAND "${CMAKE_COMMAND}" -E tar cf "${_sdk_archive}"
          --format=zip -- "${PLUGIN_SDK_BASENAME}"
     WORKING_DIRECTORY "${_stage_parent}"
     RESULT_VARIABLE _archive_result)
IF(NOT _archive_result EQUAL 0 OR NOT EXISTS "${_sdk_archive}")
     MESSAGE(FATAL_ERROR "Plug-in SDK archive creation failed")
ENDIF()
FILE(SIZE "${_sdk_archive}" _archive_size)
IF(_archive_size EQUAL 0)
     MESSAGE(FATAL_ERROR "Plug-in SDK archive is empty")
ENDIF()

GET_FILENAME_COMPONENT(_archive_name "${_sdk_archive}" NAME)
EXECUTE_PROCESS(
     COMMAND "${CMAKE_COMMAND}" -E sha256sum "${_archive_name}"
     WORKING_DIRECTORY "${_archive_dir}"
     OUTPUT_FILE "${_sdk_archive_checksum}"
     RESULT_VARIABLE _archive_hash_result)
IF(NOT _archive_hash_result EQUAL 0)
     MESSAGE(FATAL_ERROR "Plug-in SDK archive checksum failed")
ENDIF()
FILE(SIZE "${_sdk_archive_checksum}" _archive_hash_size)
IF(_archive_hash_size EQUAL 0)
     MESSAGE(FATAL_ERROR "Plug-in SDK archive checksum is empty")
ENDIF()

MESSAGE(STATUS "Created plug-in SDK archive: ${_sdk_archive}")
