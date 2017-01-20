#-------------------------------------------------
#
# Project created by QtCreator 2010-11-28T20:43:35
#
#-------------------------------------------------

QT       += core gui opengl network

DEFINES += PACKAGE_VERSION=\\\"0.10.6\\\"
DEFINES += DEFAULT_GRAPHICS_SYSTEM=\\\"raster\\\"
DEFINES += INSTALL_DATADIR=\\\"/usr/share/stellarium\\\"
DEFINES += INSTALL_LOCALEDIR=\\\"/usr/share/locale\\\"
DEFINES += NDEBUG=1

TARGET = toastForShape
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

INCLUDEPATH += ../../src/core/ ../../src/core/external/glues_stel/source ../../src/core/external

SOURCES += main.cpp \
    ../../src/core/StelSphereGeometry.cpp \
    ../../src/core/StelToastGrid.cpp \
    ../../src/core/StelJsonParser.cpp \
    ../../src/core/OctahedronPolygon.cpp \
    ../../src/core/StelUtils.cpp \
    ../../src/core/StelProjector.cpp \
    ../../src/core/external/glues_stel/source/glues_error.c \
    ../../src/core/external/glues_stel/source/libtess/tessmono.c \
    ../../src/core/external/glues_stel/source/libtess/tess.c \
    ../../src/core/external/glues_stel/source/libtess/sweep.c \
    ../../src/core/external/glues_stel/source/libtess/render.c \
    ../../src/core/external/glues_stel/source/libtess/priorityq.c \
    ../../src/core/external/glues_stel/source/libtess/normal.c \
    ../../src/core/external/glues_stel/source/libtess/mesh.c \
    ../../src/core/external/glues_stel/source/libtess/memalloc.c \
    ../../src/core/external/glues_stel/source/libtess/geom.c \
    ../../src/core/external/glues_stel/source/libtess/dict.c \
    ../../src/core/StelVertexArray.cpp \
    ../../src/core/StelTranslator.cpp \
    ../../src/core/StelFileMgr.cpp

HEADERS += \
    ../../src/core/VecMath.hpp \
    ../../src/core/StelSphereGeometry.hpp \
    ../../src/core/StelToastGrid.hpp \
    ../../src/core/StelJsonParser.hpp \
    ../../src/core/OctahedronPolygon.hpp \
    ../../src/core/StelUtils.hpp \
    ../../src/core/external/glues_stel/source/glues.h \
    ../../src/core/external/glues_stel/source/glues_error.h \
    ../../src/core/external/glues_stel/source/libtess/tessmono.h \
    ../../src/core/external/glues_stel/source/libtess/tess.h \
    ../../src/core/external/glues_stel/source/libtess/sweep.h \
    ../../src/core/external/glues_stel/source/libtess/render.h \
    ../../src/core/external/glues_stel/source/libtess/priorityq.h \
    ../../src/core/external/glues_stel/source/libtess/priorityq-sort.h \
    ../../src/core/external/glues_stel/source/libtess/priorityq-heap.h \
    ../../src/core/external/glues_stel/source/libtess/normal.h \
    ../../src/core/external/glues_stel/source/libtess/mesh.h \
    ../../src/core/external/glues_stel/source/libtess/memalloc.h \
    ../../src/core/external/glues_stel/source/libtess/geom.h \
    ../../src/core/external/glues_stel/source/libtess/dict.h \
    ../../src/core/external/glues_stel/source/libtess/dict-list.h \
    ../../src/core/StelVertexArray.hpp \
    ../../src/core/StelTranslator.hpp

OTHER_FILES += \
    ../../src/core/external/glues_stel/source/libtess/README \
    ../../src/core/external/glues_stel/source/libtess/priorityq-heap.i \
    ../../src/core/external/glues_stel/source/libtess/alg-outline
