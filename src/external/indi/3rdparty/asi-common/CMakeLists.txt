cmake_minimum_required(VERSION 2.8)
PROJECT(asi_common NONE)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/99-asi.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
endif()
