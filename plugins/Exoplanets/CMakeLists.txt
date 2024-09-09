# This is the cmake config file for the Exoplanets plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(EXOPLANETS_VERSION "${VERSION}")

ADD_DEFINITIONS(-DEXOPLANETS_PLUGIN_VERSION="${EXOPLANETS_VERSION}")
ADD_DEFINITIONS(-DEXOPLANETS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
