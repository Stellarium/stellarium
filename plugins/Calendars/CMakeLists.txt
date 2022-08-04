# This is the cmake config file for the Calendars plugin
# Further: Use program version number. (Static plugin, why bother?)
SET(CALENDARS_VERSION "${VERSION}")

ADD_DEFINITIONS(-DCALENDARS_PLUGIN_VERSION="${CALENDARS_VERSION}")
ADD_DEFINITIONS(-DCALENDARS_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
