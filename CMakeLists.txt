
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Avoid repeating the IF statement in the ENDIF and ELSE
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "10")
SET(STELLARIUM_PATCH "3")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

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
 SET(CMAKE_EXE_LINKER_FLAGS "-Wsign-promo -fexceptions -fident -mthreads -mwindows --large-address-aware")
ENDIF()

# best way to get to APPLE?
IF(APPLE)
 SET(CMAKE_INCLUDE_PATH "/usr/X11/include:/usr/X11/include/freetype2:/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include:/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include/freetype2:/sw/include:${CMAKE_INCLUDE_PATH}")
 SET(CMAKE_LIBRARY_PATH "/Developer/SDKs/MacOSX10.5.sdk/usr/X11/lib:/sw/lib:${CMAKE_LIBRARY_PATH}")
 SET(CMAKE_C_FLAGS "-fno-common -Wall -Wextra -Wno-unused-parameter -DMACOSX -I/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include -I/Developer/SDKs/MacOSX10.5.sdk/usr/X11/include/freetype2")
 SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}")
 SET(CMAKE_EXE_LINKER_FLAGS "-dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib")
ENDIF()

########### Check headers ###########
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES(byteswap.h HAVE_BYTESWAP_H)

########### Check functions ###########
INCLUDE(CheckFunctionExists)
#toupper strcasecmp strdup putenv setlocale tzset strchr pow10 timegm
CHECK_FUNCTION_EXISTS(tzset HAVE_TZSET)
CHECK_FUNCTION_EXISTS(pow10 HAVE_POW10)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)

########### Static compilation of plugins ###########
# VirGO
SET(USE_STATIC_PLUGIN_VIRGO 0 CACHE BOOL "Define whether the VirGO plugin should be statically linked.")
SET(VIRGO_STATIC_PLUGIN_PATH "$ENV{STELROOT}/../extmodules/VirGO/builds/unix/src/libVirGO.a" CACHE PATH "The location of the libVirGO.a static library")
IF (USE_STATIC_PLUGIN_VIRGO)
  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${VIRGO_STATIC_PLUGIN_PATH}  -ltokyocabinet -lz -lbz2)
  SET(QT_USE_QTXML TRUE) # For VirGO we also need QtXml module
  ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_VIRGO)
ENDIF()

# HelloStelModule
SET(USE_STATIC_PLUGIN_HELLOSTELMODULE 0 CACHE BOOL "Define whether the HelloStelModule plugin should be statically linked.")
SET(HELLOSTELMODULE_STATIC_PLUGIN_PATH "$ENV{STELROOT}/../extmodules/HelloStelModule/builds/unix/src/libHelloStelModule.a" CACHE PATH "The location of the libHelloStelModule.a static library")
IF (USE_STATIC_PLUGIN_HELLOSTELMODULE)
  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${HELLOSTELMODULE_STATIC_PLUGIN_PATH})
  ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_HELLOSTELMODULE)
ENDIF()

########### Others ###########
# Activate translation with gettext
SET(RELEASE_BUILD 1 CACHE BOOL "Flag this build as an official release, 1 or 0.")
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported, option are: 1 or 0.")
# Generate python wrappers
SET(GENERATE_PYTHON_WRAPPERS 0 CACHE BOOL "Define whether python wrapper should be build using SIP, options are: 1 or 0.")
# Create a dynamic library libstelMain
SET(GENERATE_STELMAINLIB 0)
# Activate sound support
SET(ENABLE_SOUND 0 CACHE BOOL "Define whether sound support should be activated, options are: 1 or 0.")
SET(ENABLE_STRATOSCRIPT_COMPAT 0 CACHE BOOL "Set to 1 or true if you want to build the Stratoscript compatibility mode")

# On WIN32 we need to split the main binary into a small binary and a dll
# This is for allowing to link plug-ins afterward
IF (WIN32 OR GENERATE_PYTHON_WRAPPERS)
  SET(GENERATE_STELMAINLIB 1)
ENDIF()

IF(WIN32)
  LINK_LIBRARIES(wsock32)
ENDIF()

IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF()

########### Find packages ###########
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
SET(QT_MIN_VERSION "4.5.0")
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTSCRIPT TRUE)
SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTTEST TRUE)
IF(ENABLE_SOUND)
  ADD_DEFINITIONS(-DHAVE_QT_PHONON)
  SET(QT_USE_PHONON TRUE)
ENDIF()
INCLUDE(${QT_USE_FILE})

IF(ENABLE_NLS)
  FIND_PACKAGE(Iconv REQUIRED)
  FIND_PATH(INTL_INCLUDE_DIR libintl.h
	/usr/include
	/usr/local/include)
  FIND_LIBRARY(INTL_LIBRARIES intl c
	PATHS
	/usr/lib/
	/usr/local/lib)
  FIND_PACKAGE(Gettext REQUIRED)
ENDIF()

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(Boost REQUIRED)
FIND_PACKAGE(FreeType2 REQUIRED)

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
# MESSAGE("Current SVN revision is ${StelSvn_WC_REVISION}")
	SET(SVN_REVISION ${StelSvn_WC_REVISION})
  ENDIF()
ENDIF()

IF(ENABLE_STRATOSCRIPT_COMPAT)
  ADD_DEFINITIONS(-DENABLE_STRATOSCRIPT_COMPAT)
ENDIF()

########### Generate a config.h ###########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
SET(PACKAGE_VERSION ${VERSION})
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

########### Add macros ###########
ADD_DEFINITIONS(-DHAVE_CONFIG_H)

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
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

########### Generate doxygen doc ###############
ADD_CUSTOM_TARGET(apidoc doxygen Doxyfile WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generate the doxygen documentation into the builds/doc directory.")

########### enable testing ###########
ENABLE_TESTING()

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
IF (GENERATE_PYTHON_WRAPPERS)
 ADD_SUBDIRECTORY( sip )
ENDIF()

########### Macosx Bundling ###############
IF(APPLE)
ADD_CUSTOM_TARGET(macosx_bundle bash util/macosx_bundle.sh ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} DEPENDS install WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "making the macosx bundle." VERBATIM)
ENDIF()

