# This is the cmake config file for the Historical Supernovae plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(SNE_VERSION "${VERSION}")

ADD_DEFINITIONS(-DSUPERNOVAE_PLUGIN_VERSION="${SNE_VERSION}")
ADD_DEFINITIONS(-DSUPERNOVAE_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
