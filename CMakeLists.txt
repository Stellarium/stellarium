CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Avoid repeating the IF statement in the ENDIF and ELSE
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF()

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "10")
SET(STELLARIUM_PATCH "3")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

# Use cutsomized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(APPLE 1)
ENDIF()

IF(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")       # additional C compile flags
 SET(CMAKE_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")     # additional CPP compile flags
ELSE()
 SET(CMAKE_C_FLAGS "-Wall -fexceptions -fident -mthreads")
 SET(CMAKE_CXX_FLAGS "-Wall -Wsign-promo -fexceptions -fident -mthreads")
 SET(CMAKE_EXE_LINKER_FLAGS "-Wsign-promo -fexceptions -fident -mthreads -mwindows --large-address-aware")  # --enable-runtime-pseudo-reloc
ENDIF()

SET(OPTIMIZE_INTEL_ATOM 0 CACHE BOOL "Activate optimizations for atom processor.")
IF(OPTIMIZE_INTEL_ATOM)
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
ENDIF()

# best way to get to APPLE?
IF(APPLE)
 SET(CMAKE_INCLUDE_PATH "/usr/X11/include:/usr/X11/include/freetype2:/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include:/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include/freetype2:/sw/include:${CMAKE_INCLUDE_PATH}")
 SET(CMAKE_LIBRARY_PATH "/Developer/SDKs/MacOSX10.5.sdk/usr/X11/lib:/sw/lib:${CMAKE_LIBRARY_PATH}")
 SET(CMAKE_C_FLAGS "-fno-common -Wall -Wextra -Wno-unused-parameter -DMACOSX")
 SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
 SET(CMAKE_EXE_LINKER_FLAGS "-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
ENDIF()

########### Check headers ###########
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(byteswap.h HAVE_BYTESWAP_H)

########### Check functions ###########
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(tzset HAVE_TZSET)
CHECK_FUNCTION_EXISTS(pow10 HAVE_POW10)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)

########### Others ###########
# Activate translation with gettext
SET(RELEASE_BUILD 1 CACHE BOOL "Flag this build as an official release, 1 or 0.")
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported, option are: 1 or 0.")
# Create a dynamic library libstelMain
SET(GENERATE_STELMAINLIB 0)
# Activate sound support
SET(ENABLE_SOUND 0 CACHE BOOL "Define whether sound support should be activated, options are: 1 or 0.")
SET(ENABLE_SCRIPT_CONSOLE 0 CACHE BOOL "Define whether to build the script console feature, options are: 1 or 0.")
SET(ENABLE_STRATOSCRIPT_COMPAT 0 CACHE BOOL "Set to 1 or true if you want to build the Stratoscript compatibility mode")
SET(GUI_MODE Standard CACHE STRING "Choose the type of GUI to build, options are: Standard, None, External")
SET(OPENGL_MODE Desktop CACHE STRING "Choose the type of openGL to use, options are: Desktop, ES2")

IF (GUI_MODE STREQUAL "External")
	SET(EXTERNAL_GUI_PLUGIN_PATH "/path/to/libMyGui.a" CACHE PATH "The absolute path to the .a static library containing the static GUI plugin")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${EXTERNAL_GUI_PLUGIN_PATH})
	SET(EXTERNAL_GUI_PLUGIN_EXTRALIBS CACHE STRING "Additionnal link options needed for the GUI plugin")
	SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${EXTERNAL_GUI_PLUGIN_EXTRALIBS})
ENDIF()

# On WIN32 we need to split the main binary into a small binary and a dll
# This is for allowing to link dynamic plug-ins afterward
IF(WIN32)
  SET(GENERATE_STELMAINLIB 1)
ENDIF()

IF(WIN32)
  LINK_LIBRARIES(wsock32)
ENDIF()

# For Windows, the default back end for the graphics system should be "native", as "raster"
# causes problems for many ATI users.  However, for other platforms we should use raster
# as the default as it is faster and on OSX it fixes a lot of rendering glitches.
IF(WIN32)
  ADD_DEFINITIONS(-DDEFAULT_GRAPHICS_SYSTEM="native")
