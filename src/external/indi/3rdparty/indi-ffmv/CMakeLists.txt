cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi-ffmv CXX C)

set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(DC1394 REQUIRED)

set (FFMV_VERSION_MAJOR 0)
set (FFMV_VERSION_MINOR 1)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_ffmv.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_ffmv.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${DC1394_INCLUDE_DIR})

include(CMakeCommon)

########### QSI ###########
set(indiffmv_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/ffmv_ccd.cpp
   )

add_executable(indi_ffmv_ccd ${indiffmv_SRCS})

target_link_libraries(indi_ffmv_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${DC1394_LIBRARIES} )

install(TARGETS indi_ffmv_ccd RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_ffmv.xml DESTINATION ${INDI_DATA_DIR})
install(FILES 99-fireflymv.rules DESTINATION ${UDEVRULES_INSTALL_DIR})

