cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_gphoto C CXX)

set(INDI_GPHOTO_VERSION_MAJOR 1)
set(INDI_GPHOTO_VERSION_MINOR 6)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set(UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

SET(BIN_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/bin")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall" )
SET(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG_FITS" )
SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall" )
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g -DDEBUG_FITS" )

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GPHOTO2 REQUIRED)
find_package(Threads REQUIRED)
find_package(JPEG REQUIRED)
find_package(LibRaw REQUIRED)
find_package(USB1 REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_gphoto.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_gphoto.xml )

include(CMakeCommon)

#find_program(DCRAW_EXECUTABLE NAMES dcraw
#    PATHS
#    /usr/bin
#    /usr/local/bin)

#if (NOT DCRAW_EXECUTABLE)
#    message(FATAL_ERROR "dcraw not found. Please install dcraw and try again.")
#endif (NOT DCRAW_EXECUTABLE)

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${GPHOTO2_INCLUDE_DIR})
include_directories( ${LibRaw_INCLUDE_DIR})
include_directories( ${USB1_INCLUDE_DIRS})

#if (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")
#add_definitions(-DUSE_THUMB_SIZE)
#endif (CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm*")

########### Gphoto ###########
set(indigphoto_SRCS
   ${CMAKE_CURRENT_SOURCE_DIR}/gphoto_ccd.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/gphoto_driver.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/gphoto_readimage.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/dsusbdriver.cpp
   )

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indigphoto indi_gphotoSRCS 10 cpp)
ENDIF ()

SET_SOURCE_FILES_PROPERTIES(gphoto_readimage.cpp PROPERTIES COMPILE_FLAGS "-Wno-deprecated-declarations")

add_executable(indi_gphoto_ccd ${indigphoto_SRCS})

target_link_libraries(indi_gphoto_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} ${GPHOTO2_LIBRARY} ${GPHOTO2_PORT_LIBRARY} ${CMAKE_THREAD_LIBS_INIT} ${JPEG_LIBRARIES} ${LibRaw_LIBRARIES} ${ZLIB_LIBRARIES})
install(TARGETS indi_gphoto_ccd RUNTIME DESTINATION bin )

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/make_gphoto_symlink.cmake
"exec_program(\"${CMAKE_COMMAND}\" ARGS -E create_symlink indi_gphoto_ccd \$ENV{DESTDIR}${BIN_INSTALL_DIR}/indi_canon_ccd)\n
exec_program(\"${CMAKE_COMMAND}\" ARGS -E create_symlink indi_gphoto_ccd \$ENV{DESTDIR}${BIN_INSTALL_DIR}/indi_nikon_ccd)\n")
set_target_properties(indi_gphoto_ccd PROPERTIES POST_INSTALL_SCRIPT ${CMAKE_CURRENT_BINARY_DIR}/make_gphoto_symlink.cmake)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_gphoto.xml DESTINATION ${INDI_DATA_DIR})

# Disable automount for DSLR cameras
IF (UNIX AND NOT APPLE)
    install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/85-disable-dslr-automout.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
ENDIF ()
