
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)

IF(NOT ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0))
     CMAKE_POLICY(SET CMP0050 OLD)
     # silently ignore errors in add_dependency():
     CMAKE_POLICY(SET CMP0046 OLD)
ENDIF()

# just avoid a warning mess. 
IF(NOT ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" LESS 3.0.2))
     CMAKE_POLICY(SET CMP0043 OLD)
ENDIF()

IF(NOT ("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}.${CMAKE_PATCH_VERSION}" LESS 3.1.3))
     CMAKE_POLICY(SET CMP0054 OLD)
ENDIF()

# warn about deprecated stuff so that we may try fixing it
SET(CMAKE_WARN_DEPRECATED 1)

########### Project name ###########
PROJECT(Stellarium)
# Use integer versions instead of strings for easier handling if required
SET(STELLARIUM_MAJOR 0)
SET(STELLARIUM_MINOR 90)
SET(STELLARIUM_PATCH 0)
ADD_DEFINITIONS(
-DSTELLARIUM_MAJOR=${STELLARIUM_MAJOR}
-DSTELLARIUM_MINOR=${STELLARIUM_MINOR}
-DSTELLARIUM_PATCH=${STELLARIUM_PATCH}
)
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)
SET(COPYRIGHT_YEARS "2000-2017")

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")
ADD_DEFINITIONS(-DCOPYRIGHT_YEARS="${COPYRIGHT_YEARS}")
# Write version of Stellarium to Stellarium User Guide
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/version.tex.cmake ${PROJECT_SOURCE_DIR}/guide/version.tex @ONLY NEWLINE_STYLE LF)

# currently Qt 5.4 - 5.6 is recommended, we can no longer build with Qt 5.3, and 5.7 may have some bugs (on Mac? 5.7 is fine on Windows) 5.8 on Windows seems OK.
SET(MINIMAL_QT_VERSION "5.4.0")

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Show platform info
MESSAGE(STATUS "Platform: ${CMAKE_SYSTEM}")

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
     SET(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: None Debug GProf Valgrind Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

SET(STELLARIUM_RELEASE_BUILD 0 CACHE BOOL "Set 1 to build as an official release (0 for development snapshots).")
IF(STELLARIUM_RELEASE_BUILD)
     ADD_DEFINITIONS(-DSTELLARIUM_RELEASE_BUILD)
ENDIF()

# GZ I had seen that some releases (e.g. 0.13.3 Ubuntu/ARM) were not compiled with NDEBUG defined. Maybe a bug in compilatation setup? 
# Hint from http://public.kitware.com/Bug/view.php?id=14292. Actually fixed with CMake 2.8.12, but maybe we have other effects?
IF(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
     ADD_DEFINITIONS(-DQT_NO_DEBUG -DNDEBUG)
ENDIF()

SET(OPENGL_DEBUG_LOGGING 0 CACHE BOOL "Enable to log OpenGL information using the GL_KHR_debug extension/QOpenGLLogger")
IF(OPENGL_DEBUG_LOGGING)
     ADD_DEFINITIONS(-DOPENGL_DEBUG_LOGGING)
     # This enables logging of QOpenGLFunctions through forced glGetError after each call
     ADD_DEFINITIONS(-DQ_ENABLE_OPENGL_FUNCTIONS_DEBUG)
ENDIF()

# Use ccache if possible
FIND_PROGRAM(CCACHE_PROGRAM ccache)
IF(CCACHE_PROGRAM)
     MESSAGE(STATUS "Found ccache ${CCACHE_PROGRAM}")
     SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
ENDIF()

# Add gprof build options if necessary.  Note gmon.out will be created in working directory when Stellarium is executed
IF(${CMAKE_BUILD_TYPE} MATCHES "GProf")
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fprofile-arcs")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs")
     SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
ENDIF()

# Add valgrind build options if necessary
IF(${CMAKE_BUILD_TYPE} MATCHES "Valgrind")
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     SET(APPLE 1)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
ENDIF()

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_GNUCC)
     IF(WIN32)
        # The stars structs rely on gnu gcc packing of bit-fields.
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mno-ms-bitfields")
     ENDIF()
     SET(GCC_VERSION "${CMAKE_CXX_COMPILER_VERSION}")
ELSE()
     SET(GCC_VERSION "0.0")
ENDIF()

# _USE_MATH_DEFINES enables use of math constants like M_PI,
# which are by default disabled in standard C++ mode (like std=c++11 instead of std=gnu11)
ADD_DEFINITIONS(-D_USE_MATH_DEFINES)

