# This is the cmake config file for the Quasars plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(QUASARS_VERSION "${VERSION}")

ADD_DEFINITIONS(-DQUASARS_PLUGIN_VERSION="${QUASARS_VERSION}")
ADD_DEFINITIONS(-DQUASARS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
