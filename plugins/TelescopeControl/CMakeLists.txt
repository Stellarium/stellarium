CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# This is the cmake config file for the TelescopeControl plugin

########### Project name ###########
PROJECT(TelescopeControl)
SET(TELESCOPE_CONTROL_MAJOR "0")
SET(TELESCOPE_CONTROL_MINOR "1")
SET(TELESCOPE_CONTROL_PATCH "8")
SET(VERSION "${TELESCOPE_CONTROL_MAJOR}.${TELESCOPE_CONTROL_MINOR}.${TELESCOPE_CONTROL_PATCH}")
SET(PACKAGE TelescopeControl)

SET(BUILD_STATIC_LIB 0 CACHE BOOL "Build the static version of the library.")
SET(BUILD_DYNAMIC_LIB 1 CACHE BOOL "Build the dynamic (plugin) version of the library.")

SET(USE_TOGGLEABLE_TELESCOPE_WINDOW 0 CACHE BOOL "The telescopes dialog window can be toggled with a keyboard shortcut and/or a toolbar button.")
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

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Debug CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

# Please indicate below where your stellarium source and build trees are
SET(STELLARIUM_SOURCE_DIR "$ENV{STELROOT}/src" CACHE PATH
    "Should point to the src/ directory where the stellarium source tree is located")

SET(TELESCOPE_SERVERS_SOURCE_DIR "$ENV{STELROOT}/../telescope_server/src"
    CACHE PATH "Should point to the /src/ directory where the stellarium telescope servers source tree is located")


# work out where we have built the main Stellarium code
# NOTE: plugins bild type (Release/Debug) must match the Stellarium
# build type, else they probably won't build.
IF(WIN32)
 SET(BUILD_SUBDIR msys)
ElSE(WIN32)
 SET(BUILD_SUBDIR unix)
ENDIF(WIN32)

SET(STELLARIUM_BUILD_DIR "$ENV{STELROOT}/builds/${BUILD_SUBDIR}" CACHE PATH 
    "Should point to the directory where stellarium is build in Release mode.")

IF(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS -Wall)       # additional C compile flags
 SET(CMAKE_CXX_FLAGS -Wall)     # additional CPP compile flags
ELSE(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS "-Wall -fexceptions -fident -mthreads")
 SET(CMAKE_CXX_FLAGS "-Wall -Wsign-promo -fexceptions -fident -mthreads")
 SET(CMAKE_EXE_LINKER_FLAGS "-Wsign-promo -fexceptions -fident -mthreads --enable-runtime-pseudo-reloc")
ENDIF(UNIX AND NOT WIN32)

IF(APPLE)
  SET(CMAKE_C_FLAGS "-Wall -DMACOSX -I/usr/X11R6/include -I/sw/include -L/usr/X11R6/lib -L/sw/lib")
  SET(CMAKE_CXX_FLAGS "-Wall -DMACOSX -I/usr/X11R6/include -I/sw/include -L/usr/X11R6/lib -L/sw/lib")
ENDIF(APPLE)

########### Find packages ###########
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
SET(QT_MIN_VERSION "4.5.0")
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})

FIND_PACKAGE(OpenGL REQUIRED)

########### Add macros ###########
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
ADD_DEFINITIONS(-DQT_PLUGIN)
ADD_DEFINITIONS(-DQT_DLL)
ADD_DEFINITIONS(-D_REENTRANT)
IF(COMPATIBILITY_001003)
 ADD_DEFINITIONS(-DCOMPATIBILITY_001003)
ENDIF(COMPATIBILITY_001003)
IF(USE_TOGGLEABLE_TELESCOPE_WINDOW)
 ADD_DEFINITIONS(-DUSE_TOGGLEABLE_TELESCOPE_WINDOW)
ENDIF(USE_TOGGLEABLE_TELESCOPE_WINDOW)
IF(USE_TOOLBAR_BUTTON)
 ADD_DEFINITIONS(-DUSE_TOOLBAR_BUTTON)
ENDIF(USE_TOOLBAR_BUTTON)
ADD_DEFINITIONS(-DPLUGIN_VERSION="${VERSION}")

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
 ADD_DEFINITIONS(-DQT_NO_DEBUG)
ENDIF(CMAKE_BUILD_TYPE STREQUAL "Release")

########### Subdirectories ############### 
ADD_SUBDIRECTORY( src ) 

INSTALL(FILES DESTINATION "modules/${PACKAGE}")

########### Packaging info for CPack ###########
SET(CPACK_PACKAGE_NAME "TelescopeControl")
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "TelescopeControl plugin for Stellarium")
SET(CPACK_PACKAGE_VENDOR "Bogdan Marinov")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR ${TELESCOPE_CONTROL_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TELESCOPE_CONTROL_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TELESCOPE_CONTROL_PATCH})
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE}-${VERSION}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_SOURCE_STRIP_FILES "")
SET(CPACK_SOURCE_IGNORE_FILES "/CVS/" "/.svn/" "/.git/" "builds/" "~$" "\\\\.swp$" "\\\\.#" "/#")
INCLUDE(CPack)

