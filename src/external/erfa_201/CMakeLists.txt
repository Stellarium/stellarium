cmake_minimum_required(VERSION 3.10)
project(erfa_201 C)

# Collect all ERFA C sources
file(GLOB ERFA_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

# Exclude test files
list(REMOVE_ITEM ERFA_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/t_erfa_c.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/t_erfa_c_extra.c"
)

# Build as a static library
add_library(erfa_201 STATIC ${ERFA_SOURCES})

# Expose headers
target_include_directories(erfa_201 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

set_target_properties(erfa_201 PROPERTIES AUTOMOC 0)
set_target_properties(erfa_201 PROPERTIES FOLDER "src/external")