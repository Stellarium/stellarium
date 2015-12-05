# This is the cmake config file for the SimpleDrawLine
SET(SIMPLEDRAWLINE_VERSION "0.1.0")
ADD_DEFINITIONS(-DBUILD_DYNAMIC_PLUGIN=0)

ADD_SUBDIRECTORY( src )

IF(APPLE)
     SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
     SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/SimpleDrawLine")

