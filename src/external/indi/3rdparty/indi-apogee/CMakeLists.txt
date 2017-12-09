cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-apogee CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set (APOGEE_VERSION_MAJOR 1)
set (APOGEE_VERSION_MINOR 5)

set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(APOGEE REQUIRED)
find_package(ZLIB REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_apogee.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_apogee.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${APOGEE_INCLUDE_DIR})

include(CMakeCommon)

########### QSI ###########
set(indiapogee_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/apogee_ccd.cpp
   )

add_executable(indi_apogee_ccd ${indiapogee_SRCS})

target_link_libraries(indi_apogee_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${APOGEE_LIBRARY})

install(TARGETS indi_apogee_ccd RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_apogee.xml DESTINATION ${INDI_DATA_DIR})
