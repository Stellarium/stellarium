cmake_minimum_required(VERSION 3.18)

if (POLICY CMP0091)
	cmake_policy(SET CMP0091 NEW)
endif()

project(StellariumPluginSDKProbe LANGUAGES CXX)

if (NOT WIN32 OR NOT MSVC)
	message(FATAL_ERROR "The Stellarium plug-in SDK probe requires Windows and MSVC.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreadedDLL)

if (CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES Release CACHE STRING
		"Configurations supported by the Stellarium plug-in SDK" FORCE)
elseif (NOT CMAKE_BUILD_TYPE STREQUAL "Release")
	message(FATAL_ERROR "The Stellarium plug-in SDK probe must be built as Release.")
endif()

find_package(Stellarium CONFIG REQUIRED)

set(_expected_headers
	"StelMainExport.hpp"
	"core/StelModule.hpp"
	"core/StelPluginInterface.hpp")
if (NOT "${Stellarium_PLUGIN_SDK_HEADERS}" STREQUAL "${_expected_headers}")
	message(FATAL_ERROR "The installed SDK has an unexpected header surface.")
endif()

set(_header_probe_sources)
foreach (_header IN LISTS Stellarium_PLUGIN_SDK_HEADERS)
	string(MAKE_C_IDENTIFIER "${_header}" _header_id)
	set(_header_source
		"${CMAKE_CURRENT_BINARY_DIR}/sdk-header-${_header_id}.cpp")
	file(WRITE "${_header_source}" "#include <${_header}>\n")
	list(APPEND _header_probe_sources "${_header_source}")
endforeach()
add_library(StellariumSDKHeaderProbe OBJECT ${_header_probe_sources})
target_link_libraries(StellariumSDKHeaderProbe PRIVATE Stellarium::stelMain)
set_target_properties(StellariumSDKHeaderProbe PROPERTIES AUTOMOC OFF)

add_library(DynamicPluginProbe MODULE
	DynamicPluginProbe.cpp
	DynamicPluginProbe.hpp)
target_link_libraries(DynamicPluginProbe PRIVATE Stellarium::stelMain)
set_target_properties(DynamicPluginProbe PROPERTIES
	PREFIX "lib"
	LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")

add_executable(StellariumPluginProbeLoader ProbeLoader.cpp)
target_link_libraries(StellariumPluginProbeLoader PRIVATE Stellarium::stelMain)
target_compile_definitions(StellariumPluginProbeLoader PRIVATE
	STELLARIUM_EXPECTED_VERSION="${Stellarium_VERSION}")
set_target_properties(StellariumPluginProbeLoader PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
