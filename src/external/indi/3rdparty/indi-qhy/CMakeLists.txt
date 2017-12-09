cmake_minimum_required(VERSION 2.8)
PROJECT(indi_qhy CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(QHY REQUIRED)
find_package(ZLIB REQUIRED)
find_package(USB1 REQUIRED)
find_package(Threads REQUIRED)

set(INDI_QHY_VERSION_MAJOR 1)
set(INDI_QHY_VERSION_MINOR 7)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_qhy.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_qhy.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${QHY_INCLUDE_DIR})

include(CMakeCommon)

########### QHY CCD ###########

IF (APPLE)
    SET(indiqhy_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/qhy_ccd.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/qhy_fw.cpp)
ELSE ()
    SET(indiqhy_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/qhy_ccd.cpp)
    # Force linking all referenced libraries because the recent libqhy versions are not linked against libpthread
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--no-as-needed")
ENDIF ()

add_executable(indi_qhy_ccd ${indiqhy_SRCS})

target_link_libraries(indi_qhy_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${QHY_LIBRARIES} ${USB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
target_link_libraries(indi_qhy_ccd rt)
endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

install(TARGETS indi_qhy_ccd RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_qhy.xml DESTINATION ${INDI_DATA_DIR})

########### qhy_test_ccd ###########
add_executable(qhy_ccd_test ${CMAKE_CURRENT_SOURCE_DIR}/qhy_ccd_test.cpp)
target_link_libraries(qhy_ccd_test ${QHY_LIBRARIES} ${USB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
    target_link_libraries(qhy_ccd_test rt)
endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

install(TARGETS qhy_ccd_test RUNTIME DESTINATION bin )

