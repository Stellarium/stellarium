
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

# Avoid repeating the IF statement in the ENDIF and ELSE
SET(CMAKE_ALLOW_LOOSE_LOOP_CONSTRUCTS true)

IF(COMMAND cmake_policy)
  CMAKE_POLICY(SET CMP0003 NEW)
ENDIF()

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "11")
SET(STELLARIUM_PATCH "2")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(APPLE 1)
ENDIF()


IF(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")       # additional C compile flags
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")     # additional CPP compile flags
ELSE()
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fexceptions -fident -mthreads")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wsign-promo -fexceptions -fident -mthreads")
 SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wsign-promo -fexceptions -fident -mthreads -mwindows --large-address-aware")  # --enable-runtime-pseudo-reloc --verbose
ENDIF()

SET(OPTIMIZE_INTEL_ATOM 0 CACHE BOOL "Activate optimizations for atom processor.")
IF(OPTIMIZE_INTEL_ATOM)
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
 SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic -msse -msse2 -msse3 -mfpmath=sse")
ENDIF()

SET(BUILD_FOR_MAEMO 0 CACHE BOOL "Set to 1 or true if you want to build for maemo target")
IF(BUILD_FOR_MAEMO)
  # Optimize for N900 ARM processor
  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls -D_REENTRANT -DQT_GL_NO_SCISSOR_TEST -DQT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH=1024 -mtune=cortex-a8 -march=armv7-a -ftree-vectorize -mfpu=neon -mfloat-abi=softfp -ffast-math -funroll-loops -DBUILD_FOR_MAEMO")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fno-optimize-sibling-calls -D_REENTRANT -DQT_GL_NO_SCISSOR_TEST -DQT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH=1024 -mtune=cortex-a8 -march=armv7-a -ftree-vectorize -mfpu=neon -mfloat-abi=softfp -ffast-math -funroll-loops -DBUILD_FOR_MAEMO")
ENDIF()

SET(BUILD_FOR_ANDROID 1 CACHE BOOL "Set to 1 or true if you want to build for Android target")
IF(BUILD_FOR_ANDROID)
  #android-specific defaults
  SET(OPENGL_MODE ES2 CACHE STRING "Choose the type of openGL to use, options are: Desktop, ES2")
  SET(BUILD_STATIC_PLUGINS 0 CACHE BOOL "Define whether plugins should be compiled statically and linked to the main program")
  
  #c/o Navit project
  FIND_HOST_PROGRAM(ANDROID_LOCATION NAMES android android.bat)
  FIND_HOST_PROGRAM(ANT_LOCATION NAMES ant)
  
  if (NOT ANT_LOCATION)
    message(FATAL_ERROR "Could not find ant. Please install ant and add it to the search path.")
  endif()
  if (NOT ANDROID_LOCATION)
    message(FATAL_ERROR "Could not find 'Android SDK and AVD Manager'. Please install the android sdk and add the <android-sdk>/tools directory to the search path.")
  endif()

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DQT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH=1024  -DQT_GL_NO_SCISSOR_TEST")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DQT_DEFAULT_TEXTURE_GLYPH_CACHE_WIDTH=1024  -DQT_GL_NO_SCISSOR_TEST")

  #configure the mobile GUI plugin. Android defaults:
  SET(EXTERNAL_GUI_SRC_PATH "../plugins/MobileGui" CACHE PATH "The location of the plugin main directory i.e. the one containing the top level CMakeLists.txt")
  SET(GUI_MODE External CACHE STRING "Choose the type of GUI to build, options are: Standard, None, External")
  
  #extra QT Mobility libraries; these aren't picked up by cmake's built-in scripts. Setting them manually for now.
  SET(QTMOBILITY_LIBRARIES "/c/Users/Brady/necessitas/Android/Qt/480/armeabi-v7a/lib/libQtSystemInfo.so" CACHE STRING "The paths to any necessary QtMobility libraries")
  SET(QTMOBILITY_INCLUDES "-Ic:/Users/Brady/necessitas/Android/Qt/480/armeabi-v7a/include/QtMobility -I/c/Users/Brady/necessitas/Android/Qt/480/armeabi-v7a/include/QtSystemInfo" CACHE STRING "Additional build compiler options for QtMobility include directories")

  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${QTMOBILITY_INCLUDES}")
  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${QTMOBILITY_INCLUDES}")
  
  #and CMake doesn't seem to find the rest of Qt automatically either. Is Necessitas using odd locations?...
  SET(NECESSITAS_QT_ROOT "C:/Users/Brady/necessitas/Android/Qt/480" CACHE STRING "Android: Location of necessitas Qt")
  
  SET(NECESSITAS_QT_ROOT_ARCH "${NECESSITAS_QT_ROOT}/${ARM_TARGET}")
  #SET(QT_QMAKE_EXECUTABLE ${NECESSITAS_QT_ROOT_ARCH}/bin/qmake${TOOL_OS_SUFFIX})
  SET(QT_QMAKE_EXECUTABLE "${NECESSITAS_QT_ROOT_ARCH}/bin/qmake")
  SET(CMAKE_FIND_ROOT_PATH ${CMAKE_FIND_ROOT_PATH} ${NECESSITAS_QT_ROOT_ARCH})
  #message(FATAL_ERROR "${CMAKE_FIND_ROOT_PATH}")
  
