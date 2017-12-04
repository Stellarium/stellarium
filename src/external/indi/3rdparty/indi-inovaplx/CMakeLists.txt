cmake_minimum_required(VERSION 2.7)
PROJECT(indi_inovaplx CXX C)

set (INOVAPLX_VERSION_MAJOR 1)
set (INOVAPLX_VERSION_MINOR 4)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(INOVASDK REQUIRED)
find_package(Threads REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_inovaplx_ccd.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_inovaplx_ccd.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${INOVASDK_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})

include(CMakeCommon)

############# INOVAPLX CCD ###############
set(inovaplxccd_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/inovaplx_ccd.cpp
)

add_executable(indi_inovaplx_ccd ${inovaplxccd_SRCS})

target_link_libraries(indi_inovaplx_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${INOVASDK_LIBRARIES} ${M_LIB} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS indi_inovaplx_ccd RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_inovaplx_ccd.xml DESTINATION ${INDI_DATA_DIR})