ELSE()
  ADD_DEFINITIONS(-DDEFAULT_GRAPHICS_SYSTEM="raster")
ENDIF()

########### Plugin setup #############
SET(BUILD_STATIC_PLUGINS 1 CACHE BOOL "Define whether plugins should be compiled statically and linked to the main program")
SET(BUILD_DYNAMIC_PLUGINS 0 CACHE BOOL "Define whether plugins should be compiled dynamically")

SET(USE_PLUGIN_HELLOSTELMODULE 0 CACHE BOOL "Define whether the HelloStelModule plugin should be created.")
SET(USE_PLUGIN_ANGLEMEASURE 1 CACHE BOOL "Define whether the AngleMeasure plugin should be created.")
SET(USE_PLUGIN_COMPASSMARKS 1 CACHE BOOL "Define whether the CompassMarks plugin should be created.")
SET(USE_PLUGIN_SATELLITES 1 CACHE BOOL "Define whether the Satellites plugin should be created.")
SET(USE_PLUGIN_TELESCOPECONTROL 0 CACHE BOOL "Define whether the TelescopeControl plug-in should be created.")
SET(USE_PLUGIN_OCULARS 1 CACHE BOOL "Define whether the Oculars plugin should be created.")
SET(USE_PLUGIN_SVMT 0 CACHE BOOL "Define whether the SVMT plugin should be created.")

########### Static compilation of plugins ###########
# VirGO
SET(USE_STATIC_PLUGIN_VIRGO 0 CACHE BOOL "Define whether the VirGO plugin should be statically linked.")
SET(STATIC_PLUGIN_VIRGO_PATH "$ENV{STELROOT}/../extmodules/VirGO/builds/unix/src/libVirGO.a" CACHE PATH "The location of the libVirGO.a static library")
IF (USE_STATIC_PLUGIN_VIRGO)
## Special non-standard packages
FIND_PATH(CFITSIO_INCLUDE_DIR fitsio.h
	/usr/include
	/usr/local/include
	/scisoft/include
	DOC "Should point to the directory where the include files for libcfitsio are installed")
FIND_LIBRARY(CFITSIO_LIBRARIES cfitsio
	PATHS
	/usr/lib/
	/usr/local/lib
	/scisoft/lib
	/opt/local/lib
	DOC "cfitsio link options")
  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${STATIC_PLUGIN_VIRGO_PATH} ${CFITSIO_LIBRARIES})
  SET(QT_USE_QTXML TRUE) # For VirGO we also need QtXml module
  ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_VIRGO)
ENDIF()

# TextUserInterface
SET(USE_STATIC_PLUGIN_TEXTUSERINTERFACE 0 CACHE BOOL "Define whether the TextUserInterface plugin should be statically linked.")
SET(STATIC_PLUGIN_TEXTUSERINTERFACE_PATH "$ENV{STELROOT}/../extmodules/TextUserInterface/builds/unix/src/libTextUserInterface.a" CACHE PATH "The location of the libTextUserInterface.a static library")
IF (USE_STATIC_PLUGIN_TEXTUSERINTERFACE)
  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${STATIC_PLUGIN_TEXTUSERINTERFACE_PATH})
  ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_TEXTUSERINTERFACE)
ENDIF()


