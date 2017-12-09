########### NexStar Evolution INDI driver ##############
PROJECT(indi_gpsd C CXX)
cmake_minimum_required(VERSION 2.4.7)

include(GNUInstallDirs)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
set(INDI_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/indi")

find_package(INDI REQUIRED)
find_package(Nova REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GPSD REQUIRED)

set(GPSD_VERSION_MAJOR 0)
set(GPSD_VERSION_MINOR 4)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_gpsd.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_gpsd.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})

include(CMakeCommon)

add_executable(indi_gpsd gps_driver.cpp)
target_link_libraries(indi_gpsd ${INDI_LIBRARIES} ${GPSD_LIBRARIES} ${NOVA_LIBRARIES})
install(TARGETS indi_gpsd RUNTIME DESTINATION bin )

install( FILES  ${CMAKE_CURRENT_BINARY_DIR}/indi_gpsd.xml DESTINATION ${INDI_DATA_DIR})
