CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
MESSAGE(STATUS "Found CMake ${CMAKE_VERSION}")

IF(POLICY CMP0054)
# Potential dangerous comparison of variables. Details: https://cmake.org/cmake/help/v3.1/policy/CMP0054.html
     CMAKE_POLICY(SET CMP0054 NEW)
ENDIF()
IF(POLICY CMP0071)
     CMAKE_POLICY(SET CMP0071 NEW)
ENDIF()

# warn about deprecated stuff so that we may try fixing it
SET(CMAKE_WARN_DEPRECATED 1)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

########### Project name ###########
PROJECT(Stellarium)
# Use integer versions instead of strings for easier handling if required
SET(STELLARIUM_MAJOR 0)
SET(STELLARIUM_MINOR 20)
SET(STELLARIUM_PATCH 4)
ADD_DEFINITIONS(
     -DSTELLARIUM_MAJOR=${STELLARIUM_MAJOR}
     -DSTELLARIUM_MINOR=${STELLARIUM_MINOR}
     -DSTELLARIUM_PATCH=${STELLARIUM_PATCH}
)
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(PACKAGE stellarium)
STRING(TIMESTAMP CURRENTYEAR "%Y")
SET(COPYRIGHT_YEARS "2000-${CURRENTYEAR}")

SET(STELLARIUM_URL "https://stellarium.org/")
ADD_DEFINITIONS(-DSTELLARIUM_URL="${STELLARIUM_URL}")

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")
ADD_DEFINITIONS(-DCOPYRIGHT_YEARS="${COPYRIGHT_YEARS}")
ADD_DEFINITIONS(-DSTELLARIUM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")

# currently Qt 5.9/5.12 is recommended
SET(MINIMAL_QT_VERSION "5.7.0")
# TODO: Check real minimal supported version of GPSD API
SET(MINIMAL_GPS_API_VERSION "6.0")

### Define minimal version for Windows
# Use installer for checking minimum requirements
# Stellarium/MSVC require Windows 7 (6.1) for work
# Details: http://www.jrsoftware.org/ishelp/index.php?topic=winvernotes
SET(ISS_MIN_WIN_VERSION "6.1")
# Define required version for MSVC
# Details: https://docs.microsoft.com/ru-ru/windows/win32/winprog/using-the-windows-headers
SET(MIN_WIN_VERSION "0x0601")

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)

# Show platform info
MESSAGE(STATUS "Platform: ${CMAKE_SYSTEM} (${CMAKE_SYSTEM_PROCESSOR})")

########### Main global variables ###########
SET(ENABLE_TESTING 0 CACHE BOOL "Define whether the unit tests should be activated.")

