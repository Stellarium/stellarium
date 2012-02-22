# cmake config file for the QML-based MobileGUI plugin
SET(MOBILEGUI_VERSION "0.0.1")

ADD_SUBDIRECTORY( src )

INSTALL(FILES DESTINATION "modules/AngleMeasure")