# flags shared for gcc-like compilers (also MinGW/Clang)
# Qt 5.7 requires C++11 support
SET(STEL_GCC_C_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unused-result")
SET(STEL_GCC_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unused-result")

SET(STELLARIUM_USE_CPP11 1 CACHE BOOL "Set whether to try using C++11 when compiling. Required for Qt >= 5.7")
IF(STELLARIUM_USE_CPP11)
     IF(NOT (CMAKE_VERSION VERSION_LESS 3.1.0))
        # Let CMake handle setting C++11 (since 3.1)
        SET(CMAKE_CXX_STANDARD 11)
        SET(CMAKE_C_STANDARD 11)
     ELSE()
        # Add std=c++11 to gcc flags
        SET(STEL_GCC_CXX_FLAGS "-std=c++11 ${STEL_GCC_CXX_FLAGS}")
     ENDIF()

ENDIF()

IF(WIN32)
     # We don't need the extra Windows.h stuff, this may speed up compilation a tiny bit
     # This should also prevent some Winsock.h warnings
     ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
     IF(NOT MSVC)
          # MinGW requires enabling of exceptions, version number storage and MinGW-specific threading
          SET(STEL_MINGW_FLAGS "-fexceptions -fident -mthreads")
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STEL_GCC_C_FLAGS} ${STEL_MINGW_FLAGS}")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STEL_GCC_CXX_FLAGS} ${STEL_MINGW_FLAGS}")
     ELSE() # MSVC
          # Additional flags:
          # Disabled warnings
          # C4244: implicit type conversion to a smaller type
          # C4305: type truncation
          # C4351: "new" behaviour, member array default initialization. Required since at least C++98, but funny MSVC throws a warning.
          # C4996: deprecated POSIX names (used in zlib)
          SET(STEL_MSVC_FLAGS "/wd4244 /wd4305 /wd4351 /wd4996")
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STEL_MSVC_FLAGS}")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STEL_MSVC_FLAGS}")
          # Additional defines:
          # NOMINMAX So that windows.h does not redefine min and max
          # _CRT_SECURE_NO_WARNINGS Removes warnings about using "insecure" C standard functions like scanf instead of MSVC-specific ones
          ADD_DEFINITIONS(-DNOMINMAX -D_CRT_SECURE_NO_WARNINGS)
     ENDIF()
ELSE()
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STEL_GCC_C_FLAGS}")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STEL_GCC_CXX_FLAGS}")
ENDIF()

# CMake 3.0.0 the CMAKE_<LANG>_COMPILER_ID value for Apple-provided Clang is now AppleClang
IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
     # using regular Clang or AppleClang
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual -Wno-unused-private-field -Wno-uninitialized -Wno-tautological-constant-out-of-range-compare")
     # The cosmetic fix for Clang 3.4
     IF(${CMAKE_CXX_COMPILER_VERSION} VERSION_GREATER 3.3)
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable -Wno-unused-result")
     ENDIF()
ENDIF()

# FreeBSD-specific compiler flags
# resolve bug for FreeBSD/amd64 and NVIDIA proprietary drivers 
IF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
     # Use -pthread compilation option to properly link to threading library
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
     IF(${GCC_VERSION} VERSION_GREATER 4.7)
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-local-typedefs")
     ENDIF()
ENDIF()

# best way to get to APPLE?
IF(APPLE)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -Wall -Wextra")
     # using regular Clang or AppleClang
     IF("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-private-field -Wno-unknown-warning-option -Wno-unused-const-variable -Wno-unused-result")
     ENDIF()
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -Wall -Wextra -Wno-unused-parameter")

     # We support only 64 bits MacOSX
     SET(CMAKE_OSX_ARCHITECTURES "x86_64")
ENDIF()

# This activates Link Time Optimization on recent GCC
# Unfortunately, it also seems to break static plugins
# It's providing a 20% size reduction, and probably a faster code when activated
#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden -ffunction-sections -flto")
#SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -ffunction-sections -flto")
#SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -flto -ffunction-sections -fuse-linker-plugin")

########### Others ###########
# Activate translation
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported.")
IF(ENABLE_NLS)
     ADD_DEFINITIONS(-DENABLE_NLS)
ENDIF()

# Activate media support
SET(ENABLE_MEDIA 1 CACHE BOOL "Define whether media support should be activated.")

# Activate GPS support. Solution depends on operating system:
# - Windows: Only directly-attached serial NMEA emitter.
# - Other systems: libgps if possible and NMEA emitter as fallback.
SET(ENABLE_GPS 1 CACHE BOOL "Define whether GPS queries should be supported.")

# SPOUT allows relaying the Stellarium OpenGL framebuffer as DirectX texture in other programs.
# It exists on Windows only. Syphon is a similar system for MacOS, this might be added by a Mac developer.
IF(WIN32)
     IF(${CMAKE_SYSTEM_VERSION} LESS 6.1 ) # Need Windows 7 to enable support of Spout
          SET(ENABLE_SPOUT 0)
          MESSAGE(STATUS "Spout support disabled due to operating system requirement")
     ELSE()
          SET(ENABLE_SPOUT 1 CACHE BOOL "Define whether SPOUT support should be activated.")
     ENDIF()
ELSE()
     SET(ENABLE_SPOUT 0)
ENDIF()
IF(ENABLE_SPOUT)
     ADD_DEFINITIONS(-DENABLE_SPOUT)
     IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4") # 32bit build?
          SET(SPOUT_PATH_HINT ${CMAKE_SOURCE_DIR}/util/spout2/Win32/)
     ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8") # 64bit build?
          SET(SPOUT_PATH_HINT ${CMAKE_SOURCE_DIR}/util/spout2/x64/)
     ENDIF()

     # make sure the .lib file is used, otherwise MinGW will try the .dll directly and fails to link
     FIND_LIBRARY(SPOUT_LIBRARY NAMES SpoutLibrary.lib SpoutLibrary PATHS ${SPOUT_PATH_HINT})
     MARK_AS_ADVANCED(SPOUT_LIBRARY)

     IF(NOT SPOUT_LIBRARY)
          MESSAGE(FATAL_ERROR "Spout library not found. Try setting SPOUT_LIBRARY or disable Spout support.")
     ENDIF()

     # try to find the .dll for automatic copy
     GET_FILENAME_COMPONENT(SPOUT_LIBRARY_DIR ${SPOUT_LIBRARY} DIRECTORY)
     SET(SPOUT_LIBRARY_DLL ${SPOUT_LIBRARY_DIR}/SpoutLibrary.dll CACHE FILEPATH "The path to the Spout .dll file")
     SET(SPOUT_LIBRARY_LICENCE ${CMAKE_SOURCE_DIR}/util/spout2/licence.txt CACHE FILEPATH "The path to the Spout licence file")

     IF(NOT EXISTS ${SPOUT_LIBRARY_DLL})
          UNSET(SPOUT_LIBRARY_DLL)
          # try to find it with find_library (note that this does not work with MSVC, but with MinGW it does, so the above manual version is necessary)
          FIND_LIBRARY(SPOUT_LIBRARY_DLL SpoutLibrary.dll PATHS ${SPOUT_PATH_HINT})
     ENDIF()

     IF(NOT SPOUT_LIBRARY_DLL)
          MESSAGE(WARNING "Could not find SpoutLibrary.dll, you may have to manually copy it to the binary directory.\
                           Try setting SPOUT_LIBRARY_DLL. Note that this is necessary for correct installer generation.")
     ENDIF()
     MARK_AS_ADVANCED(SPOUT_LIBRARY_DLL)
