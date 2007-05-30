########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "9")
SET(STELLARIUM_PATCH "0")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)

########### Main global variables ###########
SET(CMAKE_BUILD_TYPE Debug)   # Choices are None Debug Release RelWithDebInfo MinSizeRel

IF(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS -Wall)       # additional C compile flags
 SET(CMAKE_CXX_FLAGS -Wall)     # additional CPP compile flags
 SET(STLPORT_LIBRARY "")
ELSE(UNIX AND NOT WIN32)
 SET(CMAKE_C_FLAGS "-Wall -fexceptions -fident -mthreads -D_STLP_NO_CUSTOM_IO -D_STLP_USE_DYNAMIC_LIB  -I/home/fchereau/STLport/stlport -L/home/fchereau/STLport/lib")
 SET(CMAKE_CXX_FLAGS "-Wall -Wsign-promo -fexceptions -fident -mthreads -D_STLP_NO_CUSTOM_IO -D_STLP_USE_DYNAMIC_LIB  -I/home/fchereau/STLport/stlport -L/home/fchereau/STLport/lib")
 SET(STLPORT_LIBRARY "-lstlport.5.0")
 SET(CMAKE_EXE_LINKER_FLAGS "-Wsign-promo -fexceptions -fident -mthreads -L/home/fchereau/STLport/lib")
ENDIF(UNIX AND NOT WIN32)



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
SET(USE_QT4 1)
#SET(USE_SDL 1)

########### Find packages ###########
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
IF(USE_QT4)
  FIND_PACKAGE(Qt4 REQUIRED)
  SET(QT_USE_QTOPENGL TRUE)
  INCLUDE(${QT_USE_FILE})
ENDIF(USE_QT4)

IF(USE_SDL)
  FIND_PACKAGE(SDL)
ENDIF(USE_SDL)

FIND_PACKAGE(OpenGL REQUIRED)
FIND_PACKAGE(PNG REQUIRED)
FIND_PACKAGE(JPEG REQUIRED)
FIND_PACKAGE(Boost REQUIRED)
FIND_PACKAGE(FreeType2 REQUIRED)
FIND_PACKAGE(CURL)
FIND_PACKAGE(Gettext REQUIRED)

########### Generate a config.h ###########
INCLUDE(TestBigEndian)
TEST_BIG_ENDIAN(WORDS_BIGENDIAN)
SET(HAVE_LIBCURL ${CURL_FOUND})
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

########### Set some global variables ###########
IF(UNIX AND NOT WIN32)
  SET(INSTALL_DATADIR ${CMAKE_INSTALL_PREFIX}/share/stellarium)
  SET(INSTALL_LOCALEDIR ${CMAKE_INSTALL_PREFIX}/share/locale)
  SET(DATA_DESTINATION "share/stellarium")
  SET(PO_DESTINATION "share/locale")
  SET(BIN_DESTINATION "bin")
ELSE(UNIX AND NOT WIN32)
  SET(INSTALL_DATADIR data)
  SET(INSTALL_LOCALEDIR locale)
  SET(DATA_DESTINATION "data")
  SET(PO_DESTINATION "locale")
  SET(BIN_DESTINATION ".")
ENDIF(UNIX AND NOT WIN32)

########### Add macros ###########
ADD_DEFINITIONS(-DHAVE_CONFIG_H)
ADD_DEFINITIONS(-DINSTALL_DATADIR=\\"${INSTALL_DATADIR}\\")
ADD_DEFINITIONS(-DINSTALL_LOCALEDIR=\\"${INSTALL_LOCALEDIR}\\")
ADD_DEFINITIONS(-DPACKAGE_VERSION=\\"${VERSION}\\")

ADD_SUBDIRECTORY( intl ) 
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

########### install files ###############

########### Generate doxygen doc ###############
ADD_CUSTOM_TARGET(doc doxygen Doxyfile WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMENT "Generate the doxygen documentation into the builds/doc directory.")

