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

################################# INDI ################################
IF(USE_PLUGIN_TELESCOPECONTROL)
    SET(PREFER_SYSTEM_INDILIB 1 CACHE BOOL
        "Use system-provided INDI instead of the bundled version")
    find_library(INDICLIENT_LIB indiclient)
    if(INDICLIENT_LIB AND PREFER_SYSTEM_INDILIB)
        MESSAGE(STATUS "Using system-provided indiclient at ${INDICLIENT_LIB}")
        add_library(indiclient UNKNOWN IMPORTED GLOBAL)
        set_target_properties(indiclient PROPERTIES
            IMPORTED_LOCATION "${INDICLIENT_LIB}")
    else()
        # Git repo is large, limit download to archive
        # Included CMakeLists.txt contains unrelated targets and dependencies of
        # them, therefore DOWNLOAD_ONLY and all the code below
        CPMAddPackage(NAME indiclient
            URL https://github.com/indilib/indi/archive/v1.9.9.zip
            URL_HASH SHA256=17cc4c0cb2559f373aabe1f6c0d76f00508c0608da34a2c5bdcffbc41e3dce01
            VERSION 1.9.9
            DOWNLOAD_ONLY YES)

        # Installed version has /usr/include/libindi, copy these headers to
        # such directory for a consistent #include
        file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/libindi)

        # First, fix a missing include
        file(READ
            ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h
            INDIDEVAPI_H)
        string(REGEX REPLACE "#include .lilxml.h."
            "#include \"lilxml.h\"\n#include <stdarg.h>"
            INDIDEVAPI_H "${INDIDEVAPI_H}")
        file(WRITE
            ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h.new
	        "${INDIDEVAPI_H}")
        configure_file(${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h.new
            ${indiclient_SOURCE_DIR}/libs/indicore/indidevapi.h COPYONLY)

        # Now copy the headers
        file(GLOB INDI_HEADERS "${indiclient_SOURCE_DIR}/libs/*/*.h")
        file(COPY ${INDI_HEADERS}
            DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/libindi)

        # Avoid bundling libnova too
        file(READ
            ${indiclient_SOURCE_DIR}/libs/indibase/inditelescope.h
            INDITELESCOPE_H)
        string(REGEX REPLACE
            "#include <libnova/julian_day.h>" "struct ln_date;"
            INDITELESCOPE_H "${INDITELESCOPE_H}")
        string(REGEX REPLACE
            "ln_lnlat_posn lnobserver { 0, 0 };" ""
            INDITELESCOPE_H "${INDITELESCOPE_H}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libindi/inditelescope.h.new
            "${INDITELESCOPE_H}")
        configure_file(${CMAKE_CURRENT_BINARY_DIR}/libindi/inditelescope.h.new
            ${CMAKE_CURRENT_BINARY_DIR}/libindi/inditelescope.h COPYONLY)

        # Fix build in windows
        file(READ
            ${indiclient_SOURCE_DIR}/libs/indibase/indilogger.h
            INDILOGGER_H)
        string(REGEX REPLACE "#include <sys/time.h>"
            "#ifdef Q_OS_WIN\n#include <windows.h>\n#else\n#include <sys/time.h>\n#endif"
            INDILOGGER_H "${INDILOGGER_H}")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libindi/indilogger.h.new
            "${INDILOGGER_H}")
        configure_file(${CMAKE_CURRENT_BINARY_DIR}/libindi/indilogger.h.new
            ${CMAKE_CURRENT_BINARY_DIR}/libindi/indilogger.h COPYONLY)

        # https://devblogs.microsoft.com/cppblog/msvc-preprocessor-progress-towards-conformance/
        # This patch is upstreamed at
        # https://github.com/indilib/indi/pull/1728 so after version 1.9.8
        # shouldn't be needed
        file(GLOB INDILOCALE_H ${indiclient_SOURCE_DIR}/libs/locale_compat.h)
        if (INDILOCALE_H)
            file(READ ${indiclient_SOURCE_DIR}/libs/locale_compat.h INDILOCALE_H)
            string(REGEX REPLACE "L#s" "L\"\"#s" INDILOCALE_H "${INDILOCALE_H}")
            file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/libindi/locale_compat.h.new
                "${INDILOCALE_H}")
            configure_file(${CMAKE_CURRENT_BINARY_DIR}/libindi/locale_compat.h.new
                ${CMAKE_CURRENT_BINARY_DIR}/libindi/locale_compat.h COPYONLY)
            file(RENAME ${indiclient_SOURCE_DIR}/libs/locale_compat.h
                ${indiclient_SOURCE_DIR}/libs/locale_compat.h.bak)
        endif()

        set(INDI_SOVERSION "1")
        set(CMAKE_INDI_VERSION_MAJOR 1)
        set(CMAKE_INDI_VERSION_MINOR 9)
        set(CMAKE_INDI_VERSION_RELEASE 9)
        set(CMAKE_INDI_VERSION_STRING "${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE}")
        set(INDI_VERSION ${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE})
        set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
        configure_file(${indiclient_SOURCE_DIR}/config.h.cmake
            ${CMAKE_CURRENT_BINARY_DIR}/libindi/config.h )
        configure_file(${indiclient_SOURCE_DIR}/libs/indicore/indiapi.h.in
            ${indiclient_SOURCE_DIR}/libs/indiapi.h)
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

        target_include_directories(indiclient
            PRIVATE
            ${CMAKE_CURRENT_BINARY_DIR}/libindi
            ${indiclient_SOURCE_DIR}/libindi
            ${indiclient_SOURCE_DIR}/libs
            PUBLIC
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
