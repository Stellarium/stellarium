# This is the cmake config file for the ArchaeoLines plugin
# Versions:
#0.1.3 adding azimuth lines
#0.1.4 adding geographic locations
#0.1.5 using the StelProperty system
#0.1.6 adding arbitrary declinations
SET(ARCHAEOLINES_VERSION "0.1.6")
ADD_DEFINITIONS(-DARCHAEOLINES_VERSION="${ARCHAEOLINES_VERSION}")

ADD_SUBDIRECTORY( src )

IF(APPLE)
     SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
     SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/ArchaeoLines")
