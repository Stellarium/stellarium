SET(SATELLITES_VERSION "0.12.1")

ADD_DEFINITIONS(-DSATELLITES_PLUGIN_VERSION="${SATELLITES_VERSION}")
ADD_DEFINITIONS(-DSATELLITES_PLUGIN_LICENSE="GNU GPLv2 or later")
# set this to 1 to compile Iridium parts. They have been disabled as on v0.20.2 due to Iridium flares no longer available.
# However, we should retain the code, just in case similar satellites will be developed.
ADD_DEFINITIONS(-DSATELLITES_PLUGIN_IRIDIUM=0)

ADD_SUBDIRECTORY( src )
