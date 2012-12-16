# This is the cmake config file for the AngleMeasure plugin
SET(ANGLEMEASURE_VERSION "0.2.3")

ADD_SUBDIRECTORY( src )

IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/AngleMeasure")
