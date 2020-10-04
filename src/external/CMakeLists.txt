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
target_link_libraries(glues_stel Qt5::Gui)
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

set(qtcompress_SRCS
    qtcompress/qzip.cpp
    qtcompress/qzipreader.h
    qtcompress/qzipwriter.h
    )
add_library(qtcompress_stel STATIC EXCLUDE_FROM_ALL ${qtcompress_SRCS})
target_include_directories(qtcompress_stel PUBLIC qtcompress)
#this can use the system zlib, or our zlib
target_link_libraries(qtcompress_stel ${ZLIB_LIBRARIES} Qt5::Core)
#turn off automoc, not needed here
set_target_properties(qtcompress_stel PROPERTIES AUTOMOC 0)
SET_TARGET_PROPERTIES(qtcompress_stel PROPERTIES FOLDER "src/external")

# QCustomPlot should not be unbundled, because upstream uses GPL 3, and
# Stellarium uses GPL 2. Author of QCustomPlot has agreed for GPL 2+ for this.
set(qcustomplot_SRCS
    qcustomplot/qcustomplot.cpp
    qcustomplot/qcustomplot.h
    )
add_library(qcustomplot_stel STATIC EXCLUDE_FROM_ALL ${qcustomplot_SRCS})
target_link_libraries(qcustomplot_stel Qt5::PrintSupport Qt5::Widgets)
SET_TARGET_PROPERTIES(qcustomplot_stel PROPERTIES FOLDER "src/external")

SET(qxlsx_SRCS
    qxlsx/xlsxabstractooxmlfile.cpp
    qxlsx/xlsxabstractsheet.cpp
    qxlsx/xlsxcell.cpp
    qxlsx/xlsxcellformula.cpp
    qxlsx/xlsxcellrange.cpp
    qxlsx/xlsxcellreference.cpp
    qxlsx/xlsxchart.cpp
    qxlsx/xlsxchartsheet.cpp
    qxlsx/xlsxcolor.cpp
    qxlsx/xlsxconditionalformatting.cpp
    qxlsx/xlsxcontenttypes.cpp
    qxlsx/xlsxdatavalidation.cpp
    qxlsx/xlsxdocpropsapp.cpp
    qxlsx/xlsxdocpropscore.cpp
    qxlsx/xlsxdocument.cpp
    qxlsx/xlsxdrawing.cpp
    qxlsx/xlsxdrawinganchor.cpp
    qxlsx/xlsxformat.cpp
    qxlsx/xlsxmediafile.cpp
    qxlsx/xlsxnumformatparser.cpp
    qxlsx/xlsxrelationships.cpp
    qxlsx/xlsxrichstring.cpp
    qxlsx/xlsxsharedstrings.cpp
    qxlsx/xlsxsimpleooxmlfile.cpp
    qxlsx/xlsxstyles.cpp
    qxlsx/xlsxtheme.cpp
    qxlsx/xlsxutility.cpp
    qxlsx/xlsxworkbook.cpp
    qxlsx/xlsxworksheet.cpp
    qxlsx/xlsxzipreader.cpp
    qxlsx/xlsxzipwriter.cpp
    qxlsx/xlsxcelllocation.cpp
    qxlsx/xlsxabstractooxmlfile.h
    qxlsx/xlsxabstractooxmlfile_p.h
    qxlsx/xlsxabstractsheet.h
    qxlsx/xlsxabstractsheet_p.h
    qxlsx/xlsxcell.h
    qxlsx/xlsxcellformula.h
    qxlsx/xlsxcellformula_p.h
    qxlsx/xlsxcellrange.h
    qxlsx/xlsxcellreference.h
    qxlsx/xlsxcell_p.h
    qxlsx/xlsxchart.h
    qxlsx/xlsxchartsheet.h
    qxlsx/xlsxchartsheet_p.h
    qxlsx/xlsxchart_p.h
    qxlsx/xlsxcolor_p.h
    qxlsx/xlsxconditionalformatting.h
    qxlsx/xlsxconditionalformatting_p.h
    qxlsx/xlsxcontenttypes_p.h
    qxlsx/xlsxdatavalidation.h
    qxlsx/xlsxdatavalidation_p.h
    qxlsx/xlsxdocpropsapp_p.h
    qxlsx/xlsxdocpropscore_p.h
    qxlsx/xlsxdocument.h
    qxlsx/xlsxdocument_p.h
    qxlsx/xlsxdrawinganchor_p.h
    qxlsx/xlsxdrawing_p.h
    qxlsx/xlsxformat.h
    qxlsx/xlsxformat_p.h
    qxlsx/xlsxglobal.h
    qxlsx/xlsxmediafile_p.h
    qxlsx/xlsxnumformatparser_p.h
    qxlsx/xlsxrelationships_p.h
    qxlsx/xlsxrichstring.h
    qxlsx/xlsxrichstring_p.h
    qxlsx/xlsxsharedstrings_p.h
    qxlsx/xlsxsimpleooxmlfile_p.h
    qxlsx/xlsxstyles_p.h
    qxlsx/xlsxtheme_p.h
    qxlsx/xlsxutility_p.h
    qxlsx/xlsxworkbook.h
    qxlsx/xlsxworkbook_p.h
    qxlsx/xlsxworksheet.h
    qxlsx/xlsxworksheet_p.h
    qxlsx/xlsxzipreader_p.h
    qxlsx/xlsxzipwriter_p.h
    qxlsx/xlsxcelllocation.h
    )
add_library(qxlsx_stel STATIC EXCLUDE_FROM_ALL ${qxlsx_SRCS})
target_link_libraries(qxlsx_stel Qt5::Core Qt5::Gui qtcompress_stel)
set_target_properties(qxlsx_stel PROPERTIES FOLDER "src/external")

################################# INDI ################################
IF(USE_PLUGIN_TELESCOPECONTROL)
    set(INDI_SOVERSION "1")
    set(CMAKE_INDI_VERSION_MAJOR 1)
    set(CMAKE_INDI_VERSION_MINOR 6)
    set(CMAKE_INDI_VERSION_RELEASE 0)
    set(CMAKE_INDI_VERSION_STRING "${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE}")
    set(INDI_VERSION ${CMAKE_INDI_VERSION_MAJOR}.${CMAKE_INDI_VERSION_MINOR}.${CMAKE_INDI_VERSION_RELEASE})
    set(DATA_INSTALL_DIR "${CMAKE_INSTALL_PREFIX}/share/indi/")
    configure_file(libindi/config.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config.h )
    configure_file(libindi/indiversion.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/indiversion.h )
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    include_directories(libindi)
    include_directories(libindi/libs)
    include_directories(libindi/libs/indibase)
    add_library(indiclient STATIC
        libindi/libs/lilxml.c
        libindi/base64.c
        libindi/libs/indibase/basedevice.h
        libindi/libs/indibase/basedevice.cpp
        libindi/libs/indibase/baseclient.h
        libindi/libs/indibase/baseclient.cpp
        libindi/libs/indibase/indiproperty.cpp
        libindi/libs/indibase/indistandardproperty.cpp
        libindi/libs/indicom.c
        )
    target_link_libraries(indiclient ${ZLIB_LIBRARIES}) # Do not use 2 version of zlib! 
    target_include_directories(indiclient INTERFACE libindi)
    target_include_directories(indiclient INTERFACE libindi/libs)

    SET_TARGET_PROPERTIES(indiclient PROPERTIES FOLDER "src/external")
ENDIF()

