cmake_minimum_required(VERSION 2.8)
PROJECT(libsbig C CXX)

set(SBIG_VERSION "2.1.1")
set(SBIG_SOVERSION "2")

set(FIRMWARE_INSTALL_DIR "/lib/firmware")
set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(LIB_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/lib")

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

IF(UNIX AND NOT WIN32 AND NOT APPLE)
 IF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libsbigudrv_arm_v6.bin ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv7l")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libsbigudrv_arm_v7.bin ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
 ELSEIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "aarch64")
    exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libsbigudrv_arm_v8.bin ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
 ELSEIF(CMAKE_SIZEOF_VOID_P MATCHES "8")
  exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libsbigudrv64.bin ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
 ELSE()
  exec_program(cp ARGS ${CMAKE_CURRENT_SOURCE_DIR}/libsbigudrv32.bin ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION})
 ENDIF (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "armv6l")
ENDIF(UNIX AND NOT WIN32 AND NOT APPLE)

# Make sure symbolic links are installed
file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_sbig_symlink.cmake "
exec_program(${CMAKE_COMMAND} ARGS -E create_symlink libsbigudrv.so.${SBIG_VERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so.${SBIG_SOVERSION})\n
exec_program(${CMAKE_COMMAND} ARGS -E create_symlink libsbigudrv.so.${SBIG_SOVERSION} ${LIB_INSTALL_DIR}${LIB_POSTFIX}/libsbigudrv.so)\n
")

install( FILES ${CMAKE_BINARY_DIR}/libsbigudrv.so.${SBIG_VERSION} DESTINATION ${LIB_INSTALL_DIR}${LIB_POSTFIX})
install( FILES sbigudrv.h DESTINATION include/libsbig)
install( FILES sbigucam.hex sbiglcam.hex sbigfcam.hex sbigpcam.hex stfga.bin DESTINATION ${FIRMWARE_INSTALL_DIR})
install( FILES 51-sbig-debian.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
install( SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_sbig_symlink.cmake)