ENDIF()

#IF (GUI_MODE STREQUAL "External")
#	SET(EXTERNAL_GUI_SRC_PATH "../../MyGui/" CACHE PATH "The location of the plugin main directory i.e. the one containing the top level CMakeLists.txt")
#	ADD_SUBDIRECTORY( ${EXTERNAL_GUI_SRC_PATH} externalGui )
#	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/externalGui/src/libExternalGui.a)
#	SET(EXTERNAL_GUI_PLUGIN_EXTRALIBS CACHE STRING "Additionnal link options needed for the GUI plugin")
#	SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${EXTERNAL_GUI_PLUGIN_EXTRALIBS})
#ENDIF()

# best way to get to APPLE?
IF(APPLE)
 SET(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-common -Wall -Wextra")
 SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -Wall -Wextra -Wno-unused-parameter")

 # uncomment one of the following, depending on minimum OS level you want
 #SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.5")
 #SET(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.5.sdk/")
 #SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.6")
 #SET(CMAKE_OSX_SYSROOT "/Developer/SDKs/MacOSX10.6.sdk/")
 
 # uncomment one of the following, depending on what platforms you want.
 # NOTE: Qt build for Cocoa is Intel only.  Qt with Carbon is both, but does not seem
 # to handle 64bit well.
 #SET(CMAKE_OSX_ARCHITECTURES "i386;x86_64;ppc;ppc64")
 #SET(CMAKE_OSX_ARCHITECTURES "i386;x86_64")
 #SET(CMAKE_OSX_ARCHITECTURES "x86_64")
ENDIF()

########### Check functions ###########
INCLUDE(CheckFunctionExists)
SET(CMAKE_REQUIRED_LIBRARIES "-lm")
CHECK_FUNCTION_EXISTS(pow10 HAVE_POW10)
SET(CMAKE_REQUIRED_LIBRARIES)
IF(HAVE_POW10)
 ADD_DEFINITIONS(-DHAVE_POW10)
ENDIF()

########### Others ###########
# Activate translation with gettext
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported.")
IF(ENABLE_NLS)
  ADD_DEFINITIONS(-DENABLE_NLS)
ENDIF()

SET(RELEASE_BUILD 1 CACHE BOOL "Flag this build as an official release.")

# Activate sound support
SET(ENABLE_SOUND 0 CACHE BOOL "Define whether sound support should be activated.")

SET(ENABLE_SCRIPTING 1 CACHE BOOL "Define whether scripting features should be activated.")
IF(ENABLE_SCRIPTING)
  # (De-)Activate the script edit console
  SET(ENABLE_SCRIPT_CONSOLE 1 CACHE BOOL "Define whether to build the script console feature.")
  IF(ENABLE_SCRIPT_CONSOLE)
	ADD_DEFINITIONS(-DENABLE_SCRIPT_CONSOLE)
  ENDIF()
  SET(ENABLE_STRATOSCRIPT_COMPAT 0 CACHE BOOL "Set to 1 or true if you want to build the Stratoscript compatibility mode")
ELSE()
  ADD_DEFINITIONS(-DDISABLE_SCRIPTING)
ENDIF()


SET(GUI_MODE Standard CACHE STRING "Choose the type of GUI to build, options are: Standard, None, External")
SET(OPENGL_MODE Desktop CACHE STRING "Choose the type of openGL to use, options are: Desktop, ES2")

SET(GENERATE_PACKAGE_TARGET 1 CACHE BOOL "Set to 1 or true if you want to have make package target")

# On WIN32 we need to split the main binary into a small binary and a dll
# This is for allowing to link dynamic plug-ins afterward
IF(WIN32)
  SET(GENERATE_STELMAINLIB 1)
ELSE()
  SET(GENERATE_STELMAINLIB 0)
ENDIF()

# For Windows, the default back end for the graphics system should be "native", as "raster"
# causes problems for many ATI users.  However, for other platforms we should use raster
# as the default as it is faster.
# Mac 10.6 requires native as well.
IF(WIN32 OR APPLE)
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
SET(USE_PLUGIN_TELESCOPECONTROL 1 CACHE BOOL "Define whether the TelescopeControl plug-in should be created.")
SET(USE_PLUGIN_LOGBOOK 0 CACHE BOOL "Define whether the LogBook plugin should be created.")
SET(USE_PLUGIN_OCULARS 1 CACHE BOOL "Define whether the Oculars plugin should be created.")
SET(USE_PLUGIN_SVMT 0 CACHE BOOL "Define whether the SVMT plugin should be created.")
SET(USE_PLUGIN_TEXTUSERINTERFACE 1 CACHE BOOL "Define whether the TextUserInterface plugin should be created.")
SET(USE_PLUGIN_TIMEZONECONFIGURATION 1 CACHE BOOL "Define whether the TimeZoneConfiguration plugin should be created.")
SET(USE_PLUGIN_VIRGO 0 CACHE BOOL "Define whether the VirGO plugin should be created.")
SET(USE_PLUGIN_SOLARSYSTEMEDITOR 1 CACHE BOOL "Define whether the Solar System Editor should be built.")
SET(USE_PLUGIN_SUPERNOVAE 1 CACHE BOOL "Define whether the Historical Supernova plugin should be created.")
SET(USE_PLUGIN_QUASARS 1 CACHE BOOL "Define whether the Quasars plugin should be created.")

########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself
IF (BUILD_STATIC_PLUGINS)
  IF (USE_PLUGIN_SVMT)
	SET(QT_USE_QTXML TRUE) # For SVMT we also need QtXml module
	SET(QT_USE_QTDECLARATIVE TRUE) # For SVMT we also need QML module, this line doesn't seem to work right now
	SET(QT_USE_QTWEBKIT TRUE) # For SVMT we also need QML module, this line doesn't seem to work right now
  ENDIF()

  IF (USE_PLUGIN_VIRGO)
	## Library for loading FITS image files
	FIND_PATH(CFITSIO_INCLUDE_DIR fitsio.h
	  /usr/include
	  /usr/local/include
	  DOC "Should point to the directory where the include files for libcfitsio are installed")
	FIND_LIBRARY(CFITSIO_LIBRARIES cfitsio
	  PATHS
	  /usr/lib/
	  /usr/local/lib
	  /opt/local/lib
	  DOC "cfitsio link options")
	SET(QT_USE_QTXML TRUE) # For VirGO we also need QtXml module
  ENDIF()

  IF (USE_PLUGIN_LOGBOOK)
	SET(QT_USE_QTSQL TRUE) # For LogBook we also need SQL module
  ENDIF()

ENDIF()

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)

