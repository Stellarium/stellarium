# This is the cmake config file for the CompassMarks plugin
SET(COMPASSMARKS_VERSION "0.1.0")

ADD_SUBDIRECTORY( src )

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
INSTALL(FILES DESTINATION "modules/CompassMarks")
