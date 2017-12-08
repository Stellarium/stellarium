cmake_minimum_required(VERSION 2.4.7)
PROJECT(libfli CXX C)

include(GNUInstallDirs)
ADD_DEFINITIONS(-Wall -O2)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-error")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-error")

set(fli_LIB_SRCS
   libfli.c libfli-camera.c libfli-camera-parport.c libfli-camera-usb.c
   libfli-filter-focuser.c libfli-mem.c libfli-serial.c libfli-sys.c
   libfli-usb.c libfli-debug.c libfli-parport.c libfli-usb-sys.c
   flifilter.c libfli-raw.c
)

#build a shared library
ADD_LIBRARY(fli SHARED ${fli_LIB_SRCS})

set_target_properties(fli PROPERTIES VERSION 1.8 SOVERSION 1)

#need to link to some other libraries ? just add them here
TARGET_LINK_LIBRARIES(fli -lm)

#add an install target here
INSTALL(FILES libfli.h DESTINATION include)

INSTALL(TARGETS fli LIBRARY DESTINATION lib)
