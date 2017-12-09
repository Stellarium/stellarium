cmake_minimum_required(VERSION 2.4.7)
PROJECT(indi_gige CXX C)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules/")
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

find_package(CFITSIO REQUIRED)
find_package(INDI REQUIRED)
find_package(ZLIB REQUIRED)
find_package(GLIB2 REQUIRED)
find_package(ARAVIS REQUIRED)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )

include_directories( ${CMAKE_CURRENT_BINARY_DIR})
include_directories( ${CMAKE_CURRENT_SOURCE_DIR})
include_directories( ${INDI_INCLUDE_DIR})
include_directories( ${CFITSIO_INCLUDE_DIR})
include_directories( ${GLIB2_INCLUDE_DIRS})
include_directories( ${Arv_INCLUDE_DIRS})

include(CMakeCommon)

############# GENERIC CCD ###############
if (CFITSIO_FOUND)

    set(GIGE_SRCS
	${CMAKE_CURRENT_SOURCE_DIR}/src/indi_gige.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ArvGeneric.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ArvFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/BlackFly.cpp
)

IF (UNITY_BUILD)
    ENABLE_UNITY_BUILD(indigige GIGE_SRCS 10 cpp)
ENDIF ()

add_executable(indi_gige_ccd ${GIGE_SRCS})

#target_link_libraries(indi_gige_ccd ${INDI_LIBRARIES} ${CFITSIO_LIBRARIES} m ${ZLIB_LIBRARY} ${GLIB2_LIBRARY} ${ARV_LIBRARY})
target_link_libraries(indi_gige_ccd ${INDI_LIBRARIES} ${GLIB2_LIBRARIES} ${Arv_LIBRARIES} ${CFITSIO_LIBRARIES} m gobject-2.0 ${ZLIB_LIBRARY})

install(TARGETS indi_gige_ccd RUNTIME DESTINATION bin)

endif (CFITSIO_FOUND)

install(FILES indi_gige_ccd.xml DESTINATION ${INDI_DATA_DIR})
