SET(glues_lib_SRCS
     glues_stel/source/glues_error.c
     glues_stel/source/glues_error.h
     glues_stel/source/glues.h
     glues_stel/source/libtess/dict.h
     glues_stel/source/libtess/dict.c
     glues_stel/source/libtess/dict-list.h
     glues_stel/source/libtess/geom.c
     glues_stel/source/libtess/geom.h
     glues_stel/source/libtess/memalloc.c
     glues_stel/source/libtess/memalloc.h
     glues_stel/source/libtess/mesh.c
     glues_stel/source/libtess/mesh.h
     glues_stel/source/libtess/normal.c
     glues_stel/source/libtess/normal.h
     glues_stel/source/libtess/priorityq.c
     glues_stel/source/libtess/priorityq.h
     glues_stel/source/libtess/priorityq-heap.h
     glues_stel/source/libtess/priorityq-heap.i
     glues_stel/source/libtess/priorityq-sort.h
     glues_stel/source/libtess/render.c
     glues_stel/source/libtess/render.h
     glues_stel/source/libtess/sweep.c
     glues_stel/source/libtess/sweep.h
     glues_stel/source/libtess/tess.c
     glues_stel/source/libtess/tess.h
     glues_stel/source/libtess/tessmono.c
     glues_stel/source/libtess/tessmono.h
)
add_library(glues_stel STATIC EXCLUDE_FROM_ALL ${glues_lib_SRCS})
target_include_directories(glues_stel PUBLIC glues_stel/source)
#link the GUI module for the GL headers
#our stripped version does not call any GL functions, but requires the GL types to be defined
target_link_libraries(glues_stel Qt${QT_VERSION_MAJOR}::Gui)
#turn off automoc, not needed here
set_target_properties(glues_stel PROPERTIES AUTOMOC 0)
SET_TARGET_PROPERTIES(glues_stel PROPERTIES FOLDER "src/external")

IF(NOT USE_SYSTEM_ZLIB)
     SET(zlib_SRCS
	  zlib/adler32.c
	  zlib/compress.c
	  zlib/crc32.c
	  zlib/crc32.h
	  zlib/deflate.c
	  zlib/deflate.h
	  zlib/gzclose.c
	  zlib/gzguts.h
	  zlib/gzlib.c
	  zlib/gzread.c
	  zlib/gzwrite.c
	  zlib/infback.c
	  zlib/inffast.c
	  zlib/inffast.h
	  zlib/inffixed.h
	  zlib/inflate.c
	  zlib/inflate.h
	  zlib/inftrees.c
	  zlib/inftrees.h
	  zlib/trees.c
	  zlib/trees.h
	  zlib/uncompr.c
	  zlib/zconf.h
	  zlib/zlib.h
	  zlib/zutil.c
	  zlib/zutil.h)

     add_library(zlib_stel STATIC EXCLUDE_FROM_ALL ${zlib_SRCS})
     target_include_directories(zlib_stel PUBLIC zlib)
     #turn off automoc, not needed here
     set_target_properties(zlib_stel PROPERTIES AUTOMOC 0)
     SET_TARGET_PROPERTIES(zlib_stel PROPERTIES FOLDER "src/external")
ENDIF()

IF(USE_BUNDLED_QTCOMPRESS)
    # QtCompress library under LGPL 2.1
    set(qtcompress_SRCS
	qtcompress/qzip.cpp
	qtcompress/qzipreader.h
	qtcompress/qzipwriter.h
	)
    add_library(qtcompress_stel STATIC EXCLUDE_FROM_ALL ${qtcompress_SRCS})
    target_include_directories(qtcompress_stel PUBLIC qtcompress)
    #this can use the system zlib, or our zlib
    target_link_libraries(qtcompress_stel ${ZLIB_LIBRARIES} Qt${QT_VERSION_MAJOR}::Core)
    #turn off automoc, not needed here
    set_target_properties(qtcompress_stel PROPERTIES AUTOMOC 0)
    SET_TARGET_PROPERTIES(qtcompress_stel PROPERTIES FOLDER "src/external")
ENDIF()

