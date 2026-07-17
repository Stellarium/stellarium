cmake_minimum_required(VERSION 3.16)
project(skychart2cat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_FILE_OFFSET_BITS=64)

# Gaia DR3 .bin → Stellarium .cat converter
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

# CSV.gz -> .bin extractor (uses bundled zlib for gzip reading)
set(GAIA_ZLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/external/zlib)
add_executable(extract_columns
	extract_columns.cpp
	${GAIA_ZLIB_DIR}/inflate.c
	${GAIA_ZLIB_DIR}/inftrees.c
	${GAIA_ZLIB_DIR}/inffast.c
	${GAIA_ZLIB_DIR}/adler32.c
	${GAIA_ZLIB_DIR}/crc32.c
	${GAIA_ZLIB_DIR}/zutil.c
)
target_include_directories(extract_columns PRIVATE ${GAIA_ZLIB_DIR})

# Link pthread on Linux
if(UNIX AND NOT APPLE)
	target_link_libraries(skychart2cat PRIVATE pthread)
endif()

# MSVC: use static runtime for portability
if(MSVC)
	set_property(TARGET skychart2cat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