ENDIF(ENABLE_SPOUT)

SET(ENABLE_SCRIPTING 1 CACHE BOOL "Define whether scripting features should be activated.")
IF(ENABLE_SCRIPTING)
     # (De-)Activate the script edit console
     SET(ENABLE_SCRIPT_CONSOLE 1 CACHE BOOL "Define whether to build the script console feature.")
     IF(ENABLE_SCRIPT_CONSOLE)
          ADD_DEFINITIONS(-DENABLE_SCRIPT_CONSOLE)
     ENDIF()
ELSE()
     ADD_DEFINITIONS(-DDISABLE_SCRIPTING)
ENDIF()


SET(STELLARIUM_GUI_MODE Standard CACHE STRING "Choose the type of GUI to build, options are: Standard, None")

SET(GENERATE_PACKAGE_TARGET 1 CACHE BOOL "Set to 1 or true if you want to have make package target")

# On WIN32 we need to split the main binary into a small binary and a dll
# This is for allowing to link dynamic plug-ins afterward
# It does not seem to work with MSVC, so I disable it for the moment.
IF(WIN32 AND NOT MSVC)
     SET(GENERATE_STELMAINLIB 1)
ELSE()
     SET(GENERATE_STELMAINLIB 0)
ENDIF()

SET(ENABLE_TESTING 0 CACHE BOOL "Define whether the unit tests should be activated.")
IF(ENABLE_TESTING)
    ENABLE_TESTING()
    FIND_PACKAGE(Qt5Test REQUIRED)
ENDIF(ENABLE_TESTING)

########### User Guide ###############
IF(EXISTS "${CMAKE_SOURCE_DIR}/guide/guide.pdf")
     MESSAGE(STATUS "Found Stellarium User Guide")
     INSTALL(FILES guide/guide.pdf DESTINATION share/${PACKAGE}/guide)
     SET(ISS_GUIDE "Name: \"{group}\\{cm:UserGuide}\"; Filename: \"{app}\\guide\\guide.pdf\"")
ELSE()
     SET(ISS_GUIDE "; No link to Stellarium User Guide")
ENDIF()

########### Plugin setup #############

SET(STELLARIUM_PLUGINS) # Global list of all the plugins.
MACRO(ADD_PLUGIN NAME DEFAULT)
     STRING(TOUPPER ${NAME} NAME_UP)
     SET(USE_PLUGIN_${NAME_UP} ${DEFAULT} CACHE BOOL "Define whether the ${NAME} plugin should be created.")
     SET(STELLARIUM_PLUGINS ${STELLARIUM_PLUGINS} ${NAME})
ENDMACRO()

#### demo plugins ####
ADD_PLUGIN(HelloStelModule 0)
ADD_PLUGIN(SimpleDrawLine 0)
#### work plugins ####
ADD_PLUGIN(AngleMeasure 1)
ADD_PLUGIN(ArchaeoLines 1)
ADD_PLUGIN(CompassMarks 1)
ADD_PLUGIN(Exoplanets 1)
ADD_PLUGIN(EquationOfTime 1)
ADD_PLUGIN(FOV 1)
# TODO: Logbook build is broken for quite some time
ADD_PLUGIN(LogBook 0)
ADD_PLUGIN(MeteorShowers 1)
ADD_PLUGIN(NavStars 1)
ADD_PLUGIN(Novae 1)
ADD_PLUGIN(Observability 1)
ADD_PLUGIN(Oculars 1)
ADD_PLUGIN(PointerCoordinates 1)
ADD_PLUGIN(Pulsars 1)
ADD_PLUGIN(Quasars 1)
IF(ENABLE_SCRIPTING)
     ADD_PLUGIN(RemoteControl 1)
ENDIF()
ADD_PLUGIN(RemoteSync 1)
ADD_PLUGIN(Satellites 1)
ADD_PLUGIN(Scenery3d 1)
ADD_PLUGIN(SolarSystemEditor 1)
ADD_PLUGIN(Supernovae 1)
ADD_PLUGIN(TextUserInterface 1)
ADD_PLUGIN(TelescopeControl 1)

########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself

IF(USE_PLUGIN_LOGBOOK)
     SET(QT_USE_QTSQL TRUE) # For LogBook we also need SQL module
ENDIF()

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)

########### Find packages ###########
FIND_PACKAGE(Qt5Core REQUIRED)
GET_TARGET_PROPERTY(QMAKE_LOCATION Qt5::qmake LOCATION)
IF(${Qt5Core_VERSION} VERSION_LESS MINIMAL_QT_VERSION)
     MESSAGE(FATAL_ERROR "Found Qt5: ${QMAKE_LOCATION} (found unsuitable version \"${Qt5Core_VERSION}\", required is \"${MINIMAL_QT_VERSION}\")")
