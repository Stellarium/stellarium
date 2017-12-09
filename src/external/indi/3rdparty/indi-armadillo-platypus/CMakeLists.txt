cmake_minimum_required(VERSION 3.2)
PROJECT(indi_armplatfocuser CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

SET(CMAKE_CXX_STANDARD 11)
SET(RULES_INSTALL_DIR "/lib/udev/rules.d/")

find_package(INDI REQUIRED)

set (ARMADILLO_VERSION_MAJOR 1)
set (ARMADILLO_VERSION_MINOR 0)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_armplatfocuser.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_armplatfocuser.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})

include(CMakeCommon)

########### Lunatico Seletek Armadillo & Platypus ###########
set(indiarmadillofocuser_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/armadillo_focuser.cpp
   )

set(indiplatypusfocuser_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/platypus_focuser.cpp
   )

add_executable(indi_armadillo_focus ${indiarmadillofocuser_SRCS})
add_executable(indi_platypus_focus ${indiplatypusfocuser_SRCS})

target_link_libraries(indi_armadillo_focus ${INDI_LIBRARIES} )
target_link_libraries(indi_platypus_focus ${INDI_LIBRARIES} )

install(TARGETS indi_armadillo_focus RUNTIME DESTINATION bin )
install(TARGETS indi_platypus_focus RUNTIME DESTINATION bin )

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_armplatfocuser.xml DESTINATION ${INDI_DATA_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(FILES 99-armadilloplatypus.rules DESTINATION ${RULES_INSTALL_DIR})
endif()
