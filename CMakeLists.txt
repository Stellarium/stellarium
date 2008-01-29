CMAKE_MINIMUM_REQUIRED(VERSION 2.4.2)

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "10")
SET(STELLARIUM_PATCH "0")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  SET(MACOSX 1)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

IF(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS -Wall)       # additional C compile flags
 SET(CMAKE_CXX_FLAGS -Wall)     # additional CPP compile flags
ELSE(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS "-Wall -fexceptions -fident -mthreads")
 SET(CMAKE_CXX_FLAGS "-Wall -Wsign-promo -fexceptions -fident -mthreads")
 SET(CMAKE_EXE_LINKER_FLAGS "-Wsign-promo -fexceptions -fident -mthreads")
ENDIF(UNIX AND NOT WIN32)

# best way to get to MACOSX?
IF(MACOSX)
 SET(CMAKE_INCLUDE_PATH "/usr/X11R6/include:/usr/X11R6/include/freetype2:/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include:/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include/freetype2:/sw/include:${CMAKE_INCLUDE_PATH}")
 SET(CMAKE_LIBRARY_PATH "/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/lib:/sw/lib:${CMAKE_LIBRARY_PATH}")
 SET(CMAKE_C_FLAGS "-fno-common -Wall -DMACOSX -I/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include -I/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include/freetype2 -L/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/lib")
 SET(CMAKE_CXX_FLAGS "-fno-common -Wall -DMACOSX -I/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include -I/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/include/freetype2 -L/Developer/SDKs/MacOSX10.4u.sdk/usr/X11R6/lib")
ENDIF(MACOSX)


########### Check headers ###########
INCLUDE(CheckIncludeFiles)
CHECK_INCLUDE_FILES( unistd.h HAVE_UNISTD_H)
CHECK_INCLUDE_FILES( string.h HAVE_STRING_H)
CHECK_INCLUDE_FILES( byteswap.h HAVE_BYTESWAP_H)

########### Check functions ###########
INCLUDE(CheckFunctionExists)
#toupper strcasecmp strdup putenv setlocale tzset strchr pow10 timegm
CHECK_FUNCTION_EXISTS(timegm HAVE_TIMEGM)
CHECK_FUNCTION_EXISTS(mktime HAVE_MKTIME)
CHECK_FUNCTION_EXISTS(tzset HAVE_TZSET)
CHECK_FUNCTION_EXISTS(pow10 HAVE_POW10)
CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
CHECK_FUNCTION_EXISTS(mktime HAVE_MKTIME)
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)

########### Others ###########
# Activate translation with gettext
SET(ENABLE_NLS 1)
# Generate python wrappers
SET(GENERATE_PYTHON_WRAPPERS 0)

########### Find packages ###########
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
FIND_PACKAGE(Qt4 REQUIRED)
SET(QT_USE_QTOPENGL TRUE)
SET(QT_USE_QTSCRIPT TRUE)
SET(QT_USE_QTNETWORK TRUE)
INCLUDE(${QT_USE_FILE})


SET(SDL_LIBRARY "")
FIND_PACKAGE(SDL_mixer)

IF(SDLMIXER_LIBRARY)
  MESSAGE(STATUS "Found SDL_mixer. Activate sound support.")
  SET(HAVE_SDL_SDL_MIXER_H 1)
ELSE(SDLMIXER_LIBRARY)
  MESSAGE(STATUS "No sound support.")
  SET(SDLMIXER_INCLUDE_DIR "")
  SET(SDLMIXER_LIBRARY "")
ENDIF(SDLMIXER_LIBRARY)

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
ENDIF(ENABLE_NLS)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(Boost REQUIRED)
FIND_PACKAGE(FreeType2 REQUIRED)

########### Set some global variables ###########
IF(UNIX AND NOT WIN32)
IF(MACOSX)
  SET(CMAKE_INSTALL_PREFIX "/Users/Shared/stellarium/Stellarium.app/Contents")
  SET(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/Resources/data")
  SET(INSTALL_LOCALEDIR "${CMAKE_INSTALL_PREFIX}/Resources/locale")
ELSE(MACOSX)
  SET(INSTALL_DATADIR ${CMAKE_INSTALL_PREFIX}/share/stellarium)
  SET(INSTALL_LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
ENDIF(MACOSX)
ELSE(UNIX AND NOT WIN32)
  SET(INSTALL_DATADIR .)
  SET(INSTALL_LOCALEDIR ./locale)
ENDIF(UNIX AND NOT WIN32)

########### Generate a config.h ###########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
SET(HAVE_LIBCURL ${CURL_FOUND})
SET(PACKAGE_VERSION ${VERSION})
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

########### Add macros ###########
ADD_DEFINITIONS(-DHAVE_CONFIG_H)

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
set(CPACK_SOURCE_IGNORE_FILES "/CVS/" "/.svn/" "/.git/" "builds/" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "~$" "\\\\.swp$" "\\\\.#" "/#")
INCLUDE(CPack)

########### Generate doxygen doc ###############
ADD_CUSTOM_TARGET(doc doxygen Doxyfile WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generate the doxygen documentation into the builds/doc directory.")

########### Subdirectories ###############
ADD_SUBDIRECTORY( data ) 
ADD_SUBDIRECTORY( src ) 
ADD_SUBDIRECTORY( textures ) 
ADD_SUBDIRECTORY( doc ) 
ADD_SUBDIRECTORY( po ) 
ADD_SUBDIRECTORY( landscapes ) 
ADD_SUBDIRECTORY( skycultures ) 
ADD_SUBDIRECTORY( nebulae ) 
ADD_SUBDIRECTORY( scripts ) 
ADD_SUBDIRECTORY( stars ) 
IF (GENERATE_PYTHON_WRAPPERS)
 ADD_SUBDIRECTORY( sip ) 
ENDIF(GENERATE_PYTHON_WRAPPERS)
########### Macosx Bundling ###############
IF(MACOSX)
ADD_CUSTOM_TARGET(macosx_bundle mv ${CMAKE_INSTALL_PREFIX}/bin ${CMAKE_INSTALL_PREFIX}/MacOS COMMAND mv ${CMAKE_INSTALL_PREFIX}/share ${CMAKE_INSTALL_PREFIX}/Resources COMMAND mv ${CMAKE_INSTALL_PREFIX}/Resources/stellarium/* ${CMAKE_INSTALL_PREFIX}/Resources COMMAND rmdir ${CMAKE_INSTALL_PREFIX}/Resources/stellarium COMMAND mkdir ${CMAKE_INSTALL_PREFIX}/Frameworks COMMAND perl util/pkgApp.pl ${CMAKE_INSTALL_PREFIX} MacOS/stellarium Frameworks DEPENDS install WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "making the macosx bundle." VERBATIM)
ENDIF(MACOSX)

