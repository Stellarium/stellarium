PROJECT(indi-eqmod C CXX)
cmake_minimum_required(VERSION 2.4.7)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")

find_package(INDI REQUIRED)
find_package(Nova REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GSL REQUIRED)

set(EQMOD_VERSION_MAJOR 0)
set(EQMOD_VERSION_MINOR 3)

if (CYGWIN)
add_definitions(-U__STRICT_ANSI__)
endif(CYGWIN)

option(WITH_ALIGN "Enable Alignment Subsystem" ON)
option(WITH_ALIGN_GEEHALEL "Enable EQMod Alignment" ON)
option(WITH_SCOPE_LIMITS "Enable Scope limits" ON)

set(INDI_DATA_DIR "${CMAKE_INSTALL_PREFIX}/share/indi")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_eqmod.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_eqmod.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR})
include_directories( ${NOVA_INCLUDE_DIR}/..)
include_directories( ${GSL_INCLUDE_DIR} )

include(CMakeCommon)

IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.9.9)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format-overflow")
ENDIF ()

########### EQMod ###############
set(eqmod_CXX_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/eqmod.cpp ${CMAKE_CURRENT_SOURCE_DIR}/eqmoderror.cpp ${CMAKE_CURRENT_SOURCE_DIR}/skywatcher.cpp)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set(eqmod_CXX_SRCS ${eqmod_CXX_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/mach_gettime.cpp)
endif()

set(eqmod_CXX_SRCS ${eqmod_CXX_SRCS}
   ${CMAKE_CURRENT_SOURCE_DIR}/simulator/simulator.cpp    ${CMAKE_CURRENT_SOURCE_DIR}/simulator/skywatcher-simulator.cpp)
if(WITH_ALIGN_GEEHALEL)
  set(eqmod_CXX_SRCS ${eqmod_CXX_SRCS}
   ${CMAKE_CURRENT_SOURCE_DIR}/align/align.cpp ${CMAKE_CURRENT_SOURCE_DIR}/align/pointset.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/align/triangulate.cpp ${CMAKE_CURRENT_SOURCE_DIR}/align/triangulate_chull.cpp)
  set(eqmod_C_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/align/htm.c ${CMAKE_CURRENT_SOURCE_DIR}/align/chull/chull.c)
endif(WITH_ALIGN_GEEHALEL)
if(WITH_SCOPE_LIMITS)
  set(eqmod_CXX_SRCS ${eqmod_CXX_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/scope-limits/scope-limits.cpp)
endif(WITH_SCOPE_LIMITS)

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(eqmod_c eqmod_C_SRCS 10 c)
    ENABLE_UNITY_BUILD(eqmod_cxx eqmod_CXX_SRCS 10 cpp)
ENDIF ()

add_executable(indi_eqmod_telescope ${eqmod_C_SRCS} ${eqmod_CXX_SRCS})

if(WITH_ALIGN)
  target_link_libraries(indi_eqmod_telescope ${INDI_LIBRARIES} ${NOVA_LIBRARIES} ${INDI_ALIGN_LIBRARIES} ${GSL_LIBRARIES} ${ZLIB_LIBRARY})
else(WITH_ALIGN)
  target_link_libraries(indi_eqmod_telescope ${INDI_LIBRARIES} ${NOVA_LIBRARIES})
endif(WITH_ALIGN)

if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
target_link_libraries(indi_eqmod_telescope rt)
endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

install(TARGETS indi_eqmod_telescope RUNTIME DESTINATION bin )

install( FILES  ${CMAKE_CURRENT_BINARY_DIR}/indi_eqmod.xml indi_eqmod_sk.xml DESTINATION ${INDI_DATA_DIR})

install( FILES  simulator/indi_eqmod_simulator_sk.xml DESTINATION ${INDI_DATA_DIR})
if(WITH_ALIGN_GEEHALEL)
  install( FILES  align/indi_align_sk.xml DESTINATION ${INDI_DATA_DIR})
endif(WITH_ALIGN_GEEHALEL)
if(WITH_SCOPE_LIMITS)
  install( FILES  scope-limits/indi_eqmod_scope_limits_sk.xml DESTINATION ${INDI_DATA_DIR})
endif(WITH_SCOPE_LIMITS)