ELSE()
     MESSAGE(STATUS "Found Qt5: ${QMAKE_LOCATION} (found suitable version \"${Qt5Core_VERSION}\")")
ENDIF()
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
# required for qcustomplot external dependency
# not used elsewhere
FIND_PACKAGE(Qt5PrintSupport REQUIRED)
IF(USE_PLUGIN_TELESCOPECONTROL OR ENABLE_GPS)
     FIND_PACKAGE(Qt5SerialPort REQUIRED)
ENDIF()
IF(WIN32)
     FIND_PACKAGE(Qt5Sql REQUIRED)
     FIND_PACKAGE(Qt5XmlPatterns REQUIRED)

     # Try found OpenSSL DLL's on Windows
     FIND_FILE(OPENSSL_SSLEAY32_LIBRARY_DLL ssleay32.dll)
     FIND_FILE(OPENSSL_LIBEAY32_LIBRARY_DLL libeay32.dll)
     FIND_FILE(OPENSSL_LIBSSL32_LIBRARY_DLL libssl32.dll)
     FIND_FILE(OPENSSL_LICENSE "OpenSSL License.txt")
     SET(ISS_OPENSSL_LIBS "; OpenSSL support")
     IF(EXISTS ${OPENSSL_SSLEAY32_LIBRARY_DLL})
          MESSAGE(STATUS "Found ssleay32 library: ${OPENSSL_SSLEAY32_LIBRARY_DLL}")
          SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_SSLEAY32_LIBRARY_DLL}\"; DestDir: \"{app}\";")
     ENDIF()
     IF(EXISTS ${OPENSSL_LIBEAY32_LIBRARY_DLL})
          MESSAGE(STATUS "Found libeay32 library: ${OPENSSL_LIBEAY32_LIBRARY_DLL}")
          SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_LIBEAY32_LIBRARY_DLL}\"; DestDir: \"{app}\";")
     ENDIF()
     IF(EXISTS ${OPENSSL_LIBSSL32_LIBRARY_DLL})
          MESSAGE(STATUS "Found libssl32 library: ${OPENSSL_LIBSSL32_LIBRARY_DLL}")
          SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_LIBSSL32_LIBRARY_DLL}\"; DestDir: \"{app}\";")
     ENDIF()
     IF(EXISTS ${OPENSSL_LICENSE})
          MESSAGE(STATUS "Found OpenSSL license: ${OPENSSL_LICENSE}")
          SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_LICENSE}\"; DestDir: \"{app}\";")
     ENDIF()
     IF((NOT EXISTS ${OPENSSL_SSLEAY32_LIBRARY_DLL}) AND (NOT EXISTS ${OPENSSL_LIBEAY32_LIBRARY_DLL}))
          SET(ISS_OPENSSL_LIBS "; OpenSSL libraries isn't found")
     ENDIF()
ENDIF()

# when using Qt 5.3, default to using the old QGLWidget, but warn
IF(${Qt5Core_VERSION} VERSION_LESS "5.4")
     SET(USE_OLD_QGLWIDGET 1)
     MESSAGE(WARNING "You are using Qt ${Qt5Core_VERSION} to build, falling back to legacy QGLWidget rendering. Consider using at least Qt 5.4 if possible.")
ELSE()
     # offer the old widget as an alternative, default off
     SET(USE_OLD_QGLWIDGET 0 CACHE BOOL "Use the legacy QGLWidget instead of the new QOpenGLWidget for the main viewport, and link to the QtOpenGL module")
ENDIF()

IF(USE_OLD_QGLWIDGET)
     SET(USE_QTOPENGL 1)
     ADD_DEFINITIONS(-DUSE_OLD_QGLWIDGET)
ENDIF()

# Since Qt 5.4, linking to OpenGL is basically not required anymore,
# because the QtGui module re-implements the GL functions, and perhaps even
# dispatches the calls to a dynamically selected GL library.
#
# The only exception where this does not work with CMake is for
# ES2-only/ANGLE-only builds, which are seemingly not included in
# official Qt downloads, but may be required as a custom build
# for some embedded systems. Interestingly, this works with qmake,
# but CMake needs an explicit link definition.
# See also this bug: https://bugreports.qt.io/browse/QTBUG-29132

# Check if we have a GLES-only build
# On dynamic builds, this property is also "GL"
IF(${Qt5Gui_OPENGL_IMPLEMENTATION} MATCHES "GLES")
     MESSAGE(STATUS "Building an OpenGL ES build (${Qt5Gui_OPENGL_IMPLEMENTATION})")
     SET(STEL_GLES_LIBS Qt5::Gui_EGL Qt5::Gui_GLESv2)
ENDIF()


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(ENABLE_SCRIPTING)
     FIND_PACKAGE(Qt5Script REQUIRED)
ENDIF()

IF(ENABLE_MEDIA)
     SET(USE_QTOPENGL 1)
     ADD_DEFINITIONS(-DENABLE_MEDIA)
     FIND_PACKAGE(Qt5Multimedia REQUIRED)
     FIND_PACKAGE(Qt5MultimediaWidgets REQUIRED)
ENDIF()