########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself
IF (BUILD_STATIC_PLUGINS)
  IF (USE_PLUGIN_HELLOSTELMODULE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/HelloStelModule/src/libHelloStelModule.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_HELLOSTELMODULE)
  ENDIF()

  IF (USE_PLUGIN_ANGLEMEASURE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/AngleMeasure/src/libAngleMeasure.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_ANGLEMEASURE)
  ENDIF()

  IF (USE_PLUGIN_COMPASSMARKS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/CompassMarks/src/libCompassMarks.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_COMPASSMARKS)
  ENDIF()

  IF (USE_PLUGIN_SATELLITES)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Satellites/src/libSatellites.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SATELLITES)
  ENDIF()

  IF (USE_PLUGIN_TELESCOPECONTROL)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src/libTelescopeControl.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_TELESCOPECONTROL)
  ENDIF()

  IF (USE_PLUGIN_OCULARS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Oculars/src/libOculars.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_OCULARS)
	SET(QT_USE_QTSQL TRUE)
  ENDIF()

  IF (USE_PLUGIN_SVMT)
	## Tokyo Cabinet
	FIND_PATH(TOKYOC_INCLUDE_DIR tcadb.h
	  /usr/include
	  /usr/local/include
	  DOC "Should point to the directory where the include files for tokyo cabinet are installed")
	FIND_LIBRARY(TOKYOC_LIBRARIES tokyocabinet
	  PATHS
	  /usr/lib/
	  /usr/local/lib
	  DOC "tokyo cabinet link options")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/plugins/svmt/src/libSVMT.a ${TOKYOC_LIBRARIES} "/usr/local/Trolltech/Qt-4.6.0/lib/libQtDeclarative.so")
	SET(QT_USE_QTXML TRUE) # For SVMT we also need QtXml module
	SET(QT_USE_QTDECLARATIVE TRUE) # For SVMT we also need QML module, this line doesn't seem to work right now
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SVMT)
  ENDIF()
ENDIF()

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)


########### Find packages ###########
SET(QT_MIN_VERSION "4.6.0")
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTSCRIPT TRUE)
SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTSVG TRUE)
SET(QT_USE_QTTEST TRUE)
IF(ENABLE_SOUND)
  ADD_DEFINITIONS(-DHAVE_QT_PHONON)
  SET(QT_USE_PHONON TRUE)
ENDIF()
IF(ENABLE_SCRIPT_CONSOLE)
  ADD_DEFINITIONS(-DENABLE_SCRIPT_CONSOLE)
ENDIF()
INCLUDE(${QT_USE_FILE})

FIND_PACKAGE(ZLIB)

IF(ENABLE_NLS)
  FIND_PACKAGE(Iconv REQUIRED)
  FIND_PATH(INTL_INCLUDE_DIR libintl.h
	PATHS
	/usr/include
	/sw/include
	/usr/local/include)
  FIND_LIBRARY(INTL_LIBRARIES intl c
	PATHS
	/usr/lib/
	/sw/lib
	/usr/local/lib) # NO_DEFAULT_PATH
  FIND_PACKAGE(Gettext REQUIRED)
ENDIF()


IF (OPENGL_MODE STREQUAL "Desktop")
  FIND_PACKAGE(OpenGL REQUIRED)
ENDIF()
IF(OPENGL_MODE STREQUAL "ES2")
  ADD_DEFINITIONS(-DUSE_OPENGL_ES2 -DQT_GL_NO_SCISSOR_TEST -DQT_OPENGL_LIB -D_REENTRANT -DQT_GL_NO_SCISSOR_TEST -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED)
  # Make sure that no desktop OpenGL libraries are linked when OpenGLES is used
  STRING(REPLACE "-lGLU " "" QT_LIBRARIES "${QT_LIBRARIES}")
  STRING(REPLACE "-lGL" "-lGLESv2" QT_LIBRARIES "${QT_LIBRARIES}")
ENDIF()

