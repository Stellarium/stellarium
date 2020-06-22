# WARNING! Update also the version number in resources/satellites.json,
# otherwise the local copy of that file will be overwritten every time
# Stellarium starts. (Less of a problem if it manages to get one update.)
SET(SATELLITES_VERSION "0.12.0")

SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)

ADD_DEFINITIONS(-DSATELLITES_PLUGIN_VERSION="${SATELLITES_VERSION}")
ADD_DEFINITIONS(-DSATELLITES_PLUGIN_LICENSE="GNU GPLv2 or later")
# set this to 1 to compile Iridium parts. They have been disabled as on v0.20.2 due to Iridium flares no longer available.
# However, we should retain the code, just in case similar satellites will be developed.
ADD_DEFINITIONS(-DSATELLITES_PLUGIN_IRIDIUM=0)

ADD_SUBDIRECTORY( src )
INSTALL(FILES DESTINATION "modules/Satellites")