########### Find packages ###########
SET(QT_MIN_VERSION "4.6.0")

FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
IF(ENABLE_SCRIPTING)
  SET(QT_USE_QTSCRIPT TRUE)
ENDIF()
SET(QT_USE_QTNETWORK TRUE)
SET(QT_USE_QTTEST TRUE)
IF(ENABLE_SOUND)
  ADD_DEFINITIONS(-DHAVE_QT_PHONON)
  SET(QT_USE_PHONON TRUE)
ENDIF()
INCLUDE(${QT_USE_FILE})

# Get rid of the QtTest dependency for the main binary. It is re-added manually when compiling tests.
STRING(REPLACE "${QT_QTTEST_LIBRARY}" "" QT_LIBRARIES "${QT_LIBRARIES}")
# Get rid of the GLU dependency, it's not needed in stellarium.
STRING(REPLACE "-lGLU " "" QT_LIBRARIES "${QT_LIBRARIES}")

FIND_PACKAGE(ZLIB)

IF(ENABLE_NLS)
  IF(BUILD_FOR_ANDROID)
    ADD_LIBRARY(intllite STATIC "src/core/external/libintl-lite/libintl-lite.cpp")
	SET(INTL_LIBRARIES ${CMAKE_BINARY_DIR}/libintllite.a)
	
	FIND_HOST_PACKAGE(Gettext REQUIRED)
	FIND_HOST_PACKAGE(XGettext REQUIRED)
	
	#assumes these utilities are on the path
	#we need these utilities, and we need them from the local system, not the NDK
	#todo: find a better way to do this
	#SET(GETTEXT_MSGFMT_EXECUTABLE msgfmt)
	#SET(GETTEXT_MSGMERGE_EXECUTABLE msgmerge)
	
    #FIND_HOST_PROGRAM(GETTEXT_MSGFMT_EXECUTABLE NAMES msgfmt msgfmt.exe)
    #FIND_HOST_PROGRAM(GETTEXT_MSGMERGE_EXECUTABLE NAMES msgmerge msgmerge.exe)
  ELSE()
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
  FIND_PACKAGE(XGettext REQUIRED)
  ENDIF()
