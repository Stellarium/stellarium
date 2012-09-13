#-------------------------------------------------
#
# Project created by QtCreator 2012-09-10T19:57:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = locations-editor
TEMPLATE = app


SOURCES += main.cpp\
        LocationListEditor.cpp \
    Location.cpp \
    LocationListModel.cpp \
    ../../src/core/external/kfilter/kzip.cpp \
    ../../src/core/external/kfilter/klimitediodevice.cpp \
    ../../src/core/external/kfilter/kgzipfilter.cpp \
    ../../src/core/external/kfilter/kfilterdev.cpp \
    ../../src/core/external/kfilter/kfilterbase.cpp \
    ../../src/core/external/kfilter/karchive.cpp

HEADERS  += LocationListEditor.hpp \
    Location.hpp \
    LocationListModel.hpp \
    ../../src/core/external/kfilter/kzip.h \
    ../../src/core/external/kfilter/klimitediodevice.h \
    ../../src/core/external/kfilter/kgzipfilter.h \
    ../../src/core/external/kfilter/kfilterdev.h \
    ../../src/core/external/kfilter/kfilterbase.h \
    ../../src/core/external/kfilter/karchive.h

FORMS    += LocationListEditor.ui

RESOURCES += \
    ../../data/locationsEditor.qrc
