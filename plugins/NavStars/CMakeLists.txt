# This is the cmake config file for the Navigational Stars plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(NS_VERSION "${VERSION}")

ADD_DEFINITIONS(-DNAVSTARS_PLUGIN_VERSION="${NS_VERSION}")
ADD_DEFINITIONS(-DNAVSTARS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