IF(NOT CMAKE_BUILD_TYPE)
     SET(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: None Debug GProf Valgrind ASan UBSan Release RelWithDebInfo MinSizeRel." FORCE)
ENDIF()

IF(${CMAKE_BUILD_TYPE} MATCHES "Debug")
     IF(CMAKE_COMPILER_IS_GNUCXX)
          INCLUDE(CodeCoverage)
          IF(LCOV_PATH AND ENABLE_TESTING)
               APPEND_COVERAGE_COMPILER_FLAGS()
               MESSAGE(STATUS "Found lcov ${LCOV_PATH} (enable coverage support)")
               SETUP_TARGET_FOR_COVERAGE_LCOV(
                    NAME coverage 
                    EXECUTABLE ctest -j ${PROCESSOR_COUNT} 
                    DEPENDENCIES buildTests
               )
          ENDIF()
     ENDIF()
ENDIF()

SET(STELLARIUM_RELEASE_BUILD 0 CACHE BOOL "Set 1 to build as an official release (0 for development snapshots).")
IF(STELLARIUM_RELEASE_BUILD)
     ADD_DEFINITIONS(-DSTELLARIUM_RELEASE_BUILD)
     SET(SERIES "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}")
ELSE()
     SET(SERIES "head")
ENDIF()

########### Get revision number for non-release builds ###########
IF(NOT STELLARIUM_RELEASE_BUILD)
     # Hack until CMake detection for Git is finished
     IF(WIN32)
          FIND_FILE(GITCMD git.exe)
     ELSE()
          FIND_FILE(GITCMD git)
     ENDIF()
     STRING(TIMESTAMP CDNUMBER "%j")
     SET(WINDOWS_PACKAGE_VERSION "${VERSION}.${CDNUMBER}")
     IF(EXISTS ${GITCMD})
          EXECUTE_PROCESS(COMMAND git rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
          EXECUTE_PROCESS(COMMAND git log -1 --pretty=format:%h WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
          EXECUTE_PROCESS(COMMAND git rev-list --count ${GIT_BRANCH} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_COMMITS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
          IF(GIT_REVISION)
               # Let's use short hash in all cases
               STRING(SUBSTRING ${GIT_REVISION} 0 7 REVISION)
               SET(PACKAGE_VERSION "${VERSION}-${REVISION}")
               ADD_DEFINITIONS(-DGIT_REVISION="${REVISION}")
               ADD_DEFINITIONS(-DGIT_BRANCH="${GIT_BRANCH}")
               # better way to numbering
               SET(WINDOWS_PACKAGE_VERSION "${VERSION}.${GIT_COMMITS}")
          ENDIF()
     ELSE()
          STRING(TIMESTAMP TODAY "%j%H%M")
          SET(PACKAGE_VERSION "${VERSION}-${TODAY}")
          ADD_DEFINITIONS(-DGIT_REVISION="${TODAY}")
          ADD_DEFINITIONS(-DGIT_BRANCH="tarball")
     ENDIF()
     IF(STELLARIUM_VERSION)
          SET(PACKAGE_VERSION "${STELLARIUM_VERSION}")
          ADD_DEFINITIONS(-DSTELLARIUM_VERSION="${STELLARIUM_VERSION}")
     ENDIF()
ENDIF()
MESSAGE(STATUS "Building Stellarium ${PACKAGE_VERSION} (Mode: ${CMAKE_BUILD_TYPE})")

# Write version and series of Stellarium to Stellarium User Guide
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/version.tex.cmake ${PROJECT_SOURCE_DIR}/guide/version.tex @ONLY NEWLINE_STYLE LF)

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

# Add ASan (AddressSanitizer) build options if necessary
# Details: https://github.com/google/sanitizers/wiki/AddressSanitizer
IF(${CMAKE_BUILD_TYPE} MATCHES "ASan")
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
ENDIF()

# Add UBSan (UndefinedBehaviorSanitizer) build options if necessary
# Details: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
IF(${CMAKE_BUILD_TYPE} MATCHES "UBSan")
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -g")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -g")
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     SET(APPLE 1)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
     # share data location:
     SET(SDATALOC "Resources")
ELSE()
     # share data location:
     SET(SDATALOC "share/${PACKAGE}")
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
SET(STEL_GCC_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")
SET(STEL_GCC_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter")
# Intel C/C++ compilers do not have an option -Wno-unused-results
IF(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "Intel")
     SET(STEL_GCC_C_FLAGS "${STEL_GCC_C_FLAGS} -Wno-unused-result")
     SET(STEL_GCC_CXX_FLAGS "${STEL_GCC_CXX_FLAGS} -Wno-unused-result")
ELSE()
     # Suppress some warning for Intel C/C++ compilers
     # Diagnostic 1875: offsetof applied to non-POD (Plain Old Data) types is nonstandard
     # Diagnostic 654: overloaded virtual function
     SET(STEL_GCC_C_FLAGS "${STEL_GCC_C_FLAGS} -diag-disable:remark -wd1875,654")
     SET(STEL_GCC_CXX_FLAGS "${STEL_GCC_CXX_FLAGS} -diag-disable:remark -wd1875,654")
ENDIF()

# Set whether to try using C++11 when compiling. Required for Qt >= 5.7"
IF(NOT (CMAKE_VERSION VERSION_LESS 3.1.0))
     # Let CMake handle setting C++11 (since 3.1)
     SET(CMAKE_CXX_STANDARD 11)
     SET(CMAKE_C_STANDARD 11)
ELSE()
     # Add std=c++11 to gcc flags
     SET(STEL_GCC_CXX_FLAGS "-std=c++11 ${STEL_GCC_CXX_FLAGS}")
     # Add std=c11 to gcc flags
     SET(STEL_GCC_C_FLAGS "-std=c11 ${STEL_GCC_C_FLAGS}")
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
          # Set minimal version of Windows
          SET(STEL_MSVC_FLAGS "${STEL_MSVC_FLAGS} /D_WIN32_WINNT=${MIN_WIN_VERSION}")
          IF(MSVC_VERSION GREATER 1700)
               SET(STEL_MSVC_FLAGS "${STEL_MSVC_FLAGS} /MP")

               FOREACH(flag_var
                    CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
                    CMAKE_SHARED_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO)
                    SET(${flag_var} "${${flag_var}} /DEBUG:FASTLINK")
               ENDFOREACH()
          ENDIF()
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
     IF(${CMAKE_SYSTEM_VERSION} LESS 6.1 ) # Need Windows 7 or later to enable support of Spout
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
     ADD_DEFINITIONS(-DENABLE_SCRIPTING)
     # (De-)Activate the script edit console
     SET(ENABLE_SCRIPT_CONSOLE 1 CACHE BOOL "Define whether to build the script console feature.")
     IF(ENABLE_SCRIPT_CONSOLE)
          ADD_DEFINITIONS(-DENABLE_SCRIPT_CONSOLE)
     ENDIF()
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

IF(ENABLE_TESTING)
    ENABLE_TESTING()
    ADD_DEFINITIONS(-DENABLE_TESTING)
    FIND_PACKAGE(Qt5Test REQUIRED)
    MESSAGE(STATUS "Unit tests support: enabled")
ELSE(ENABLE_TESTING)
    MESSAGE(STATUS "Unit tests support: disabled")
ENDIF(ENABLE_TESTING)

########### User Guide ###############
IF(EXISTS "${CMAKE_SOURCE_DIR}/guide/guide.pdf")
     MESSAGE(STATUS "Found Stellarium User Guide")
     INSTALL(FILES guide/guide.pdf DESTINATION ${SDATALOC}/guide)
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
ADD_PLUGIN(Calendars 1)
ADD_PLUGIN(Exoplanets 1)
ADD_PLUGIN(EquationOfTime 1)
ADD_PLUGIN(MeteorShowers 1)
ADD_PLUGIN(NavStars 1)
ADD_PLUGIN(Novae 1)
ADD_PLUGIN(Observability 1)
ADD_PLUGIN(Oculars 1)
ADD_PLUGIN(Oculus 0)
ADD_PLUGIN(PointerCoordinates 1)
ADD_PLUGIN(Pulsars 1)
ADD_PLUGIN(Quasars 1)
IF(ENABLE_SCRIPTING)
     ADD_PLUGIN(RemoteControl 1)
ELSE()
     ADD_PLUGIN(RemoteControl 0)
ENDIF()
ADD_PLUGIN(RemoteSync 1)
ADD_PLUGIN(Satellites 1)
ADD_PLUGIN(Scenery3d 1)
ADD_PLUGIN(SolarSystemEditor 1)
ADD_PLUGIN(Supernovae 1)
ADD_PLUGIN(TextUserInterface 1)
ADD_PLUGIN(TelescopeControl 1)
ADD_PLUGIN(Vts 0)

########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)
SET_TARGET_PROPERTIES(AllStaticPlugins PROPERTIES FOLDER "plugins")

########### Find packages ###########
FIND_PACKAGE(Qt5Core REQUIRED)
GET_TARGET_PROPERTY(QMAKE_LOCATION Qt5::qmake LOCATION)
IF(${Qt5Core_VERSION} VERSION_LESS MINIMAL_QT_VERSION)
     MESSAGE(FATAL_ERROR "Found Qt5: ${QMAKE_LOCATION} (found unsuitable version ${Qt5Core_VERSION}, required is ${MINIMAL_QT_VERSION})")
ELSE()
     MESSAGE(STATUS "Found Qt5: ${QMAKE_LOCATION} (found suitable version ${Qt5Core_VERSION})")
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
     FIND_PACKAGE(Qt5Svg REQUIRED)
     FIND_PACKAGE(Qt5XmlPatterns REQUIRED)

     # Qt 5.9 (LTS) use OpenSSL 1.0* and Qt 5.12 (LTS) and newest version of Qt use OpenSSL 1.1*
     # Source: https://lists.qt-project.org/pipermail/releasing/2019-March/002614.html
     IF(${Qt5Core_VERSION} VERSION_LESS "5.10")
          # OK, it's Qt 5.9, let's try found OpenSSL 1.0.x DLL's on Windows
          FIND_FILE(OPENSSL_SSLEAY32_LIBRARY_DLL ssleay32.dll)
          FIND_FILE(OPENSSL_LIBEAY32_LIBRARY_DLL libeay32.dll)
          FIND_FILE(OPENSSL_LIBSSL32_LIBRARY_DLL libssl32.dll)
          SET(ISS_OPENSSL_LIBS "; OpenSSL support")
          IF(EXISTS ${OPENSSL_SSLEAY32_LIBRARY_DLL})
               MESSAGE(STATUS "Found ssleay32 library: ${OPENSSL_SSLEAY32_LIBRARY_DLL}")
               SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_SSLEAY32_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
          ENDIF()
          IF(EXISTS ${OPENSSL_LIBEAY32_LIBRARY_DLL})
               MESSAGE(STATUS "Found libeay32 library: ${OPENSSL_LIBEAY32_LIBRARY_DLL}")
               SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_LIBEAY32_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
          ENDIF()
          IF(EXISTS ${OPENSSL_LIBSSL32_LIBRARY_DLL})
               MESSAGE(STATUS "Found libssl32 library: ${OPENSSL_LIBSSL32_LIBRARY_DLL}")
               SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_LIBSSL32_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
          ENDIF()
          IF((NOT EXISTS ${OPENSSL_SSLEAY32_LIBRARY_DLL}) AND (NOT EXISTS ${OPENSSL_LIBEAY32_LIBRARY_DLL}))
               SET(ISS_OPENSSL_LIBS "; OpenSSL libraries not found")
          ENDIF()
     ELSE()
          # Try found OpenSSL 1.1.x DLL's on Windows (newest Qt)
          IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
               FIND_FILE(OPENSSL_SSL_LIBRARY_DLL libssl-1_1.dll)
               FIND_FILE(OPENSSL_CRYPTO_LIBRARY_DLL libcrypto-1_1.dll)
          ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
               FIND_FILE(OPENSSL_SSL_LIBRARY_DLL libssl-1_1-x64.dll)
               FIND_FILE(OPENSSL_CRYPTO_LIBRARY_DLL libcrypto-1_1-x64.dll)
          ENDIF()
          SET(ISS_OPENSSL_LIBS "; OpenSSL support")
          IF(EXISTS ${OPENSSL_SSL_LIBRARY_DLL})
               MESSAGE(STATUS "Found OpenSSL SSL library: ${OPENSSL_SSL_LIBRARY_DLL}")
               SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_SSL_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
          ENDIF()
          IF(EXISTS ${OPENSSL_CRYPTO_LIBRARY_DLL})
               MESSAGE(STATUS "Found OpenSSL crypto library: ${OPENSSL_CRYPTO_LIBRARY_DLL}")
               SET(ISS_OPENSSL_LIBS "${ISS_OPENSSL_LIBS}\nSource: \"${OPENSSL_CRYPTO_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
          ENDIF()
          IF((NOT EXISTS ${OPENSSL_SSL_LIBRARY_DLL}) AND (NOT EXISTS ${OPENSSL_CRYPTO_LIBRARY_DLL}))
               SET(ISS_OPENSSL_LIBS "; OpenSSL libraries not found")
          ENDIF()
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
ELSE()
     MESSAGE(STATUS "Building an OpenGL build")
ENDIF()


# Tell CMake to run moc when necessary:
SET(CMAKE_AUTOMOC ON)
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
               IF(${GPS_VERSION_STRING} VERSION_LESS MINIMAL_GPS_API_VERSION)
                    SET(ENABLE_LIBGPS 0)
                    MESSAGE(STATUS "GPS: found GPS library at ${GPS_LIBRARY} (found unsuitable GPSD API version ${GPS_VERSION_STRING}, required is ${MINIMAL_GPS_API_VERSION})")
                    MESSAGE(STATUS "GPS: GPSD query disabled!")
               ELSE()
                    SET(ENABLE_LIBGPS 1)
                    ADD_DEFINITIONS(-DENABLE_LIBGPS)
                    MESSAGE(STATUS "GPS: using GPS library at ${GPS_LIBRARY} (found suitable GPSD API version: ${GPS_VERSION_STRING})")
               ENDIF()
          ELSE(GPS_FOUND)
               SET(ENABLE_LIBGPS 0)
               MESSAGE(STATUS "GPS: could not find GPS library - GPSD query disabled!")
          ENDIF(GPS_FOUND)
     ENDIF(WIN32)
ELSE(ENABLE_GPS)
     SET(ENABLE_LIBGPS 0)
     MESSAGE(STATUS "GPS: disabled.")
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
          ADD_DEFINITIONS(-DINSTALL_DATADIR="${CMAKE_INSTALL_PREFIX}/${SDATALOC}")          
          # Used in generating the documentation (doc/stellarium.pod.cmake):
          SET(INSTALL_DATADIR "${CMAKE_INSTALL_PREFIX}/${SDATALOC}")
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

IF(WIN32)
     #######################################################
     ### Generate icon file name
     #######################################################

     IF(STELLARIUM_RELEASE_BUILD)
          SET(PACKAGE_ICON "stellarium")
     ELSE()
          SET(PACKAGE_ICON "stellarium-gray")
     ENDIF()

     #######################################################
     ### Generate a VersionInfo file
     #######################################################

     IF(STELLARIUM_RELEASE_BUILD)
          SET(PACKAGE_VERSION_RC "${PACKAGE_VERSION},0")
     ELSE()
          SET(PACKAGE_VERSION_RC "${WINDOWS_PACKAGE_VERSION}")
     ENDIF()
     STRING(REGEX REPLACE "([.]+)" "," PACKAGE_VERSION_RC ${PACKAGE_VERSION_RC})
     CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/stellarium.rc.cmake ${CMAKE_CURRENT_BINARY_DIR}/stellarium.rc @ONLY)

     #######################################################
     ### [Optional] Embed icon in the Windows executable
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
     ### Generate an Inno Setup project file
     #######################################################

     IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "4")
          SET(ISS_ARCHITECTURE_SPECIFIC ";In the 64-bit only version, this line switches the installer to 64-bit mode.")
          SET(ISS_PACKAGE_PLATFORM "win32")
     ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
          SET(ISS_ARCHITECTURE_SPECIFIC ";Make the installer run only on win64:\nArchitecturesAllowed=x64\n;Switch to 64-bit install mode:\nArchitecturesInstallIn64BitMode=x64")
          SET(ISS_PACKAGE_PLATFORM "win64")
     ENDIF()
     SET(ISS_AUTOGENERATED_WARNING "Do not edit this file! It has been automatically generated by CMake. Your changes will be lost the next time CMake is run.")

     GET_FILENAME_COMPONENT(_qt_bin_dir "${QMAKE_LOCATION}" DIRECTORY)
     FIND_PROGRAM(WINDEPLOYQT_COMMAND windeployqt HINTS "${_qt_bin_dir}")

     SET(ISS_STELLARIUM_STUFF "; Additional libraries")
     IF(ENABLE_SPOUT)
          IF(SPOUT_LIBRARY_DLL) # we know the dll
               MESSAGE(STATUS "Found Spout library: ${SPOUT_LIBRARY_DLL}")
               SET(ISS_STELLARIUM_STUFF "${ISS_STELLARIUM_STUFF}\nSource: \"${SPOUT_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
               SET(ISS_STELLARIUM_STUFF "${ISS_STELLARIUM_STUFF}\nSource: \"${SPOUT_LIBRARY_LICENCE}\"; DestDir: \"{app}\"; DestName: \"Spout_licence.txt\"; Flags: ignoreversion")
          ENDIF()
          SET(ISS_SPOUT "Name: \"{group}\\Stellarium {cm:SpoutMode}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
          # SET(ISS_SPOUT "${ISS_SPOUT}\nName: \"{group}\\Stellarium {cm:SpoutMode2K}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky --spoutRes=2048.2048\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
          # SET(ISS_SPOUT "${ISS_SPOUT}\nName: \"{group}\\Stellarium {cm:SpoutMode4K}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky --spoutRes=4096.4096\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
          # SET(ISS_SPOUT "${ISS_SPOUT}\nName: \"{group}\\Stellarium {cm:SpoutMode8K}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky --spoutRes=8192.8192\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
     ELSE()
          SET(ISS_SPOUT "; Spout support is disabled")
     ENDIF(ENABLE_SPOUT)
ENDIF(WIN32)


IF(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
     ADD_DEFINITIONS(-DQT_NO_DEBUG)
     ADD_DEFINITIONS(-DNDEBUG)
ENDIF()

CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/default_cfg.ini.cmake ${CMAKE_SOURCE_DIR}/data/default_cfg.ini @ONLY)

########### uninstall files ###############
CONFIGURE_FILE(
     "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
     "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
     IMMEDIATE @ONLY)

ADD_CUSTOM_TARGET(uninstall
     "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")
SET_TARGET_PROPERTIES(uninstall PROPERTIES FOLDER "src")

########### Packaging info for CPack ###########

IF(GENERATE_PACKAGE_TARGET)
     IF(MSVC_VERSION GREATER 1800)
          SET(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
     ENDIF()
     INCLUDE(InstallRequiredSystemLibraries)
     SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Stellarium is a free open source planetarium for your computer. It shows a realistic sky in 3D, just like what you see with the naked eye, binoculars or a telescope.")
     SET(CPACK_PACKAGE_VENDOR "Stellarium's team")
     SET(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
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
     SET(CPACK_SOURCE_IGNORE_FILES "/.tx/" "/CVS/" "/.svn/" "/.git/" "/.github/" "/.bzr/" "builds/" "installers/" "util/spout2" "util/DSSToStellarium/toastForShape$" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "/CMakeLists.txt.user$" "\\\\.bzrignore$" "\\\\.gitignore$" "\\\\.clang-format$" "\\\\.yml$" "~$" "\\\\.swp$" "\\\\.#" "/#")
     SET(CPACK_RPM_PACKAGE_LICENSE "GNU GPLv2 or later")
     SET(CPACK_RPM_PACKAGE_GROUP "Amusements/Graphics")
     SET(CPACK_RPM_PACKAGE_URL "${STELLARIUM_URL}")
     SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_VENDOR} <stellarium@googlegroups.com>")
     SET(CPACK_DEBIAN_PACKAGE_SECTION "science")
     SET(CPACK_DEBIAN_PACKAGE_VERSION "${VERSION}+deb1")
     SET(CPACK_DEBIAN_PACKAGE_HOMEPAGE "${CPACK_RPM_PACKAGE_URL}")
     SET(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
     INCLUDE(CPack)
ENDIF()

########### OS X Bundling ###############
IF(APPLE)
     GET_FILENAME_COMPONENT(_qt_bin_dir "${QMAKE_LOCATION}" DIRECTORY)
     FIND_PROGRAM(MACDEPLOYQT_COMMAND macdeployqt HINTS "${_qt_bin_dir}")

     IF(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
          SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum OS X deployment version" FORCE)
     ENDIF()
     MESSAGE(STATUS "Mac OS X deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET}")
     SET(PACKAGE_OSX_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
     ADD_DEFINITIONS(-DPACKAGE_OSX_TARGET)
     CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/Info.plist.cmake ${CMAKE_SOURCE_DIR}/data/Info.plist @ONLY)
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
          MESSAGE(STATUS "Could NOT find Inno Setup Compiler (missing: ${ISS_COMPILER_FILENAME}). You won't be able to build setup files.")
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
