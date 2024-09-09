# This is the cmake config file for the Missing Stars plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(MSP_VERSION "${VERSION}")

ADD_DEFINITIONS(-DMISSINGSTARS_PLUGIN_VERSION="${MSP_VERSION}")
ADD_DEFINITIONS(-DMISSINGSTARS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
