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
    LocationListModel.cpp

HEADERS  += LocationListEditor.hpp \
    Location.hpp \
    LocationListModel.hpp

FORMS    += LocationListEditor.ui
