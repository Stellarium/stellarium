# This is the cmake config file for the Bright Novae plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(NOVAE_VERSION "${VERSION}")

ADD_DEFINITIONS(-DNOVAE_PLUGIN_VERSION="${NOVAE_VERSION}")
ADD_DEFINITIONS(-DNOVAE_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
