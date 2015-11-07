
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

########### Project name ###########
PROJECT(Stellarium)
SET(STELLARIUM_MAJOR "0")
SET(STELLARIUM_MINOR "14")
SET(STELLARIUM_PATCH "1")
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)
SET(COPYRIGHT_YEARS "2000-2015")

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")
ADD_DEFINITIONS(-DCOPYRIGHT_YEARS="${COPYRIGHT_YEARS}")

SET(MINIMAL_QT_VERSION "5.3.0")

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

########### Main global variables ###########
IF(NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug GProf Valgrind Release RelWithDebInfo MinSizeRel." FORCE)
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

IF(WIN32)
     IF(NOT MSVC)
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fexceptions -fident -mthreads -Wno-unused-parameter -Wno-unused-result")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fexceptions -fident -mthreads -Wno-unused-parameter -Wno-unused-result")
          SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fexceptions -fident -mthreads -mwindows")
     ELSE()
          SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
          SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
          # _USE_MATH_DEFINES Prevent MSVC from complaining about M_PI no matter where <cmath> is included first
          # NOMINMAX So that windows.h does not redefine min and max
          # _WINSOCKAPI_ Prevent window.s from including winsock.h
          ADD_DEFINITIONS(-D_USE_MATH_DEFINES -DNOMINMAX -D_WINSOCKAPI_ /D_CRT_SECURE_NO_WARNINGS /wd4244 /wd4005 /wd4101 /wd4996 /wd4305 /wd4805)
     ENDIF()
ELSE()
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-unused-parameter -Wno-unused-result")
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

SET(STELLARIUM_RELEASE_BUILD 1 CACHE BOOL "Flag this build as an official release.")
IF(STELLARIUM_RELEASE_BUILD)
     ADD_DEFINITIONS(-DSTELLARIUM_RELEASE_BUILD)
ENDIF()

# Activate sound support
SET(ENABLE_SOUND 0 CACHE BOOL "Define whether sound support should be activated.")

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

########### Plugin setup #############

SET(STELLARIUM_PLUGINS) # Global list of all the plugins.
MACRO(ADD_PLUGIN NAME DEFAULT)
     STRING(TOUPPER ${NAME} NAME_UP)
     SET(USE_PLUGIN_${NAME_UP} ${DEFAULT} CACHE BOOL "Define wheter the ${NAME} plugin should be created.")
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
ADD_PLUGIN(LogBook 0)
ADD_PLUGIN(MeteorShowers 1)
ADD_PLUGIN(NavStars 1)
ADD_PLUGIN(Novae 1)
ADD_PLUGIN(Observability 1)
ADD_PLUGIN(Oculars 1)
ADD_PLUGIN(PointerCoordinates 1)
ADD_PLUGIN(Pulsars 1)
ADD_PLUGIN(Quasars 1)
ADD_PLUGIN(Satellites 1)
ADD_PLUGIN(Scenery3d 1)
ADD_PLUGIN(SolarSystemEditor 1)
ADD_PLUGIN(Supernovae 1)
ADD_PLUGIN(TextUserInterface 1)
ADD_PLUGIN(TelescopeControl 1)
ADD_PLUGIN(TimeZoneConfiguration 1)

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
IF(${Qt5Core_VERSION_STRING} VERSION_LESS MINIMAL_QT_VERSION)
     MESSAGE(FATAL_ERROR "Found Qt5: ${QMAKE_LOCATION} (found unsuitable version \"${Qt5Core_VERSION_STRING}\", required is \"${MINIMAL_QT_VERSION}\")")
ELSE()
     MESSAGE(STATUS "Found Qt5: ${QMAKE_LOCATION} (found suitable version \"${Qt5Core_VERSION_STRING}\")")
ENDIF()
FIND_PACKAGE(Qt5Concurrent REQUIRED)
FIND_PACKAGE(Qt5Gui REQUIRED)
FIND_PACKAGE(Qt5Network REQUIRED)
FIND_PACKAGE(Qt5OpenGL REQUIRED)
FIND_PACKAGE(Qt5Widgets REQUIRED)
IF(WIN32)
     FIND_PACKAGE(Qt5Sql REQUIRED)
     FIND_PACKAGE(Qt5XmlPatterns REQUIRED)
ENDIF()


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(ENABLE_SCRIPTING)
    SET(QT_USE_QTSCRIPT TRUE)
    FIND_PACKAGE(Qt5Script REQUIRED)
    INCLUDE_DIRECTORIES(${Qt5Script_INCLUDE_DIRS})
ENDIF()
IF(ENABLE_SOUND)
    ADD_DEFINITIONS(-DENABLE_SOUND)
    FIND_PACKAGE(Qt5Multimedia REQUIRED)
    FIND_PACKAGE(Qt5MultimediaWidgets REQUIRED)
    INCLUDE_DIRECTORIES(${Qt5Multimedia_INCLUDE_DIRS})