ENDIF()


IF (OPENGL_MODE STREQUAL "Desktop")
  FIND_PACKAGE(OpenGL REQUIRED)
ENDIF()
IF(OPENGL_MODE STREQUAL "ES2")
  ADD_DEFINITIONS(-DUSE_OPENGL_ES2 -DQT_OPENGL_LIB -D_REENTRANT -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED)
  # Make sure that no desktop OpenGL libraries are linked when OpenGLES is used
  STRING(REPLACE "-lGL" "-lGLESv2" QT_LIBRARIES "${QT_LIBRARIES}")
ENDIF()

########### Set some global variables ###########
IF(BUILD_FOR_ANDROID)
  ADD_DEFINITIONS(-DINSTALL_DATADIR="//sdcard/stellarium")
  ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="//sdcard/stellarium/locale")
  SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/Stellarium-apk")
ELSEIF(UNIX AND NOT WIN32)
IF(APPLE)
  SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/Stellarium.app/Contents")
ELSE(APPLE)
  ADD_DEFINITIONS(-DINSTALL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/stellarium")
  ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
ENDIF()
ELSE()
  ADD_DEFINITIONS(-DINSTALL_DATADIR=".")
  ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="./locale")
ENDIF()

########### Get revision number for non-release builds ###########
IF(NOT RELEASE_BUILD)
  #Hack until CMake detection for Bazaar is finished
  IF(BZR_REVISION)
	SET(PACKAGE_VERSION "bzr${BZR_REVISION}")
	ADD_DEFINITIONS(-DBZR_REVISION="${BZR_REVISION}")
  ELSE()
	FIND_PACKAGE(Subversion)
	IF(Subversion_FOUND)
	  Subversion_WC_INFO(${PROJECT_SOURCE_DIR} StelSvn)
	  SET(SVN_REVISION ${StelSvn_WC_REVISION})
	  ADD_DEFINITIONS(-DSVN_REVISION="${SVN_REVISION}")
	ENDIF()
  ENDIF()
ENDIF()

IF(ENABLE_STRATOSCRIPT_COMPAT)
  ADD_DEFINITIONS(-DENABLE_STRATOSCRIPT_COMPAT)
ENDIF()


### Generate an Inno Setup project file ###
IF(WIN32)
  SET(CONFIGURED_FILE_WARNING "Do not edit this file! It has been automatically generated by CMake. Your changes will be lost the next time CMake is run.")
  CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium.iss.cmake ${CMAKE_SOURCE_DIR}/stellarium.iss @ONLY)
ENDIF(WIN32)

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

IF(GENERATE_PACKAGE_TARGET)
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
  SET(CPACK_SOURCE_IGNORE_FILES "/CVS/" "/.svn/" "/.git/" "/.bzr/" "builds/" "installers/" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "/CMakeLists.txt.user$" "\\\\.bzrignore$" "~$" "\\\\.swp$" "\\\\.#" "/#")
  INCLUDE(CPack)
ENDIF()

########### Macosx Bundling ###############
IF(APPLE)
ADD_CUSTOM_TARGET(macosx_bundle bash util/macosx_bundle.sh ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} DEPENDS install WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "making the macosx bundle." VERBATIM)
ENDIF()


########### Generate doxygen doc ###############
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
ADD_CUSTOM_TARGET(apidoc doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate the doxygen documentation into the doc directory.")


########### Top level include directories ###########
# This will be used for all compilations in sub-directories
INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/core
	${CMAKE_SOURCE_DIR}/src/core/modules
	${CMAKE_SOURCE_DIR}/src/core/planetsephems
	${CMAKE_SOURCE_DIR}/src/core/external
	${CMAKE_SOURCE_DIR}/src/core/external/kfilter
	${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/
	${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/libtess
	${CMAKE_SOURCE_DIR}/src/gui
	${CMAKE_SOURCE_DIR}/src/scripting
	${OPENGL_INCLUDE_DIR}
	${ICONV_INCLUDE_DIR}
	${INTL_INCLUDE_DIR}
	${ZLIB_INCLUDE_DIR}
	${QT_INCLUDE_DIR}/QtScript
	${QT_QTSCRIPT_INCLUDE_DIR}
	${CMAKE_BINARY_DIR}/src
	${QT_QTTEST_INCLUDE_DIR}
	${QT_DECLARATIVE_INCLUDE_DIR}
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
IF(ENABLE_SCRIPTING)
  ADD_SUBDIRECTORY( scripts )
ENDIF()
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
IF(BUILD_FOR_ANDROID)
  ADD_SUBDIRECTORY( android )
ENDIF()