########### Set some global variables ###########
IF(UNIX AND NOT WIN32)
IF(APPLE)
  SET(CMAKE_INSTALL_PREFIX "/Users/Shared/stellarium/Stellarium.app/Contents")
  SET(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/Resources/data")
  SET(INSTALL_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/Resources/locale")
ELSE(APPLE)
  SET(INSTALL_DATADIR ${CMAKE_INSTALL_PREFIX}/share/stellarium)
  SET(INSTALL_LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
ENDIF(APPLE)
ELSE()
  SET(INSTALL_DATADIR .)
  SET(INSTALL_LOCALEDIR ./locale)
ENDIF()

########### Get SVN revision for non-release builds ###########
IF(NOT RELEASE_BUILD)
  FIND_PACKAGE(Subversion)
  IF(Subversion_FOUND)
	Subversion_WC_INFO(${PROJECT_SOURCE_DIR} StelSvn)
	SET(SVN_REVISION ${StelSvn_WC_REVISION})
  ENDIF()
ENDIF()

IF(ENABLE_STRATOSCRIPT_COMPAT)
  ADD_DEFINITIONS(-DENABLE_STRATOSCRIPT_COMPAT)
ENDIF()

########### Generate a config.h ###########
SET(PACKAGE_VERSION ${VERSION})
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)
ADD_DEFINITIONS(-DHAVE_CONFIG_H)

IF(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  ADD_DEFINITIONS(-DQT_NO_DEBUG)
  ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

########### uninstall files ###############
CONFIGURE_FILE(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

########### Packaging info for CPack ###########
INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Stellarium is a free open source planetarium for your computer. It shows a realistic sky in 3D, just like what you see with the naked eye, binoculars or a telescope.")
SET(CPACK_PACKAGE_VENDOR "Stellarium's team")
SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README")
SET(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/COPYING")
SET(CPACK_PACKAGE_VERSION_MAJOR ${STELLARIUM_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${STELLARIUM_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${STELLARIUM_PATCH})
SET(CPACK_PACKAGE_INSTALL_DIRECTORY "stellarium")
SET(CPACK_SOURCE_PACKAGE_FILE_NAME "stellarium-${VERSION}")
SET(CPACK_SOURCE_GENERATOR "TGZ")
SET(CPACK_GENERATOR "TGZ")
SET(CPACK_STRIP_FILES "bin/stellarium")
SET(CPACK_PACKAGE_EXECUTABLES "stellarium" "Stellarium")
SET(CPACK_SOURCE_IGNORE_FILES "/CVS/" "/.svn/" "/.git/" "builds/" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "~$" "\\\\.swp$" "\\\\.#" "/#")
INCLUDE(CPack)


########### Macosx Bundling ###############
IF(APPLE)
ADD_CUSTOM_TARGET(macosx_bundle bash util/macosx_bundle.sh ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} DEPENDS install WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "making the macosx bundle." VERBATIM)
ENDIF()


########### Generate doxygen doc ###############
ADD_CUSTOM_TARGET(apidoc doxygen Doxyfile WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generate the doxygen documentation into the builds/doc directory.")

########### enable testing ###########
ENABLE_TESTING()


########### Top level include directories ###########
# This will be used for all compilations in sub-directories
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/core
	${CMAKE_SOURCE_DIR}/src/gui
	${CMAKE_SOURCE_DIR}/src/modules
	${CMAKE_SOURCE_DIR}/src/planetsephems
	${CMAKE_SOURCE_DIR}/src/external
	${CMAKE_SOURCE_DIR}/src/scripting
	${CMAKE_SOURCE_DIR}/src/external/kfilter
	${CMAKE_SOURCE_DIR}/src/external/glues_stel/source/
	${CMAKE_SOURCE_DIR}/src/external/glues_stel/source/libtess
	${OPENGL_INCLUDE_DIR}
	${ICONV_INCLUDE_DIR}
	${INTL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${QT_INCLUDE_DIR}/QtScript
	${QT_QTSCRIPT_INCLUDE_DIR}
	${CMAKE_BINARY_DIR}/src
	${QT_QTTEST_INCLUDE_DIR}
)

########### Subdirectories ###############
ADD_SUBDIRECTORY( data )
ADD_SUBDIRECTORY( src )
ADD_SUBDIRECTORY( textures )
ADD_SUBDIRECTORY( doc )
IF(ENABLE_NLS)
  ADD_SUBDIRECTORY( po )
ENDIF()
ADD_SUBDIRECTORY( landscapes )
ADD_SUBDIRECTORY( skycultures )
ADD_SUBDIRECTORY( nebulae )
ADD_SUBDIRECTORY( scripts )
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
