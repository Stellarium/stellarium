cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_tess CXX C)
Include (CheckCXXSourceCompiles)
include(GNUInstallDirs)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")

set (TESS_VERSION_MAJOR 0)
set (TESS_VERSION_MINOR 1)
 
find_package(INDI REQUIRED)
find_package(Nova REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_tess.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_tess.xml )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})

include(CMakeCommon)

##################### indi arduino #####################
set(inditess_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/tess.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/tess_algebra.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(inditess inditess_SRCS 10 cpp)
ENDIF ()

add_executable(indi_tess ${inditess_SRCS})
target_link_libraries(indi_tess ${INDI_LIBRARIES} ${NOVA_LIBRARIES})

install(TARGETS indi_tess RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_tess.xml DESTINATION ${INDI_DATA_DIR})
install(FILES tess_sk.xml DESTINATION ${INDI_DATA_DIR})

