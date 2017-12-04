cmake_minimum_required(VERSION 2.4.7)
PROJECT(aagcloudwatcher CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set (AAG_VERSION_MAJOR 1)
set (AAG_VERSION_MINOR 3)

find_package(INDI REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_aagcloudwatcher.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_aagcloudwatcher.xml)

include(CMakeCommon)

set(indiaag_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/indi_aagcloudwatcher.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/CloudWatcherController.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indiaag indiaag_SRCS 10 cpp)
ENDIF ()

add_executable(indi_aagcloudwatcher ${indiaag_SRCS})
target_link_libraries(indi_aagcloudwatcher ${INDI_LIBRARIES})

set(test_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/CloudWatcherController.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(test test_SRCS 10 cpp)
ENDIF ()

add_executable(aagcloudwatcher_test ${test_SRCS})

install(TARGETS indi_aagcloudwatcher RUNTIME DESTINATION bin)
install(TARGETS aagcloudwatcher_test RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_aagcloudwatcher.xml DESTINATION ${INDI_DATA_DIR})
install(FILES indi_aagcloudwatcher_sk.xml DESTINATION ${INDI_DATA_DIR})
