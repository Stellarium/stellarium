cmake_minimum_required(VERSION 2.8)
PROJECT(indi_nexdome C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(INDI_NEXDOME_VERSION_MAJOR 1)
set(INDI_NEXDOME_VERSION_MINOR 0)

find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Nova REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_nexdome.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_nexdome.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})

include(CMakeCommon)

########### MaxDome II ###########
set(indi_nexdome_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/nex_dome.cpp
   )

add_executable(indi_nexdome ${indi_nexdome_SRCS})

target_link_libraries(indi_nexdome ${INDI_LIBRARIES} ${NOVA_LIBRARIES})

install(TARGETS indi_nexdome RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_nexdome.xml DESTINATION ${INDI_DATA_DIR})