ENDIF()

# I add this test because on Windows with angle we should not link
# with OpenGL.  Maybe there is a better way to do that.
IF("${Qt5Gui_OPENGL_LIBRARIES}" MATCHES "opengl")
    FIND_PACKAGE(OpenGL REQUIRED)
ENDIF()
IF(NOT WIN32)
    FIND_PACKAGE(ZLIB)
ELSE()
    SET(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/core/external/zlib)
ENDIF()

FIND_PACKAGE(Qt5Test)
INCLUDE_DIRECTORIES(${Qt5Test_INCLUDE_DIRS})

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

########### Get revision number for non-release builds ###########
IF(NOT STELLARIUM_RELEASE_BUILD)
    #Hack until CMake detection for Bazaar is finished
    IF(BZR_REVISION)
        SET(PACKAGE_VERSION "bzr${BZR_REVISION}")
        ADD_DEFINITIONS(-DBZR_REVISION="${BZR_REVISION}")
    ENDIF()
    IF(STELLARIUM_VERSION)
        SET(PACKAGE_VERSION "${STELLARIUM_VERSION}")
        ADD_DEFINITIONS(-DSTELLARIUM_VERSION="${STELLARIUM_VERSION}")
    ENDIF()
ENDIF()


IF(WIN32)
    #######################################################
    ### Generate a manifest file                        ###
    #######################################################

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium.exe.manifest.cmake ${CMAKE_SOURCE_DIR}/stellarium.exe.manifest @ONLY)

    #######################################################
    ### Generate a VersionInfo file                     ###
    #######################################################

    IF(STELLARIUM_RELEASE_BUILD)
       SET(PACKAGE_VERSION_RC "${PACKAGE_VERSION},0")
    ELSE()
       SET(PACKAGE_VERSION_RC "${PACKAGE_VERSION}")
    ENDIF()
    STRING(REGEX REPLACE "([.]+)" "," PACKAGE_VERSION_RC ${PACKAGE_VERSION_RC})
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/stellarium.rc.cmake ${CMAKE_SOURCE_DIR}/data/stellarium.rc @ONLY)

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

    IF(GENERATE_STELMAINLIB)
        SET(STELMAINLIB "Source: \"${CMAKE_INSTALL_PREFIX}/lib/libstelMain.dll\"; DestDir: \"{app}\";")
        SET(MESALIB "; MESA libraries isn't used")
    ELSE()
        SET(STELMAINLIB "; StelMainLib isn't used")
        SET(MESALIB "Source: \"${CMAKE_SOURCE_DIR}/util/MESA/opengl32sw.dll\"; DestDir: \"{app}\";")
    ENDIF()
    GET_TARGET_PROPERTY(QtConcurrent_location Qt5::Concurrent LOCATION)
    GET_TARGET_PROPERTY(QtCore_location Qt5::Core LOCATION)
    GET_TARGET_PROPERTY(QtGui_location Qt5::Gui LOCATION)
    GET_TARGET_PROPERTY(QtOpenGL_location Qt5::OpenGL LOCATION)
    GET_TARGET_PROPERTY(QtNetwork_location Qt5::Network LOCATION)
    GET_TARGET_PROPERTY(QtWidgets_location Qt5::Widgets LOCATION)
    GET_TARGET_PROPERTY(QtSql_location Qt5::Sql LOCATION)    
    GET_TARGET_PROPERTY(QtXmlPatterns_location Qt5::XmlPatterns LOCATION)
    IF(ENABLE_SCRIPTING)
        GET_TARGET_PROPERTY(QtScript_location Qt5::Script LOCATION)
        SET(ISS_QT_SCRIPT "Source: \"${QtScript_location}\"; DestDir: \"{app}\";")
    ELSE()
        SET(ISS_QT_SCRIPT "; QtScript isn't used")
    ENDIF()
    IF(ENABLE_SOUND)
        GET_TARGET_PROPERTY(QtMultimedia_location Qt5::Multimedia LOCATION)
        GET_TARGET_PROPERTY(QtMultimediaWidgets_location Qt5::MultimediaWidgets LOCATION)
        SET(ISS_QT_MULTIMEDIA "Source: \"${QtMultimedia_location}\"; DestDir: \"{app}\";\nSource: \"${QtMultimediaWidgets_location}\"; DestDir: \"{app}\";")
    ELSE()
        SET(ISS_QT_MULTIMEDIA "; QtMultimedia isn't used")
    ENDIF()
    GET_FILENAME_COMPONENT(QT5_LIBS ${QtCore_location} PATH)
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
    SET(ISS_ICU_LIBS "; ICU support\nSource: \"${QT5_LIBS}/icu*.dll\"; DestDir: \"{app}\";")
    # Deploy related stuff
    SET(ISS_WINDOWS_PLUGIN "Source: \"${QT5_LIBS}/../plugins/platforms/qwindows.dll\"; DestDir: \"{app}/platforms/\";")
    SET(ISS_ICO_PLUGIN "Source: \"${QT5_LIBS}/../plugins/imageformats/qico.dll\"; DestDir: \"{app}/imageformats/\";")
    SET(ISS_JPEG_PLUGIN "Source: \"${QT5_LIBS}/../plugins/imageformats/qjpeg.dll\"; DestDir: \"{app}/imageformats/\";")
    IF(ENABLE_SOUND)
        SET(ISS_MULTIMEDIA_PLUGINS "Source: \"${QT5_LIBS}/../plugins/mediaservice/dsengine.dll\"; DestDir: \"{app}/mediaservice/\";\nSource: \"${QT5_LIBS}/../plugins/mediaservice/qtmedia_audioengine.dll\"; DestDir: \"{app}/mediaservice/\";\nSource: \"${QT5_LIBS}/../plugins/playlistformats/qtmultimedia_m3u.dll\"; DestDir: \"{app}/playlistformats/\";")
    ELSE()
        SET(ISS_MULTIMEDIA_PLUGINS "; QtMultimedia isn't used")
    ENDIF()
    SET(ISS_QML_DIR "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/qmldir\"; DestDir: \"{app}/Qt/labs/shaders/\";")
    SET(ISS_QML_PLUGINS "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/plugins.qmltypes\"; DestDir: \"{app}/Qt/labs/shaders/\";")
    SET(ISS_QML_SHADERS "Source: \"${QT5_LIBS}/../imports/Qt/labs/shaders/qmlshadersplugin.dll\"; DestDir: \"{app}/Qt/labs/shaders/\";")

    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium.iss.cmake ${CMAKE_SOURCE_DIR}/stellarium.iss @ONLY)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/stellarium-patch.iss.cmake ${CMAKE_SOURCE_DIR}/stellarium-patch.iss @ONLY)