IF(ENABLE_GPS)
     MESSAGE(STATUS "GPS: support by Qt's NMEA handling enabled.")
     FIND_PACKAGE(Qt5Positioning REQUIRED)
     ADD_DEFINITIONS(-DENABLE_GPS)
     IF(WIN32)
          SET(ENABLE_LIBGPS 0)
     ELSE(WIN32)
          FIND_PACKAGE(GPS QUIET)
          IF(GPS_FOUND)
               SET(ENABLE_LIBGPS 1)
               ADD_DEFINITIONS(-DENABLE_LIBGPS)
               MESSAGE(STATUS "GPS: Using GPS library at ${GPS_LIBRARY}")
          ELSE(GPS_FOUND)
               SET(ENABLE_LIBGPS 0)
               MESSAGE(STATUS "GPS: Could not find GPS library - GPSD query disabled!")
          ENDIF(GPS_FOUND)
     ENDIF(WIN32)
ELSE(ENABLE_GPS)
     SET(ENABLE_LIBGPS 0)
ENDIF(ENABLE_GPS)


IF(USE_QTOPENGL)
     # QtOpenGL module is deprecated. After merging the QOpenGLWidget branch, we don't use this module directly anymore,
     # but the QtMultimediaWidgets module requires it internally, so we still have to package it.
     FIND_PACKAGE(Qt5OpenGL REQUIRED)
ENDIF()

### Zlib package
FIND_PACKAGE(ZLIB QUIET) #quiet warnings about not found, we have our own version anyway

# if a system zlib is found, let the user choose if to instead use our bundled version
IF(ZLIB_FOUND)
     IF(WIN32) #on win, we default to "no"
          SET(USE_SYSTEM_ZLIB 0 CACHE BOOL "Use system-provided zlib instead of the bundled version")
     ELSE() #else, we default to "yes"
          SET(USE_SYSTEM_ZLIB 1 CACHE BOOL "Use system-provided zlib instead of the bundled version")
     ENDIF()
ELSE()
     SET(USE_SYSTEM_ZLIB 0)
ENDIF()

IF(NOT USE_SYSTEM_ZLIB)
     #use our own zlib
     MESSAGE(STATUS "Using bundled zlib version at ${CMAKE_SOURCE_DIR}/src/external/zlib")
     SET(ZLIB_LIBRARIES zlib_stel)
ELSE()
     MESSAGE(STATUS "Using system-provided zlib at ${ZLIB_LIBRARIES}")
ENDIF()

