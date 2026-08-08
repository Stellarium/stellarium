cmake_minimum_required(VERSION 3.16)
project(gaia2cat LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-D_FILE_OFFSET_BITS=64)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Gaia DR3 .bin → Stellarium .cat converter
add_executable(gaia2cat
	src/main.cpp
	src/convert.cpp
	src/geodesic.cpp
	src/bucket.cpp
	src/cat_writer.cpp
)

add_executable(dedupcat
	src/dedup_cat.cpp
)

add_executable(cmpcat
	src/cmpcat.cpp
)

add_executable(bvextract
	src/bvextract.cpp
)

add_executable(apply_bv
	src/apply_bv.cpp
)

add_executable(gaiahip2cat
	src/gaiahip2cat.cpp
	src/convert.cpp
	src/geodesic.cpp
)

# CSV.gz -> .bin extractor (uses bundled zlib for gzip reading)
set(GAIA_ZLIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../src/external/zlib)
add_executable(extract_columns
	src/extract_columns.cpp
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
	target_link_libraries(gaia2cat PRIVATE pthread)
endif()

# MSVC: use static runtime for portability
if(MSVC)
	set_property(TARGET gaia2cat PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
