cmake_minimum_required(VERSION 2.8)
PROJECT(indi_asi CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(USB1 REQUIRED)
find_package(Threads REQUIRED)

# Using ASI Camera SDK Version 0.6.0921 updated on 2017-11-21
# Using ASI EFW SDK Version 3.0214 updated on 2017-11-21
set(ASI_VERSION_MAJOR 0)
set(ASI_VERSION_MINOR 7)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_asi.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_asi.xml)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})

include(CMakeCommon)

if (APPLE)
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/mac/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/mac/libEFWFilter.bin)
    message("-- Using Mac static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv5l")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv5/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv5/libEFWFilter.bin)
    add_definitions(-DLOW_USB_BANDWIDTH)
    message("-- Using armv5 static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv6/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv6/libEFWFilter.bin)
    add_definitions(-DLOW_USB_BANDWIDTH)
    message("-- Using armv6 static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv7/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv7/libEFWFilter.bin)
    add_definitions(-DLOW_USB_BANDWIDTH)
    message("-- Using armv7 static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv8/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/armv8/libEFWFilter.bin)
    add_definitions(-DLOW_USB_BANDWIDTH)
    message("-- Using armv8 static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/x64/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/x64/libEFWFilter.bin)
    message("-- Using 64 bit static ASI library")
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(ASICAMLIB ${CMAKE_CURRENT_SOURCE_DIR}/x86/libASICamera2.bin)
    set(ASIFWLIB ${CMAKE_CURRENT_SOURCE_DIR}/x86/libEFWFilter.bin)
    message("-- Using 32 bit static ASI library")
endif ()

########### indi_asi_ccd ###########
set(indi_asi_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/asi_ccd.cpp
   )

add_executable(indi_asi_ccd ${indi_asi_SRCS})

target_link_libraries(indi_asi_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${ASICAMLIB} ${USB1_LIBRARIES} ${ZLIB_LIBRARY} ${CMAKE_THREAD_LIBS_INIT})


########### indi_asi_wheel ###########
add_executable(indi_asi_wheel ${CMAKE_CURRENT_SOURCE_DIR}/asi_wheel.cpp)
IF (APPLE)
set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation")
target_link_libraries(indi_asi_wheel ${INDI_LIBRARIES} ${ASIFWLIB} ${LIBUSB_LIBRARIES})
ELSE()
target_link_libraries(indi_asi_wheel ${INDI_LIBRARIES} ${ASIFWLIB} ${USB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF()


if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
target_link_libraries(indi_asi_ccd rt)
target_link_libraries(indi_asi_wheel rt)
endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

install(TARGETS indi_asi_ccd RUNTIME DESTINATION bin)
install(TARGETS indi_asi_wheel RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_asi.xml DESTINATION ${INDI_DATA_DIR})

