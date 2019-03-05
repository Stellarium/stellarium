# WARNING! Update also the version number in resources/satellites.json,
# otherwise the local copy of that file will be overwritten every time
# Stellarium starts. (Less of a problem if it manages to get one update.)
SET(SATELLITES_VERSION "0.10.12")

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)

ADD_DEFINITIONS(-DSATELLITES_PLUGIN_VERSION="${SATELLITES_VERSION}")
ADD_DEFINITIONS(-DSATELLITES_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
INSTALL(FILES DESTINATION "modules/Satellites")
