cmake_minimum_required(VERSION 2.8)
PROJECT(indi_miccd CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(USB1 REQUIRED)
find_package(Threads REQUIRED)

set(INDI_MI_VERSION_MAJOR 1)
set(INDI_MI_VERSION_MINOR 3)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_miccd.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_miccd.xml)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${INDI_INCLUDE_DIR})
include_directories(${CFITSIO_INCLUDE_DIR})

if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
    set(MILIB ${CMAKE_CURRENT_SOURCE_DIR}/libgxccdarmv6.bin)
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
    set(MILIB ${CMAKE_CURRENT_SOURCE_DIR}/libgxccdarmv7.bin)
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(MILIB ${CMAKE_CURRENT_SOURCE_DIR}/libgxccdarmv8.bin)
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(MILIB ${CMAKE_CURRENT_SOURCE_DIR}/libgxccd64.bin)
elseif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i[3-6]86")
    set(MILIB ${CMAKE_CURRENT_SOURCE_DIR}/libgxccd32.bin)
endif ()

include(CMakeCommon)

########### MI CCD ###########
set(indi_miccd_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/mi_ccd.cpp
   )

add_executable(indi_mi_ccd ${indi_miccd_SRCS})

target_link_libraries(indi_mi_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES}
                      ${MILIB} ${USB1_LIBRARIES} rt)

if(THREADS_HAVE_PTHREAD_ARG)
  target_compile_options(indi_mi_ccd "-pthread")
endif()
if(CMAKE_THREAD_LIBS_INIT)
  target_link_libraries(indi_mi_ccd "${CMAKE_THREAD_LIBS_INIT}")
endif()

install(TARGETS indi_mi_ccd RUNTIME DESTINATION bin)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_mi_ccd_symlink.cmake
"exec_program(\"${CMAKE_COMMAND}\" ARGS -E create_symlink indi_mi_ccd \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/indi_mi_ccd_usb)\n
 exec_program(\"${CMAKE_COMMAND}\" ARGS -E create_symlink indi_mi_ccd \$ENV{DESTDIR}${CMAKE_INSTALL_FULL_BINDIR}/indi_mi_ccd_eth)\n")

set_target_properties(indi_mi_ccd PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_mi_ccd_symlink.cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_miccd.xml DESTINATION ${INDI_DATA_DIR})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/99-miccd.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
