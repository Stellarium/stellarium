# This is the cmake config file for the CompassMarks plugin
SET(COMPASSMARKS_VERSION "0.2.2")

ADD_SUBDIRECTORY( src )

IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/CompassMarks")