ENDIF()

IF(APPLE)
    CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/Info.plist.cmake ${CMAKE_SOURCE_DIR}/data/Info.plist @ONLY)
ENDIF()

IF(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    ADD_DEFINITIONS(-DQT_NO_DEBUG)
    ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/data/default_config.ini.cmake ${CMAKE_SOURCE_DIR}/data/default_config.ini @ONLY)

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
    SET(CPACK_RPM_PACKAGE_LICENSE "GPLv2+")
    SET(CPACK_RPM_PACKAGE_GROUP "Amusements/Graphics")
    SET(CPACK_RPM_PACKAGE_URL "http://stellarium.org/")
    SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <stellarium-pubdevel@lists.sourceforge.net>")
    SET(CPACK_DEBIAN_PACKAGE_SECTION "science")
    SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}+deb1")
    SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_RPM_PACKAGE_URL}")
    SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
    INCLUDE(CPack)
ENDIF()

########### Macosx Bundling ###############
IF(APPLE)
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


########### Generate doxygen doc ###############
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
ADD_CUSTOM_TARGET(apidoc doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate the doxygen documentation into the doc directory.")
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/Doxyfile-plugins.cmake ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile-plugins @ONLY)
ADD_CUSTOM_TARGET(apidoc-plugins doxygen ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile-plugins WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generate the doxygen documentation into the doc-plugins directory.")

##################### Generate translation copying script ######################
IF(WIN32)
    STRING(REPLACE "/" "\\" PROJECT_SOURCE_DIR_WINPATH ${PROJECT_SOURCE_DIR})
    STRING(REPLACE "/" "\\" PROJECT_BINARY_DIR_WINPATH ${PROJECT_BINARY_DIR})
    CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/util/copy-translations.bat.cmake ${PROJECT_SOURCE_DIR}/util/copy-translations.bat @ONLY NEWLINE_STYLE WIN32)
ENDIF() 


########### Top level include directories ###########
# This will be used for all compilations in sub-directories
INCLUDE_DIRECTORIES(
     ${CMAKE_BINARY_DIR}
     ${CMAKE_SOURCE_DIR}/src
     ${CMAKE_SOURCE_DIR}/src/core
     ${CMAKE_SOURCE_DIR}/src/core/modules
     ${CMAKE_SOURCE_DIR}/src/core/planetsephems
     ${CMAKE_SOURCE_DIR}/src/core/external
     ${CMAKE_SOURCE_DIR}/src/core/external/qtcompress
     ${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/
     ${CMAKE_SOURCE_DIR}/src/core/external/glues_stel/source/libtess
     ${CMAKE_SOURCE_DIR}/src/gui
     ${CMAKE_SOURCE_DIR}/src/scripting
     ${INTL_INCLUDE_DIR}
     ${CMAKE_BINARY_DIR}/src
     ${ZLIB_INCLUDE_DIR}
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
IF(USE_PLUGIN_SCENERY3D)
    # add scenery3d scenes
    ADD_SUBDIRECTORY( scenery3d )
ENDIF()
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
