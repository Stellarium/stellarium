cmake_minimum_required(VERSION 2.8)
PROJECT(indi_maxdomeii C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(INDI_MAXDOMEII_VERSION_MAJOR 1)
set(INDI_MAXDOMEII_VERSION_MINOR 1)

find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Nova REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_maxdomeii.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_maxdomeii.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})

include(CMakeCommon)

########### MaxDome II ###########
set(indimaxdomeii_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/maxdomeiidriver.c
   ${CMAKE_CURRENT_SOURCE_DIR}/maxdomeii.cpp
   )

add_executable(indi_maxdomeii ${indimaxdomeii_SRCS})

target_link_libraries(indi_maxdomeii ${INDI_LIBRARIES} ${NOVA_LIBRARIES})

install(TARGETS indi_maxdomeii RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_maxdomeii.xml DESTINATION ${INDI_DATA_DIR})
