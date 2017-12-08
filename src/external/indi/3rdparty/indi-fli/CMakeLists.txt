cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_fli C CXX)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

set (UDEVRULES_INSTALL_DIR "/lib/udev/rules.d" CACHE STRING "Base directory for udev rules")

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(FLI REQUIRED)
find_package(ZLIB REQUIRED)

set (FLI_CCD_VERSION_MAJOR 1)
set (FLI_CCD_VERSION_MINOR 1)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/indi_fli.xml.cmake ${CMAKE_CURRENT_BINARY_DIR}/indi_fli.xml )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${FLI_INCLUDE_DIR})

include(CMakeCommon)

###### FLI Precision Digital Focuser ######
set(flipdf_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/fli_pdf.cpp
)

add_executable(indi_fli_focus ${flipdf_SRCS})

target_link_libraries(indi_fli_focus ${INDI_LIBRARIES} ${FLI_LIBRARIES} ${M_LIB} ${ZLIB_LIBRARY})

install(TARGETS indi_fli_focus RUNTIME DESTINATION bin )

###### FLI Filter wheel ######
set(flicfw_SRCS
        ${CMAKE_CURRENT_SOURCE_DIR}/fli_cfw.cpp
)

add_executable(indi_fli_wheel ${flicfw_SRCS})

target_link_libraries(indi_fli_wheel ${INDI_LIBRARIES} ${FLI_LIBRARIES} ${M_LIB} ${ZLIB_LIBRARY})

install(TARGETS indi_fli_wheel RUNTIME DESTINATION bin )
############# FLI CCD ###############
set(fliccd_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/fli_ccd.cpp
)

add_executable(indi_fli_ccd ${fliccd_SRCS})

target_link_libraries(indi_fli_ccd ${INDI_LIBRARIES} ${FLI_LIBRARIES} ${CFITSIO_LIBRARIES} ${M_LIB} ${ZLIB_LIBRARY})

install(TARGETS indi_fli_ccd RUNTIME DESTINATION bin)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/indi_fli.xml DESTINATION ${INDI_DATA_DIR})

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/99-fli.rules DESTINATION ${UDEVRULES_INSTALL_DIR})
endif()

