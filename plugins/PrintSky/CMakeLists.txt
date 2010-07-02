# This is the cmake config file for the PrintSky plugin
SET(PRINTSKY_VERSION "0.1.0")

ADD_SUBDIRECTORY( src )

IF(APPLE)
	 SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
	 SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/PrintSky")
