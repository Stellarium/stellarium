# This is the cmake config file for the Pulsars plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(PSR_VERSION "${VERSION}")

ADD_DEFINITIONS(-DPULSARS_PLUGIN_VERSION="${PSR_VERSION}")
ADD_DEFINITIONS(-DPULSARS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