################################# INDI ################################
IF(USE_PLUGIN_TELESCOPECONTROL AND ENABLE_INDI)
    SET(PREFER_SYSTEM_INDILIB 1 CACHE BOOL "Use system-provided INDI instead of the bundled version")
    find_library(INDICLIENT_LIB indiclient)
    if(INDICLIENT_LIB AND PREFER_SYSTEM_INDILIB)
        MESSAGE(STATUS "Using system-provided indiclient at ${INDICLIENT_LIB}")
        add_library(indiclient UNKNOWN IMPORTED GLOBAL)
	set_target_properties(indiclient PROPERTIES IMPORTED_LOCATION "${INDICLIENT_LIB}")
    else()
        include(CheckSymbolExists)
        check_symbol_exists(mremap sys/mman.h HAVE_MREMAP)
        check_symbol_exists(timespec_get time.h HAVE_TIMESPEC_GET)
        check_symbol_exists(clock_gettime time.h HAVE_CLOCK_GETTIME)

        # Git repo is large, limit download to archive
        # Included CMakeLists.txt contains unrelated targets and dependencies of
        # them, therefore DOWNLOAD_ONLY and all the code below
        CPMAddPackage(NAME indiclient
		URL https://github.com/indilib/indi/archive/v2.1.1.zip
		URL_HASH SHA256=431ccb9f0631e35bc68eaefd3c60a8afa2211bc4c6b4c332abb8a18a6875b3f1
		VERSION 2.1.1
		DOWNLOAD_ONLY YES)

        # First, fix a missing include
	file(READ ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h INDIDEVAPI_H)
	string(REGEX REPLACE "#include .lilxml.h." "#include \"lilxml.h\"\n#include <stdarg.h>" INDIDEVAPI_H "${INDIDEVAPI_H}")
	file(WRITE ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h.new "${INDIDEVAPI_H}")
	configure_file(${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h.new ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h COPYONLY)
	
	# Fix missing include
	file(READ ${indiclient_SOURCE_DIR}/libs/sockets/select.h INDISOCKETSELECT_H)
	string(REGEX REPLACE "#include .sys/socket.h." "#include <sys/socket.h>\n#include <sys/select.h>" INDISOCKETSELECT_H "${INDISOCKETSELECT_H}")
	file(WRITE ${indiclient_SOURCE_DIR}/libs/sockets/select.h.new "${INDISOCKETSELECT_H}")
	configure_file(${indiclient_SOURCE_DIR}/libs/sockets/select.h.new ${indiclient_SOURCE_DIR}/libs/sockets/select.h COPYONLY)

        # libastro.h doesn't need libnova/utility.h
	file(READ ${indiclient_SOURCE_DIR}/libs/indicore/libastro.h LIBASTRO_H)
        string(REGEX REPLACE "#include <libnova/utility.h>" "" LIBASTRO_H "${LIBASTRO_H}")
	file(WRITE ${indiclient_SOURCE_DIR}/libs/indicore/libastro.h.new "${LIBASTRO_H}")
	configure_file(${indiclient_SOURCE_DIR}/libs/indicore/libastro.h.new ${indiclient_SOURCE_DIR}/libs/indicore/libastro.h COPYONLY)

        # Avoid bundling libnova too
	file(READ ${indiclient_SOURCE_DIR}/libs/indibase/inditelescope.h INDITELESCOPE_H)
	string(REGEX REPLACE "#include <libnova/julian_day.h>" "struct ln_date;" INDITELESCOPE_H "${INDITELESCOPE_H}")
	string(REGEX REPLACE "ln_lnlat_posn lnobserver { 0, 0 };" "" INDITELESCOPE_H "${INDITELESCOPE_H}")
	file(WRITE ${indiclient_SOURCE_DIR}/inditelescope.h.new "${INDITELESCOPE_H}")
	configure_file(${indiclient_SOURCE_DIR}/inditelescope.h.new ${indiclient_SOURCE_DIR}/libs/indibase/inditelescope.h COPYONLY)

        # Fix build in windows
	file(READ ${indiclient_SOURCE_DIR}/libs/indibase/indilogger.h INDILOGGER_H)
	string(REGEX REPLACE "#include <sys/time.h>" "#ifdef Q_OS_WIN\n#include <windows.h>\n#else\n#include <sys/time.h>\n#endif" INDILOGGER_H "${INDILOGGER_H}")
	file(WRITE ${indiclient_SOURCE_DIR}/indilogger.h.new "${INDILOGGER_H}")
	configure_file(${indiclient_SOURCE_DIR}/indilogger.h.new ${indiclient_SOURCE_DIR}/libs/indibase/indilogger.h COPYONLY)

        set(INDI_SOVERSION "2")
        set(CMAKE_INDI_VERSION_MAJOR 2)
	set(CMAKE_INDI_VERSION_MINOR 1)
	set(CMAKE_INDI_VERSION_RELEASE 1)
        set(CMAKE_INDI_VERSION_STRING "${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE}")
        set(INDI_VERSION ${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE})
        set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
	configure_file(${indiclient_SOURCE_DIR}/config.h.cmake ${indiclient_SOURCE_DIR}/config.h )

	configure_file(${indiclient_SOURCE_DIR}/libs/indicore/indiapi.h.in ${indiclient_SOURCE_DIR}/libs/indiapi.h)
        list(APPEND INDILIB_SOURCES
                ${indiclient_SOURCE_DIR}/libs/indicore/lilxml.cpp
                ${indiclient_SOURCE_DIR}/libs/indicore/base64.c
                ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.c
                ${indiclient_SOURCE_DIR}/libs/indicore/indicom.c
                ${indiclient_SOURCE_DIR}/libs/indicore/userio.c
                ${indiclient_SOURCE_DIR}/libs/indicore/indiuserio.c
                ${indiclient_SOURCE_DIR}/libs/indiabstractclient/abstractbaseclient.cpp
                ${indiclient_SOURCE_DIR}/libs/indiclient/baseclient.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/basedevice.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/indibase.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/indistandardproperty.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/parentdevice.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/watchdeviceproperty.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indiproperties.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertybasic.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertyblob.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indiproperty.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertylight.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertynumber.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertyswitch.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertytext.cpp
                ${indiclient_SOURCE_DIR}/libs/indidevice/property/indipropertyview.cpp
                ${indiclient_SOURCE_DIR}/libs/sockets/tcpsocket.cpp
                )
        if(WIN32)
            list(APPEND INDILIB_SOURCES ${indiclient_SOURCE_DIR}/libs/sockets/tcpsocket_win.cpp)
        else()
            list(APPEND INDILIB_SOURCES ${indiclient_SOURCE_DIR}/libs/sockets/tcpsocket_unix.cpp)
        endif()

        add_library(indiclient STATIC ${INDILIB_SOURCES})

        target_compile_definitions(indiclient
          PUBLIC
            $<$<BOOL:${HAVE_TIMESPEC_GET}>:HAVE_TIMESPEC_GET>
            $<$<BOOL:${HAVE_CLOCK_GETTIME}>:HAVE_CLOCK_GETTIME>
        )

        target_include_directories(indiclient
                PRIVATE
                ${CMAKE_CURRENT_BINARY_DIR}/libindi
                ${indiclient_SOURCE_DIR}/libindi
                ${indiclient_SOURCE_DIR}/libs
                PUBLIC
                ${CMAKE_CURRENT_SOURCE_DIR}/fake-indi
                ${CMAKE_CURRENT_BINARY_DIR}
                ${indiclient_SOURCE_DIR}
                ${indiclient_SOURCE_DIR}/libs/sockets
                ${indiclient_SOURCE_DIR}/libs/indiabstractclient
                ${indiclient_SOURCE_DIR}/libs/indibase
                ${indiclient_SOURCE_DIR}/libs/indicore
                ${indiclient_SOURCE_DIR}/libs/indiclient
                ${indiclient_SOURCE_DIR}/libs/indidevice
                ${indiclient_SOURCE_DIR}/libs/indidevice/property
                ${indiclient_SOURCE_DIR}/libs)
        target_link_libraries(indiclient ${ZLIB_LIBRARIES})
    endif()
ENDIF()
