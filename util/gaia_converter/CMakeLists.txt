cmake_minimum_required(VERSION 3.16)
project(gaia_converter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_FILE_OFFSET_BITS=64)

add_executable(gaia_converter
	main.cpp
	convert.cpp
	geodesic.cpp
	bucket.cpp
	cat_writer.cpp
)

target_include_directories(gaia_converter PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Link pthread on Linux
if(UNIX AND NOT APPLE)
	target_link_libraries(gaia_converter PRIVATE pthread)
endif()

# MSVC: use static runtime for portability
if(MSVC)
	set_property(TARGET gaia_converter PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
