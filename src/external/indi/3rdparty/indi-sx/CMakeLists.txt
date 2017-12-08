cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_sx CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)
set(RULES_INSTALL_DIR "/lib/udev/rules.d/")

set (VERSION_MAJOR 1)
set (VERSION_MINOR 12)

configure_file (
  "${CMAKE_CURRENT_SOURCE_DIR}/sxconfig.h.in"
  "${CMAKE_CURRENT_SOURCE_DIR}/sxconfig.h"
  )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_sx.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_sx.xml )

find_package(CFITSIO REQUIRED)
find_package(USB1 REQUIRED)
find_package(INDI REQUIRED)
find_package(Threads REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})

include(CMakeCommon)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-usb.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-usb.h)

set(indisxccd_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/sxccd.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/sxccdusb.cpp
   )

add_executable(indi_sx_ccd ${indisxccd_SRCS})
target_link_libraries(indi_sx_ccd ${INDI_LIBRARIES} ${USB1_LIBRARIES})

#IF (APPLE)
#set(indisxwheel_SRCS
#  ${CMAKE_CURRENT_SOURCE_DIR}/sxwheel.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/hid_mac.c
#  )
#ELSEIF (WIN32)
#set(indisxwheel_SRCS
#  ${CMAKE_CURRENT_SOURCE_DIR}/sxwheel.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/hid_win.c
#  )
#ELSE ()
#include_directories(${USB1_INCLUDE_DIRS})
#set(indisxwheel_SRCS
#  ${CMAKE_CURRENT_SOURCE_DIR}/sxwheel.cpp
#  ${CMAKE_CURRENT_SOURCE_DIR}/hid_libusb.c
#  )
#ENDIF()

#add_executable(indi_sx_wheel ${indisxwheel_SRCS})
add_executable(indi_sx_wheel ${CMAKE_CURRENT_SOURCE_DIR}/sxwheel.cpp)
IF (APPLE)
set(CMAKE_EXE_LINKER_FLAGS "-framework IOKit -framework CoreFoundation")
target_link_libraries(indi_sx_wheel ${INDI_LIBRARIES} ${LIBUSB_LIBRARIES})
ELSEIF (WIN32)
ELSE ()
target_link_libraries(indi_sx_wheel ${INDI_LIBRARIES} ${USB1_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
ENDIF()

if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
target_link_libraries(indi_sx_wheel rt)
endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

set(indisxao_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/sxao.cpp
   )

add_executable(indi_sx_ao ${indisxao_SRCS})
target_link_libraries(indi_sx_ao ${INDI_LIBRARIES})

set(sx_ccd_test_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/sxccdtest.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/sxccdusb.cpp
   )

add_executable(sx_ccd_test ${sx_ccd_test_SRCS})
target_link_libraries(sx_ccd_test ${USB1_LIBRARIES})

install(TARGETS indi_sx_ccd RUNTIME DESTINATION bin)
install(TARGETS indi_sx_wheel RUNTIME DESTINATION bin)
install(TARGETS indi_sx_ao RUNTIME DESTINATION bin)
install(TARGETS sx_ccd_test RUNTIME DESTINATION bin)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_sx.xml DESTINATION ${INDI_DATA_DIR})

set (CPACK_PACKAGE_NAME "sxccd")
set (CPACK_PACKAGE_VENDOR "CloudMakers, s. r. o.")
set (CPACK_PACKAGE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}")
set (CPACK_PACKAGE_CONTACT "Peter Polakovic <peter.polakovic@cloudmakers.eu>")
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "INDI Driver for SX CCD, FW wheel and AO")
set (CPACK_PACKAGE_DESCRIPTION "This driver for INDI 0.9.7 supports SXVF-xxx, SXVR-xxx, LodeStar, CoStar and SuperStar cameras with full cooler and guider port control, USB Filter Wheel and Active Optics device.")

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  file (WRITE "README.txt" "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\nVersion ${CPACK_PACKAGE_VERSION}\n\n${CPACK_PACKAGE_DESCRIPTION}\n\nVisit http://www.cloudmakers.eu/indi to learn more.")
  set (CPACK_GENERATOR "PackageMaker")
  set (CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYRIGHT.txt")
  set (CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.txt")
  set (CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")
  set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  install(FILES 99-sx.rules DESTINATION ${RULES_INSTALL_DIR})
  set (CPACK_DEBIAN_PACKAGE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}\n  ${CPACK_PACKAGE_DESCRIPTION}")
  set (CPACK_RPM_PACKAGE_DESCRIPTION ${CPACK_PACKAGE_DESCRIPTION})
  set (CPACK_GENERATOR "DEB;RPM")
  set (CPACK_DEBIAN_PACKAGE_DEPENDS "indi-bin (>= 0.9.7)")
  set (CPACK_DEBIAN_PACKAGE_SECTION "Science")
  set (CPACK_RPM_PACKAGE_DEPENDS "indi-bin >= 0.9.7")
  set (CPACK_RPM_PACKAGE_GROUP "Applications/Science")
  set (CPACK_PACKAGING_INSTALL_PREFIX "/usr")
  if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm.*")
    set (CPACK_GENERATOR "DEB")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "armhf")
    set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    set (CPACK_GENERATOR "DEB;RPM")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
    set (CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set (CPACK_GENERATOR "DEB;RPM")
    set (CPACK_DEBIAN_PACKAGE_ARCHITECTURE "i386")
    set (CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
    set (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}")
  endif ()
endif ()

include (CPack)
