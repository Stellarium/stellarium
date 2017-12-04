cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_mgenautoguider C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(INDI_MGENAUTOGUIDER_VERSION_MAJOR 0)
set(INDI_MGENAUTOGUIDER_VERSION_MINOR 1)

find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(FTDI1 REQUIRED)
find_package(USB1 REQUIRED)
find_package(Threads REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_mgenautoguider.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_mgenautoguider.xml )

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${FTDI1_INCLUDE_DIR})
include_directories(${USB1_INCLUDE_DIRS})

include(CMakeCommon)

########### MGenAutoguider ###########
set(indimgenautoguider_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/mgenautoguider.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/mgen_device.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indimgen indimgenautoguider_SRCS 10 cpp)
ENDIF ()

add_executable(indi_mgenautoguider ${indimgenautoguider_SRCS})

target_link_libraries(indi_mgenautoguider ${INDI_DRIVER_LIBRARIES} ${FTDI1_LIBRARIES} ${USB1_LIBRARIES} pthread)

install(TARGETS indi_mgenautoguider RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_mgenautoguider.xml DESTINATION ${INDI_DATA_DIR})

