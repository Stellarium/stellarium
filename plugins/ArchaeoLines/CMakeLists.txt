# This is the cmake config file for the ArchaeoLines plugin
# Versions:
#0.1.3 adding azimuth lines
#0.1.4 adding geographic locations
#0.1.5 using the StelProperty system
#0.1.6 adding arbitrary declinations
#0.1.7 adding license info for GUI
#0.1.8 adding 2 more lines to plugin (selected object azimuth and selected object hour angle)
#0.1.9 Refactor color settings, use properties more sensible.
#0.1.10 Refactor GUI
#0.1.11 Fix crossquarter declination
SET(ARCHAEOLINES_VERSION "0.1.11")
ADD_DEFINITIONS(-DARCHAEOLINES_PLUGIN_VERSION="${ARCHAEOLINES_VERSION}")
ADD_DEFINITIONS(-DARCHAEOLINES_PLUGIN_LICENSE="GNU GPLv2 or later")

ADD_SUBDIRECTORY( src )
