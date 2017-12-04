cmake_minimum_required(VERSION 2.8)
PROJECT(libinovasdk C CXX)

set(INOVASDK_VERSION "1.3.2")
set(INOVASDK_SOVERSION "1")

set(FIRMWARE_INSTALL_DIR "/lib/firmware")
set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

IF(UNIX AND NOT WIN32 AND NOT APPLE)
 IF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libinovasdk.so.armv6 ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION})
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libinovasdk.so.armv7 ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION})
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libinovasdk.so.armv8 ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION})
 ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
  exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libinovasdk.so.x64 ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION})
 ELSE()
  exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libinovasdk.so.x86 ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION})
 ENDIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE)

# Make sure symbolic links are installed
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_inovasdk_symlink.cmake "
exec_program(${CMAKE_COMMAND} ARGS -E create_symlink libinovasdk.so.${INOVASDK_VERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libinovasdk.so.${INOVASDK_SOVERSION})\n
exec_program(${CMAKE_COMMAND} ARGS -E create_symlink libinovasdk.so.${INOVASDK_SOVERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libinovasdk.so)\n
")

install( FILES ${CMAKE_BINARY_DIR}/libinovasdk.so.${INOVASDK_VERSION} DESTINATION ${LIB_INSTALL_DIR}${LIB_POSTFIX})
install( FILES inovasdk.h DESTINATION include/inovasdk)
install( FILES 99-inovaplx.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
install( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_inovasdk_symlink.cmake)
