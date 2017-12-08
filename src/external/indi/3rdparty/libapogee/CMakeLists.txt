cmake_minimum_required(VERSION 2.8)
PROJECT(libapogee C CXX)

set(CMAKE_CXX_STANDARD 11)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")
set(CONF_DIR "/etc/Apogee/camera" CACHE STRING "Configuration directory")

set(APOGEE_VERSION "3.0.3234")
set(APOGEE_SOVERSION "3")

find_package(USB1 REQUIRED)
find_package(CURL REQUIRED)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
find_package(Boost REQUIRED COMPONENTS regex)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR}/linux)
include_directories( ${Boost_INCLUDE_DIRS})
include_directories( ${CURL_INCLUDE_DIR})

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

# Less error prone with libapogee upstream updates since they do not use CMake
file(GLOB libapogee_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/linux/*.cpp)

# Windows file
list(REMOVE_ITEM libapogee_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/COMHelper.cpp")
# Missing headers
list(REMOVE_ITEM libapogee_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/AspenFx2.cpp")

add_library(apogee SHARED ${libapogee_SRCS})

set_target_properties(apogee PROPERTIES VERSION ${APOGEE_VERSION} SOVERSION ${APOGEE_SOVERSION})

target_link_libraries(apogee ${USB1_LIBRARIES} ${CURL_LIBRARY} ${Boost_LIBRARIES})

install(TARGETS apogee LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

file(GLOB libapogee_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
install( FILES ${libapogee_HEADERS} DESTINATION include/libapogee COMPONENT Devel)

install(
   CODE "
   file(GLOB APOGEE_CONF ${CMAKE_CURRENT_SOURCE_DIR}/conf/*) \n
   file(INSTALL DESTINATION ${CONF_DIR} TYPE FILE FILES \${APOGEE_CONF})"
 )

install(FILES 99-apogee.rules DESTINATION ${UDEVRULES_INSTALL_DIR})

