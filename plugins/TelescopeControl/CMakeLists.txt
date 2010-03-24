
SET(TELESCOPE_CONTROL_MAJOR "0")
SET(TELESCOPE_CONTROL_MINOR "2")
SET(TELESCOPE_CONTROL_PATCH "0")
SET(TELESCOPE_CONTROL_VERSION "${TELESCOPE_CONTROL_MAJOR}.${TELESCOPE_CONTROL_MINOR}.${TELESCOPE_CONTROL_PATCH}")
#Don't forget to increment the version in device_models.json too,
#otherwise the local copy will be overwritten each time Stellarium is started.

SET(USE_TOGGLEABLE_TELESCOPE_WINDOW 1 CACHE BOOL "The telescopes dialog window can be toggled with a keyboard shortcut and/or a toolbar button.")
SET(USE_TOOLBAR_BUTTON 1 CACHE BOOL "Add a button to the bottom toolbar to toggle the telescopes dialog window.")
SET(COMPATIBILITY_001003 1 CACHE BOOL "Make the plug-in compatible with Stellarium version 0.10.3.")

IF(NOT USE_TOGGLEABLE_TELESCOPE_WINDOW)
SET(USE_TOOLBAR_BUTTON 0)
ENDIF(NOT USE_TOGGLEABLE_TELESCOPE_WINDOW)

IF(APPLE)
	SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
	SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)

#SET(TELESCOPE_SERVERS_SOURCE_DIR "$ENV{STELROOT}/../telescope_server/src"
#	CACHE PATH "Should point to the /src/ directory where the stellarium telescope servers source tree is located")

IF(COMPATIBILITY_001003)
 ADD_DEFINITIONS(-DCOMPATIBILITY_001003)
ENDIF()
IF(USE_TOGGLEABLE_TELESCOPE_WINDOW)
 ADD_DEFINITIONS(-DUSE_TOGGLEABLE_TELESCOPE_WINDOW)
ENDIF()
IF(USE_TOOLBAR_BUTTON)
 ADD_DEFINITIONS(-DUSE_TOOLBAR_BUTTON)
ENDIF()
ADD_DEFINITIONS(-DPLUGIN_VERSION="${TELESCOPE_CONTROL_VERSION}")

#Temporary debugging definitions for the telescope servers code
ADD_DEFINITIONS(-DDEBUG3 -DDEBUG4 -DDEBUG5)

ADD_SUBDIRECTORY( src )

INSTALL(FILES DESTINATION "modules/TelescopeControl")


