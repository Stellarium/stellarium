# This is the cmake config file for the Pointer Coordinates plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(POINTERCOORDINATES_VERSION "${VERSION}")

ADD_DEFINITIONS(-DPOINTERCOORDINATES_PLUGIN_VERSION="${POINTERCOORDINATES_VERSION}")
ADD_DEFINITIONS(-DPOINTERCOORDINATES_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )


