# This is the cmake config file for the Meteor Showers plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(METEORSHOWERS_VERSION "${VERSION}")

ADD_DEFINITIONS(-DMETEORSHOWERS_PLUGIN_VERSION="${METEORSHOWERS_VERSION}")
ADD_DEFINITIONS(-DMETEORSHOWERS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