########### Set some global variables ###########
IF(UNIX AND NOT WIN32)
     IF(APPLE)
          SET(CMAKE_INSTALL_PREFIX "${PROJECT_BINARY_DIR}/Stellarium.app/Contents")
     ELSE()
          ADD_DEFINITIONS(-DINSTALL_DATADIR="${CMAKE_INSTALL_PREFIX}/share/stellarium")
          ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="${CMAKE_INSTALL_PREFIX}/share/locale")
          # Used in generating the documentation (doc/stellarium.pod.cmake):
          SET(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/share/stellarium")
     ENDIF()
ELSE()
     ADD_DEFINITIONS(-DINSTALL_DATADIR=".")
     ADD_DEFINITIONS(-DINSTALL_LOCALEDIR="./locale")
ENDIF()
# For not installed debug builds with build directory path that not match sources directory path ${PROJECT_BINARY_DIR} != ${CMAKE_SOURCE_DIR}
IF(CMAKE_BUILD_TYPE STREQUAL "Debug")
     ADD_DEFINITIONS(-DINSTALL_DATADIR_FOR_DEBUG="${CMAKE_SOURCE_DIR}")
     ADD_DEFINITIONS(-DINSTALL_LOCALEDIR_FOR_DEBUG="${CMAKE_SOURCE_DIR}")
ENDIF()

########### Get revision number for non-release builds ###########
IF(NOT STELLARIUM_RELEASE_BUILD)
     # Hack until CMake detection for Git is finished
     EXECUTE_PROCESS(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
     EXECUTE_PROCESS(COMMAND git rev-list --count ${GIT_BRANCH} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_COMMITS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
     IF(GIT_COMMITS)
          SET(PACKAGE_VERSION "${VERSION}.${GIT_COMMITS}")
          ADD_DEFINITIONS(-DGIT_COMMITS="${GIT_COMMITS}")
          ADD_DEFINITIONS(-DGIT_BRANCH="${GIT_BRANCH}")
     ENDIF()
     # MESSAGE(STATUS "Branch: ${GIT_BRANCH}; Commits: ${GIT_COMMITS}; Error: ${GIT_ERROR}")
     IF(STELLARIUM_VERSION)
          SET(PACKAGE_VERSION "${STELLARIUM_VERSION}")
          ADD_DEFINITIONS(-DSTELLARIUM_VERSION="${STELLARIUM_VERSION}")
     ENDIF()
ENDIF()


IF(WIN32)
     #######################################################
     ### Generate icon file name                         ###
     #######################################################

     IF(STELLARIUM_RELEASE_BUILD)
          SET(PACKAGE_ICON "stellarium")
     ELSE()
          SET(PACKAGE_ICON "stellarium-gray")
     ENDIF()

     #######################################################
     ### Generate a VersionInfo file                     ###
     #######################################################

     IF(STELLARIUM_RELEASE_BUILD)
           SET(PACKAGE_VERSION_RC "${PACKAGE_VERSION},0")
     ELSE()
          SET(PACKAGE_VERSION_RC "${PACKAGE_VERSION}")
     ENDIF()
     STRING(REGEX REPLACE "([.]+)" "," PACKAGE_VERSION_RC ${PACKAGE_VERSION_RC})
     CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/stellarium.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/stellarium.rc @ONLY)

     #######################################################
     ### [Optional] Embed icon in the Windows executable ###
     #######################################################

     SET(ENABLE_WINDOWS_EXE_ICON 1 CACHE BOOL "Determine if it should try to embed the Stellarium icon in the Windows .exe file")
     IF(ENABLE_WINDOWS_EXE_ICON AND NOT RC_COMPILER_PATH)
          IF(NOT MSVC)
               #The mingGW snapshot distributed with the Qt SDK has it under this name.
               SET(RC_COMPILER_FILENAME "windres.exe")
               FIND_FILE(RC_COMPILER_PATH ${RC_COMPILER_FILENAME})
          ENDIF()
          IF(RC_COMPILER_PATH)
               MESSAGE(STATUS "Found .rc compiler: ${RC_COMPILER_PATH}")
          ENDIF(RC_COMPILER_PATH)
     ENDIF(ENABLE_WINDOWS_EXE_ICON AND NOT RC_COMPILER_PATH)

     #######################################################
     ### Generate an Inno Setup project file             ###
     #######################################################

     # Try to guess the MinGW /bin directory...
     GET_FILENAME_COMPONENT(MINGW_BIN_DIRECTORY ${CMAKE_CXX_COMPILER} PATH)
     IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
          SET(ISS_ARCHITECTURE_SPECIFIC ";In the 64-bit only version, this line switches the installer to 64-bit mode.")
          SET(ISS_PACKAGE_PLATFORM "win32")
          # TODO: Add selector for VC2013/VC2015
          SET(REDIST_VERSION "VC_REDIST_X86")
          IF(MSVC)
               SET(REDIST_FILES "Source: \"${CMAKE_SOURCE_DIR}/util/vcredist/vcredist_x86.exe\"; DestDir: \"{tmp}\";")
               SET(REDIST_RUN "Filename: \"{tmp}/vcredist_x86.exe\"; Parameters: \"/passive /Q:a /c:\"\"msiexec /qb /i vcredist.msi\"\" \"; StatusMsg: \"{cm:RedistRun}\"; Check: VCRedistNeedsInstall")
          ELSE()
               SET(REDIST_FILES "Source: \"${MINGW_BIN_DIRECTORY}/libstdc++*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libgcc_s_dw2-1.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libwinpthread*.dll\"; DestDir: \"{app}\";")
               SET(REDIST_RUN "; Redistributable package not required for MinGW")
          ENDIF()
     ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
          SET(ISS_ARCHITECTURE_SPECIFIC ";Make the installer run only on win64:\nArchitecturesAllowed=x64\n;Switch to 64-bit install mode:\nArchitecturesInstallIn64BitMode=x64")
          SET(ISS_PACKAGE_PLATFORM "win64")
          # TODO: Add selector for VC2013/VC2015
          SET(REDIST_VERSION "VC_REDIST_X64")
          IF(MSVC)
               SET(REDIST_FILES "Source: \"${CMAKE_SOURCE_DIR}/util/vcredist/vcredist_x64.exe\"; DestDir: \"{tmp}\";")
               SET(REDIST_RUN "Filename: \"{tmp}/vcredist_x64.exe\"; Parameters: \"/passive /Q:a /c:\"\"msiexec /qb /i vcredist.msi\"\" \"; StatusMsg: \"{cm:RedistRun}\"; Check: VCRedistNeedsInstall")
          ELSE()
               SET(REDIST_FILES "Source: \"${MINGW_BIN_DIRECTORY}/libstdc++*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libwinpthread*.dll\"; DestDir: \"{app}\";\nSource: \"${MINGW_BIN_DIRECTORY}/libgcc_s_sjlj*.dll\"; DestDir: \"{app}\";")
               SET(REDIST_RUN "; Redistributable package not required for MinGW")
          ENDIF()
     ENDIF()
     SET(ISS_AUTOGENERATED_WARNING "Do not edit this file! It has been automatically generated by CMake. Your changes will be lost the next time CMake is run.")

     GET_TARGET_PROPERTY(QtConcurrent_location Qt5::Concurrent LOCATION)
     GET_TARGET_PROPERTY(QtCore_location Qt5::Core LOCATION)
     GET_TARGET_PROPERTY(QtGui_location Qt5::Gui LOCATION)
     IF(USE_QTOPENGL)
          GET_TARGET_PROPERTY(QtOpenGL_location Qt5::OpenGL LOCATION)
          SET(ISS_QT_OPENGL "Source: \"${QtOpenGL_location}\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_QT_OPENGL "; QtOpenGL isn't used")
     ENDIF()
     GET_TARGET_PROPERTY(QtNetwork_location Qt5::Network LOCATION)
     GET_TARGET_PROPERTY(QtWidgets_location Qt5::Widgets LOCATION)
     GET_TARGET_PROPERTY(QtSql_location Qt5::Sql LOCATION)     
     GET_TARGET_PROPERTY(QtXmlPatterns_location Qt5::XmlPatterns LOCATION)
     GET_TARGET_PROPERTY(QtPrintSupport_location Qt5::PrintSupport LOCATION)
     SET(ISS_QT_PRINTSUPPORT "Source: \"${QtPrintSupport_location}\"; DestDir: \"{app}\";")
     IF(ENABLE_SCRIPTING)
          GET_TARGET_PROPERTY(QtScript_location Qt5::Script LOCATION)
          SET(ISS_QT_SCRIPT "Source: \"${QtScript_location}\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_QT_SCRIPT "; QtScript isn't used")
     ENDIF()
     IF(USE_PLUGIN_TELESCOPECONTROL OR ENABLE_GPS)
          GET_TARGET_PROPERTY(QtSerialPort_location Qt5::SerialPort LOCATION)
          SET(ISS_QT_SERIALPORT "Source: \"${QtSerialPort_location}\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_QT_SERIALPORT "; QtSerialPort isn't used")
     ENDIF()
     IF(ENABLE_GPS)
          GET_TARGET_PROPERTY(QtPositioning_location Qt5::Positioning LOCATION)
          SET(ISS_QT_POSITIONING "Source: \"${QtPositioning_location}\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_QT_POSITIONING "; QtPositioning isn't used")
     ENDIF()
     IF(ENABLE_MEDIA)
          GET_TARGET_PROPERTY(QtMultimedia_location Qt5::Multimedia LOCATION)
          GET_TARGET_PROPERTY(QtMultimediaWidgets_location Qt5::MultimediaWidgets LOCATION)
          SET(ISS_QT_MULTIMEDIA "Source: \"${QtMultimedia_location}\"; DestDir: \"{app}\";\nSource: \"${QtMultimediaWidgets_location}\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_QT_MULTIMEDIA "; QtMultimedia isn't used")
     ENDIF()
     GET_FILENAME_COMPONENT(QT5_LIBS ${QtCore_location} PATH)
     IF(GENERATE_STELMAINLIB)
          SET(STELMAINLIB "Source: \"${CMAKE_INSTALL_PREFIX}/lib/libstelMain.dll\"; DestDir: \"{app}\";")
     ELSE()
          SET(STELMAINLIB "; StelMainLib isn't used")
     ENDIF()
     SET(MESALIB "Source: \"${QT5_LIBS}/opengl32sw.dll\"; DestDir: \"{app}\";")
     IF(MSVC)
          # Use installer for checking minimum requirements
          # Stellarium/MSVC require Windows Vista (6.0) for work
          # Details: http://www.jrsoftware.org/ishelp/index.php?topic=winvernotes
          SET(MIN_VERSION "6.0")
          SET(ISS_ANGLE_LIBS "; ANGLE support (libEGL.dll, libGLESv2.dll and d3dcompiler_*.dll)\nSource: \"${QT5_LIBS}/libEGL.dll\"; DestDir: \"{app}\";\nSource: \"${QT5_LIBS}/libGLESv2.dll\"; DestDir: \"{app}\";\nSource: \"${QT5_LIBS}/d3dcompiler_*.dll\"; DestDir: \"{app}\";")
     ELSE()
          # Use installer for checking minimum requirements
          # Stellarium/MinGW require Windows XP (5.1) for work
          # Details: http://www.jrsoftware.org/ishelp/index.php?topic=winvernotes
          SET(MIN_VERSION "5.1")
          SET(ISS_ANGLE_LIBS "; ANGLE support doesn't exists for MinGW")
     ENDIF()
     IF(${Qt5Core_VERSION} VERSION_LESS "5.9.0")
          SET(ISS_ICU_LIBS "; ICU support\nSource: \"${QT5_LIBS}/icu*.dll\"; DestDir: \"{app}\";")
     ELSE()
          SET(ISS_ICU_LIBS "; No need for separate ICU libs with Qt 5.9+")
     ENDIF()
     # Deploy related stuff
     SET(ISS_QT_PLUGINS "; Qt plugins")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/audio/qtaudio_windows.dll\"; DestDir: \"{app}/audio/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/bearer/qgenericbearer.dll\"; DestDir: \"{app}/bearer/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/bearer/qnativewifibearer.dll\"; DestDir: \"{app}/bearer/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/platforms/qwindows.dll\"; DestDir: \"{app}/platforms/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/imageformats/qico.dll\"; DestDir: \"{app}/imageformats/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/imageformats/qjpeg.dll\"; DestDir: \"{app}/imageformats/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/imageformats/qgif.dll\"; DestDir: \"{app}/imageformats/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/imageformats/qwbmp.dll\"; DestDir: \"{app}/imageformats/\";")
     SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/imageformats/qwebp.dll\"; DestDir: \"{app}/imageformats/\";")
     IF(ENABLE_MEDIA)
          SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${QT5_LIBS}/../plugins/mediaservice/dsengine.dll\"; DestDir: \"{app}/mediaservice/\";\nSource: \"${QT5_LIBS}/../plugins/mediaservice/qtmedia_audioengine.dll\"; DestDir: \"{app}/mediaservice/\";\nSource: \"${QT5_LIBS}/../plugins/playlistformats/qtmultimedia_m3u.dll\"; DestDir: \"{app}/playlistformats/\";")
     ENDIF()
     IF(ENABLE_SPOUT)
          IF(SPOUT_LIBRARY_DLL) # we know the dll
               MESSAGE(STATUS "Found Spout library: ${SPOUT_LIBRARY_DLL}")
               SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${SPOUT_LIBRARY_DLL}\"; DestDir: \"{app}\";")
               SET(ISS_QT_PLUGINS "${ISS_QT_PLUGINS}\nSource: \"${SPOUT_LIBRARY_LICENCE}\"; DestDir: \"{app}\"; DestName: \"Spout_licence.txt\";")
          ENDIF()
          SET(ISS_SPOUT "Name: \"{group}\\Stellarium {cm:SpoutMode}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
     ELSE()
          SET(ISS_SPOUT "; Spout support is disabled")
     ENDIF(ENABLE_SPOUT)

ENDIF(WIN32)


IF(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
     ADD_DEFINITIONS(-DQT_NO_DEBUG)
     ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/default_config.ini.cmake ${CMAKE_SOURCE_DIR}/data/default_config.ini @ONLY)

########### uninstall files ###############
CONFIGURE_FILE(
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
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
     SET(CPACK_PACKAGE_VERSION_MAJOR "${STELLARIUM_MAJOR}")
     SET(CPACK_PACKAGE_VERSION_MINOR "${STELLARIUM_MINOR}")
     SET(CPACK_PACKAGE_VERSION_PATCH "${STELLARIUM_PATCH}")
     SET(CPACK_PACKAGE_INSTALL_DIRECTORY "stellarium")
     SET(CPACK_SOURCE_PACKAGE_FILE_NAME "stellarium-${PACKAGE_VERSION}")
     SET(CPACK_SOURCE_GENERATOR "TGZ")
     SET(CPACK_GENERATOR "TGZ")
     SET(CPACK_STRIP_FILES "bin/stellarium")
     SET(CPACK_PACKAGE_EXECUTABLES "stellarium" "Stellarium")
     SET(CPACK_SOURCE_IGNORE_FILES "/.tx/" "/CVS/" "/.svn/" "/.git/" "/.bzr/" "builds/" "installers/" "util/spout2" "util/DSSToStellarium/toastForShape$" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "/CMakeLists.txt.user$" "\\\\.bzrignore$" "\\\\.yml$" "~$" "\\\\.swp$" "\\\\.#" "/#")
     SET(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
     SET(CPACK_RPM_PACKAGE_GROUP "Amusements/Graphics")
     SET(CPACK_RPM_PACKAGE_URL "http://stellarium.org/")
     SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <stellarium@googlegroups.com>")
     SET(CPACK_DEBIAN_PACKAGE_SECTION "science")
     SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}+deb1")
     SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_RPM_PACKAGE_URL}")
     SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
     INCLUDE(CPack)
ENDIF()

########### OS X Bundling ###############
IF(APPLE)
     CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/Info.plist.cmake ${CMAKE_SOURCE_DIR}/data/Info.plist @ONLY)

     # ${Qt5Core_INCLUDE_DIRS} is a list; I'm expecting the first entry to always be QtCore.framework.
     ADD_CUSTOM_TARGET(
          mac_app
          python util/mac_app.py ${CMAKE_INSTALL_PREFIX} ${PROJECT_SOURCE_DIR} ${CMAKE_BUILD_TYPE} ${Qt5Core_INCLUDE_DIRS} 
          DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/Stellarium.app
          WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
          COMMENT "making the macosx bundle."
          VERBATIM
     )
ENDIF()

########### Windows packaging ###############
IF(WIN32)
     CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/stellarium.iss.cmake ${CMAKE_CURRENT_BINARY_DIR}/stellarium.iss @ONLY)
     SET(ISS_COMPILER_FILENAME "ISCC.exe")
     FIND_FILE(ISS_COMPILER_PATH ${ISS_COMPILER_FILENAME})
     IF(ISS_COMPILER_PATH)
          MESSAGE(STATUS "Found Inno Setup (.iss) compiler: ${ISS_COMPILER_PATH}")
          # Add installer target
          ADD_CUSTOM_TARGET(
               stellarium-installer
               COMMAND ${ISS_COMPILER_PATH} "${CMAKE_CURRENT_BINARY_DIR}/stellarium.iss"
               COMMENT "Making an installer package for Windows..."
               VERBATIM
          )
     ELSE()
          MESSAGE(WARNING "Inno Setup Compiler not found. You won't be able to build setup files.")
     ENDIF()
ENDIF()

########### Generate doxygen doc ###############
FIND_PACKAGE(Doxygen)
IF(DOXYGEN_FOUND)
     IF(DOXYGEN_DOT_FOUND)
          SET(HAVE_DOT "YES")
          SET(DOT_PATH ${DOXYGEN_DOT_PATH})
     ELSE()
          SET(HAVE_DOT "NO")
          SET(DOT_PATH "")
     ENDIF()
     CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
     ADD_CUSTOM_TARGET(apidoc ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate the doxygen documentation into the doc directory.")
ENDIF()

##################### Generate translation copying script ######################
IF(WIN32)
     STRING(REPLACE "/" "\\" PROJECT_SOURCE_DIR_WINPATH ${PROJECT_SOURCE_DIR})
     STRING(REPLACE "/" "\\" PROJECT_BINARY_DIR_WINPATH ${PROJECT_BINARY_DIR})
     CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/copy-translations.bat.cmake ${PROJECT_SOURCE_DIR}/util/copy-translations.bat @ONLY NEWLINE_STYLE WIN32)
ENDIF() 


########### Top level include directories ###########
# This will be used for all compilations in sub-directories
INCLUDE_DIRECTORIES(
      ${CMAKE_BINARY_DIR}
      ${CMAKE_SOURCE_DIR}/src
      ${CMAKE_SOURCE_DIR}/src/core
      ${CMAKE_SOURCE_DIR}/src/core/modules
      ${CMAKE_SOURCE_DIR}/src/core/planetsephems
      ${CMAKE_SOURCE_DIR}/src/gui
      ${CMAKE_SOURCE_DIR}/src/scripting
      ${INTL_INCLUDE_DIR}
      ${CMAKE_BINARY_DIR}/src
)

########### Subdirectories ###############
ADD_SUBDIRECTORY( data )
ADD_SUBDIRECTORY( models )
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
IF(USE_PLUGIN_SCENERY3D)
     # add scenery3d scenes
     ADD_SUBDIRECTORY( scenery3d )
ENDIF()
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
