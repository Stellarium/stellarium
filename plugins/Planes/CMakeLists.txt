# This is the cmake config file for the Planes plugin
SET(PLANES_VERSION "0.1.0")

ADD_DEFINITIONS(-DPLANES_PLUGIN_VERSION="${PLANES_VERSION}")

ADD_SUBDIRECTORY( src )


IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/Planes")
