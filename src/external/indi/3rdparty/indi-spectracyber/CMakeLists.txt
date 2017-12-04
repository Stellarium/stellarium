cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_spectracyber C CXX)

set(CMAKE_CXX_FLAGS "-std=c++0x ${CMAKE_CXX_FLAGS}")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

include(GNUInstallDirs)


find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(Threads REQUIRED)
find_package(Nova REQUIRED)

set (SPECTRACYBER_VERSION_MAJOR 1)
set (SPECTRACYBER_VERSION_MINOR 2)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_spectracyber.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_spectracyber.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR}/..)

########### SpectraCyber ###########
set(indispectracyber_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/spectracyber.cpp
   )

include(CMakeCommon)

add_executable(indi_spectracyber ${indispectracyber_SRCS})

target_link_libraries(indi_spectracyber ${INDI_LIBRARIES} ${NOVA_LIBRARIES} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})

install(TARGETS indi_spectracyber RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_spectracyber.xml indi_spectracyber_sk.xml DESTINATION ${INDI_DATA_DIR})

