PROJECT(libdspau C CXX)
cmake_minimum_required(VERSION 2.4.7)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../cmake_modules/")
include(GNUInstallDirs)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

FIND_PACKAGE(FFTW3 REQUIRED)

SET(PACKAGE_VERSION "1.0.0")

ADD_DEFINITIONS(-Wall -O2)

include_directories( ${CMAKE_CURRENT_SOURCE_DIR})

set(dspau_LIB_SRCS
   libdspau-fft.c
   libdspau-filters.c
   libdspau-correlations.c
   libdspau-stats.c
   libdspau-buffer.c
   libdspau-convert.c
)

#build a shared library
ADD_LIBRARY(dspau SHARED ${dspau_LIB_SRCS})

set_target_properties(dspau PROPERTIES VERSION 1.0.0 SOVERSION 1)

#need to link to some other libraries ? just add them here
TARGET_LINK_LIBRARIES(dspau ${FFTW3_LIBRARIES} ${M_LIB})

#add an install target here
INSTALL(FILES libdspau.h DESTINATION include)

INSTALL(TARGETS dspau LIBRARY DESTINATION lib)
