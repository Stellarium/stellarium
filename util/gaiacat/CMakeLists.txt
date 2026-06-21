cmake_minimum_required(VERSION 3.16)
project(skychart2cat LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_FILE_OFFSET_BITS=64)

add_executable(skychart2cat
	main.cpp
	convert.cpp
	geodesic.cpp
	bucket.cpp
	cat_writer.cpp
)
target_include_directories(skychart2cat PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(dedupcat
	dedup_cat.cpp
)

add_executable(cmpcat
	cmpcat.cpp
)

# Link pthread on Linux
if(UNIX AND NOT APPLE)
	target_link_libraries(skychart2cat PRIVATE pthread)
endif()

# MSVC: use static runtime for portability
if(MSVC)
	set_property(TARGET skychart2cat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
