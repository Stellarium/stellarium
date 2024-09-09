# This is the cmake config file for the Equation of Time plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(EQUATIONOFTIME_VERSION "${VERSION}")

ADD_DEFINITIONS(-DEQUATIONOFTIME_PLUGIN_VERSION="${EQUATIONOFTIME_VERSION}")
ADD_DEFINITIONS(-DEQUATIONOFTIME_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )


