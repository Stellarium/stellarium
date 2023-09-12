CMAKE_MINIMUM_REQUIRED(VERSION 3.16)
MESSAGE(STATUS "Found CMake ${CMAKE_VERSION}")

# Fix behavior of CMAKE_CXX_STANDARD and CMAKE_C_STANDARD when targeting macOS.
IF(POLICY CMP0025)
     CMAKE_POLICY(SET CMP0025 NEW)
ENDIF()
# Potential dangerous comparison of variables. Details: https://cmake.org/cmake/help/v3.1/policy/CMP0054.html
IF(POLICY CMP0054)
     CMAKE_POLICY(SET CMP0054 NEW)
ENDIF()
IF(POLICY CMP0071)
     CMAKE_POLICY(SET CMP0071 NEW)
ENDIF()

# warn about deprecated stuff so that we may try fixing it
SET(CMAKE_WARN_DEPRECATED 1)

SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

########### Project name ###########
PROJECT(Stellarium C CXX)

SET(STELLARIUM_RELEASE_BUILD 0 CACHE BOOL "Set 1 to build as an official release (0 for development snapshots).")
# Number of the release (0 is used before first release)
SET(STELLARIUM_RELEASE_NUMBER 2)

########### Detect Qt version
SET(ENABLE_QT6 1 CACHE BOOL "Whether to try building with Qt6. If Qt6 is not found, Qt5 will be used.")
IF(ENABLE_QT6)
    FIND_PACKAGE(Qt6 COMPONENTS Core QUIET)
ENDIF()
if (NOT Qt6_FOUND)
    FIND_PACKAGE(Qt5 REQUIRED COMPONENTS Core)
ENDIF()

# Activate this to error on all things deprecated in 5.15
#add_compile_definitions("QT_DISABLE_DEPRECATED_BEFORE=0x050F00")

# two last digits of the current year
STRING(TIMESTAMP SHORTYEAR "%y")
MATH(EXPR STELLARIUM_MAJOR_DIGIT "${SHORTYEAR} + 0" OUTPUT_FORMAT DECIMAL)
# Version of Stellarium has format YY.V.YD, where:
# -- YY (MAJOR) - two last digits of the year of release
# -- V  (MINOR) - version of the release (0 is used before first release)
# -- YD (PATCH) - 0 for releases and the day of the current year for snapshots
# Example: first release in year 2023 has version 23.1.0 and short (public) version 23.1, series 23.0
# Note: Use integer versions instead of strings for easier handling if required
SET(STELLARIUM_MAJOR ${STELLARIUM_MAJOR_DIGIT})
SET(STELLARIUM_MINOR ${STELLARIUM_RELEASE_NUMBER})
IF(STELLARIUM_RELEASE_BUILD)
    ADD_DEFINITIONS(-DSTELLARIUM_RELEASE_BUILD)
    SET(CMAKE_BUILD_TYPE Release)
    SET(STELLARIUM_PATCH 0)
ELSE()
    ########### Get revision number for non-release builds ###########
    STRING(TIMESTAMP CDNUMBER "%j")
    MATH(EXPR STELLARIUM_PATCH_DIGIT "${CDNUMBER} + 0" OUTPUT_FORMAT DECIMAL)
    SET(STELLARIUM_PATCH ${STELLARIUM_PATCH_DIGIT})
ENDIF()
# Use integer versions instead of strings for easier handling if required
ADD_DEFINITIONS(
     -DSTELLARIUM_MAJOR=${STELLARIUM_MAJOR}
     -DSTELLARIUM_MINOR=${STELLARIUM_MINOR}
     -DSTELLARIUM_PATCH=${STELLARIUM_PATCH}
)
SET(VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}.${STELLARIUM_PATCH}")
SET(STELLARIUM_SERIES "${STELLARIUM_MAJOR}.0")
SET(STELLARIUM_PUBLIC_VERSION "${STELLARIUM_MAJOR}.${STELLARIUM_MINOR}")
SET(PACKAGE stellarium)
STRING(TIMESTAMP CURRENTYEAR "%Y")
SET(COPYRIGHT_YEARS "2000-${CURRENTYEAR}")

SET(STELLARIUM_URL "https://stellarium.org/")
ADD_DEFINITIONS(-DSTELLARIUM_URL="${STELLARIUM_URL}")

# The line below is copied from src/gui/HelpDialog.cpp file
SET(STELLARIUM_COPYRIGHT "Copyright (C) ${COPYRIGHT_YEARS} ${PROJECT_NAME} Developers")
# The line below is copied from src/main.cpp file
#SET(STELLARIUM_COPYRIGHT "Copyright (C) ${COPYRIGHT_YEARS} Fabien Chereau et al.")
ADD_DEFINITIONS(-DSTELLARIUM_COPYRIGHT="${STELLARIUM_COPYRIGHT}")

# Define URL for usage within program (feedback URL for plug-ins)
SET(STELLARIUM_DEV_URL "https://github.com/Stellarium/stellarium")
ADD_DEFINITIONS(-DSTELLARIUM_DEV_URL="${STELLARIUM_DEV_URL}")

SET(PACKAGE_VERSION "${VERSION}")
ADD_DEFINITIONS(-DPACKAGE_VERSION="${PACKAGE_VERSION}")
ADD_DEFINITIONS(-DCOPYRIGHT_YEARS="${COPYRIGHT_YEARS}")
ADD_DEFINITIONS(-DSTELLARIUM_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
ADD_DEFINITIONS(-DSTELLARIUM_PUBLIC_VERSION="${STELLARIUM_PUBLIC_VERSION}")
ADD_DEFINITIONS(-DSTELLARIUM_SERIES="${STELLARIUM_SERIES}")

# Qt 5.15+ is preferred as it provides version-less CMake functions (QT_WRAP_UI etc.)
IF(Qt6_FOUND)
     SET(MINIMAL_QT_VERSION "6.2.0")
     IF(NOT DEFINED QT_VERSION_MAJOR)
         SET(QT_VERSION_MAJOR 6)
     ENDIF()
ELSE()
     SET(MINIMAL_QT_VERSION "5.12.0")
     IF(NOT DEFINED QT_VERSION_MAJOR)
         SET(QT_VERSION_MAJOR 5)
     ENDIF()
ENDIF()
# TODO: Check real minimal supported version of GPSD API
SET(MINIMAL_GPS_API_VERSION "6.0")

### Define minimal version for Windows
# Use installer for checking minimum requirements
# Stellarium/MSVC require Windows 7 (6.1) for work
# Details: http://www.jrsoftware.org/ishelp/index.php?topic=winvernotes
IF(Qt6_FOUND)
     SET(ISS_MIN_WIN_VERSION "10.0")
ELSE()
     SET(ISS_MIN_WIN_VERSION "6.1")
ENDIF()
# Define required version for MSVC
# Details: https://docs.microsoft.com/ru-ru/windows/win32/winprog/using-the-windows-headers
IF(Qt6_FOUND)
     SET(MIN_WIN_VERSION "0x0A00")
ELSE()
     SET(MIN_WIN_VERSION "0x0601")
ENDIF()

# Use customized cmake macros
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules)
INCLUDE(CPM)

# Show platform info
MESSAGE(STATUS "Platform: ${CMAKE_SYSTEM} (${CMAKE_SYSTEM_PROCESSOR})")

########### Main global variables ###########
SET(ENABLE_TESTING 0 CACHE BOOL "Define whether the unit tests should be activated.")
SET(ENABLE_LTO 0 CACHE BOOL "Define whether the Link Time Optimization should be activated.")
SET(ENABLE_PCH 1 CACHE BOOL "Define whether to use precompiled headers.")

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

IF(STELLARIUM_RELEASE_BUILD)
     SET(ISS_PACKAGE_VERSION "${STELLARIUM_PUBLIC_VERSION}")
     SET(STELLARIUM_BUIDING_VERSION "${STELLARIUM_PUBLIC_VERSION}")
ELSE()
     SET(STELLARIUM_SERIES "head")
     FIND_PACKAGE(Git QUIET)
     IF(Git_FOUND)
          MESSAGE(STATUS "Found Git ${GIT_VERSION_STRING}")
          # get the name of current branch
          EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
          # get the hash of latest commit
          EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} log -1 --pretty=format:%h WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
          # get the name of latest annotated tag
          EXECUTE_PROCESS(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} TIMEOUT 30 OUTPUT_VARIABLE GIT_TAGS OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_VARIABLE GIT_ERROR)
	  IF(GIT_REVISION)
               # Let's use short hash in all cases
               STRING(SUBSTRING ${GIT_REVISION} 0 7 REVISION)               
	       ADD_DEFINITIONS(-DGIT_REVISION="${REVISION}")
               ADD_DEFINITIONS(-DGIT_BRANCH="${GIT_BRANCH}")
	  ENDIF()
     ELSE()
          STRING(TIMESTAMP TODAY "%j%H%M")
          SET(REVISION "${TODAY}")
	  ADD_DEFINITIONS(-DGIT_REVISION="${TODAY}")
          ADD_DEFINITIONS(-DGIT_BRANCH="tarball")
     ENDIF()
     SET(ISS_PACKAGE_VERSION "${STELLARIUM_PUBLIC_VERSION}-${REVISION}")
     SET(STELLARIUM_BUIDING_VERSION "${STELLARIUM_PUBLIC_VERSION}+")
ENDIF()
MESSAGE(STATUS "Building ${PROJECT_NAME} ${STELLARIUM_BUIDING_VERSION} (v${PACKAGE_VERSION}; Mode: ${CMAKE_BUILD_TYPE})")

# Write version and series of Stellarium to Stellarium User Guide
CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/version.tex.cmake ${PROJECT_SOURCE_DIR}/guide/version.tex @ONLY NEWLINE_STYLE LF)

SET(OPENGL_DEBUG_LOGGING 0 CACHE BOOL "Enable to log OpenGL information using the GL_KHR_debug extension/QOpenGLLogger")
IF(OPENGL_DEBUG_LOGGING)
     ADD_DEFINITIONS(-DOPENGL_DEBUG_LOGGING)
     # This enables logging of QOpenGLFunctions through forced glGetError after each call
     ADD_DEFINITIONS(-DQ_ENABLE_OPENGL_FUNCTIONS_DEBUG)
ENDIF()

# Use ccache if possible
IF(NOT WIN32)
     FIND_PROGRAM(CCACHE_PROGRAM ccache)
     IF(CCACHE_PROGRAM)
          MESSAGE(STATUS "Found ccache ${CCACHE_PROGRAM}")
          SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
     ENDIF()
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
#          https://docs.microsoft.com/ru-ru/cpp/build/reference/fsanitize?view=msvc-150
IF(${CMAKE_BUILD_TYPE} MATCHES "ASan")
     IF(MSVC)
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=address")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address")
     ELSE()
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -O1 -fno-omit-frame-pointer -g")
     ENDIF()
ENDIF()

# Add UBSan (UndefinedBehaviorSanitizer) build options if necessary
# Details: https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html
IF(${CMAKE_BUILD_TYPE} MATCHES "UBSan")
     SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined -g")
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -g")
ENDIF()

# Add Fuzzer (Fuzzer Sanitizer) build options if necessary
# Details: https://llvm.org/docs/LibFuzzer.html
#          https://docs.microsoft.com/ru-ru/cpp/build/reference/fsanitize?view=msvc-150
IF(${CMAKE_BUILD_TYPE} MATCHES "Fuzzer")
     IF(MSVC)
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /fsanitize=fuzzer")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=fuzzer")
     ELSE()
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=fuzzer -O1 -g")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=fuzzer -O1 -g")
     ENDIF()
ENDIF()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
     SET(APPLE 1)
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
     # share data location:
     SET(SDATALOC "Resources")
ELSE()
     IF(HAIKU)
	  SET(SDATAPREFIX "data")
     ELSE()
	  SET(SDATAPREFIX "share")
     ENDIF()
     # share data location:
     SET(SDATALOC "${SDATAPREFIX}/${PACKAGE}")
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
SET(STEL_GCC_C_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas")
SET(STEL_GCC_CXX_FLAGS "-Wall -Wextra -Wno-unused-parameter -Wno-unknown-pragmas")
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

# Enable using C++17 when compiling.
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)
SET(CMAKE_CXX_EXTENSIONS OFF)
# NOTE: C_STANDARD 17 and 23 values added in CMake 3.21
# https://gitlab.kitware.com/cmake/cmake/-/issues/22366
# Ubuntu 18.04 have GCC 7.5 - so, C11 only
SET(CMAKE_C_STANDARD 11)
SET(CMAKE_C_STANDARD_REQUIRED ON)
SET(CMAKE_C_EXTENSIONS OFF)

IF(WIN32)
     # We don't need the extra Windows.h stuff, this may speed up compilation a tiny bit
     # This should also prevent some Winsock.h warnings
     ADD_DEFINITIONS(-DWIN32_LEAN_AND_MEAN)
     IF(NOT MSVC)
          # MinGW requires enabling of exceptions, version number storage and MinGW-specific threading
          SET(STEL_MINGW_FLAGS "-fexceptions -fident -mthreads")
          SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${STEL_GCC_C_FLAGS} ${STEL_MINGW_FLAGS}")
          SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STEL_GCC_CXX_FLAGS} ${STEL_MINGW_FLAGS}")
     ELSE()
          # MSVC
          # Additional flags:
          # Disabled warnings
          # C4244: implicit type conversion to a smaller type
          # C4305: type truncation
          # C4351: "new" behaviour, member array default initialization. Required since at least C++98, but funny MSVC throws a warning.
          # C4996: deprecated POSIX names (used in zlib)
	  # C5105: defines in macros
          SET(STEL_MSVC_FLAGS "/wd4244 /wd4305 /wd4351 /wd4996 /wd5105")
          # Avoid type conflict with C++17 standard
          # SET(STEL_MSVC_FLAGS "${STEL_MSVC_FLAGS} /D_HAS_STD_BYTE=0") # Don't do this in Qt6. Just avoid "using namespace std" anywhere! https://developercommunity.visualstudio.com/t/error-c2872-byte-ambiguous-symbol/93889
          # Set multiprocessing and minimal version of Windows
          SET(STEL_MSVC_FLAGS "${STEL_MSVC_FLAGS} /MP /D_WIN32_WINNT=${MIN_WIN_VERSION}")

          FOREACH(flag_var
               CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO
               CMAKE_SHARED_LINKER_FLAGS_DEBUG CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO)
               SET(${flag_var} "${${flag_var}} /DEBUG:FASTLINK")
          ENDFOREACH()
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
     IF(APPLE)
	  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fno-common -Wall -Wextra -Wno-unused-parameter -Wno-string-plus-int")
	  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common -Wall -Wextra -Wno-unknown-warning-option -Wno-string-plus-int")
     ENDIF()
ENDIF()

# FreeBSD-specific compiler flags
# resolve bug for FreeBSD/amd64 and NVIDIA proprietary drivers 
IF(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
     # Use -pthread compilation option to properly link to threading library
     SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wno-unused-local-typedefs")
ENDIF()

# This activates Link Time Optimization
# LTO has longer build times but optimizes code (smaller by 15-20 percents)
# LTO fails a test for unknown reasons on Linux/GCC and on Windows/MSVC.
IF(ENABLE_LTO)
     IF(WIN32 AND MSVC)
	  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /GL")
	  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /GL")
	  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LTCG")
     ELSE()
	  SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -flto")
	  SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -flto")
	  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${CMAKE_CXX_FLAGS}")
     ENDIF()
ENDIF()

########### Others ###########
# Activate translation
SET(ENABLE_NLS 1 CACHE BOOL "Define whether program translation should be supported.")
IF(ENABLE_NLS)
     ADD_DEFINITIONS(-DENABLE_NLS)
ENDIF()

# Activate media support
SET(ENABLE_MEDIA 1 CACHE BOOL "Define whether media support should be activated.")

# Activate QtWebEngine support
SET(ENABLE_QTWEBENGINE 1 CACHE BOOL "Define whether QtWebEngine module should be supported if it installed.")

# Activate GPS support. Solution depends on operating system:
# - Windows: Only directly-attached serial NMEA emitter.
# - Other systems: libgps if possible and NMEA emitter as fallback.
SET(ENABLE_GPS 1 CACHE BOOL "Define whether GPS queries should be supported.")

# Activate support for ShowMySky atmosphere
SET(ENABLE_SHOWMYSKY 1 CACHE BOOL "Define whether to support ShowMySky atmosphere model if installed")

# Activate support for XLSX (Excel) files
SET(ENABLE_XLSX 1 CACHE BOOL "Define whether to support XLSX (Excel) files")
IF(ENABLE_XLSX)
     ADD_DEFINITIONS(-DENABLE_XLSX)
ENDIF()

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
     ENDIF(ENABLE_SCRIPT_CONSOLE)
     IF(Qt5_FOUND)
          SET(ENABLE_SCRIPT_QML 0 CACHE BOOL "Define whether scripting should be handled by the QML module, not the deprecated QtScript. Requires Qt5.14 or later.")
     ELSE(Qt5_FOUND)
          # With Qt6, this is compulsory.
          SET(ENABLE_SCRIPT_QML 1)
     ENDIF(Qt5_FOUND)
     IF(ENABLE_SCRIPT_QML)
          SET(MINIMAL_QT_VERSION "5.14.0")
          ADD_DEFINITIONS(-DENABLE_SCRIPT_QML)
     ENDIF(ENABLE_SCRIPT_QML)
ENDIF(ENABLE_SCRIPTING)

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

########### User Guide ###############
IF(EXISTS "${CMAKE_SOURCE_DIR}/guide/guide.pdf")
     MESSAGE(STATUS "Found Stellarium User Guide")
     INSTALL(FILES guide/guide.pdf DESTINATION ${SDATALOC}/guide)
     SET(ISS_GUIDE "Name: \"{group}\\{cm:UserGuide}\"; Filename: \"{app}\\guide\\guide.pdf\"")
     SET(ISS_GUIDE_CD "Name: \"{commondesktop}\\{cm:UserGuide}\"; Filename: \"{app}\\guide\\guide.pdf\"; Tasks: desktopicon\\common")
     SET(ISS_GUIDE_UD "Name: \"{userdesktop}\\{cm:UserGuide}\"; Filename: \"{app}\\guide\\guide.pdf\"; Tasks: desktopicon\\user")
ELSE()
     SET(ISS_GUIDE "; No link to Stellarium User Guide")
     SET(ISS_GUIDE_CD "; No link to Stellarium User Guide")
     SET(ISS_GUIDE_UD "; No link to Stellarium User Guide")
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
ADD_PLUGIN(MissingStars 1)
ADD_PLUGIN(NavStars 1)
ADD_PLUGIN(Novae 1)
ADD_PLUGIN(Observability 1)
ADD_PLUGIN(Oculars 1)
ADD_PLUGIN(Oculus 0)
ADD_PLUGIN(OnlineQueries 1)
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
# Candidate to removing as an archaic plugin
ADD_PLUGIN(TextUserInterface 0)
ADD_PLUGIN(TelescopeControl 1)
ADD_PLUGIN(Vts 0)

########## Static plugins need to define includes and libraries
########## for the compilation of Stellarium itself

# Custom target used to manage dependencies of stellarium -> Static plugins
# It is important that static plugins are compiled before stellarium main executable is linked
ADD_CUSTOM_TARGET(AllStaticPlugins ALL)
SET_TARGET_PROPERTIES(AllStaticPlugins PROPERTIES FOLDER "plugins")

########### Find packages ###########
FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Core REQUIRED)
# Catch the real path for used Qt Framework
GET_TARGET_PROPERTY(QMAKE_LOCATION Qt${QT_VERSION_MAJOR}::qmake LOCATION)
IF(NOT QT_VERSION)
     # Variable ${QT_VERSION} is not defined, but no problem - we can get it from qmake
     EXECUTE_PROCESS(COMMAND ${QMAKE_LOCATION} -query QT_VERSION OUTPUT_VARIABLE QT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
     STRING(REPLACE " " "" QT_VERSION ${QT_VERSION})
ENDIF()
IF(${QT_VERSION} VERSION_LESS MINIMAL_QT_VERSION)
     MESSAGE(FATAL_ERROR "Found Qt${QT_VERSION_MAJOR}: ${QMAKE_LOCATION} (found unsuitable version ${QT_VERSION}, required is ${MINIMAL_QT_VERSION})")
ELSE()
     MESSAGE(STATUS "Found Qt${QT_VERSION_MAJOR}: ${QMAKE_LOCATION} (found suitable version ${QT_VERSION})")
ENDIF()
FIND_PACKAGE(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS
     Concurrent Gui Network Widgets Charts)
IF(USE_PLUGIN_TELESCOPECONTROL OR ENABLE_GPS)
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS SerialPort REQUIRED)
ENDIF()
IF(WIN32)
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Svg REQUIRED)
     IF(${QT_VERSION_MAJOR} EQUAL 5)
          FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS XmlPatterns REQUIRED)
     ENDIF()

     # Qt 5.12 (LTS) and newest version of Qt use OpenSSL 1.1*
     # Source: https://lists.qt-project.org/pipermail/releasing/2019-March/002614.html
     SET(OPENSSL_LIBRARY_ARCH "")
     IF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	  SET(OPENSSL_LIBRARY_ARCH "-x64")
     ENDIF()
     # Try found OpenSSL 3.x DLL's on Windows (newest Qt)
     FIND_FILE(OPENSSL_SSL_LIBRARY_DLL libssl-3${OPENSSL_LIBRARY_ARCH}.dll)
     FIND_FILE(OPENSSL_CRYPTO_LIBRARY_DLL libcrypto-3${OPENSSL_LIBRARY_ARCH}.dll)
     # OK, OpenSSL 3.x is not exist, let's try found OpenSSL 1.1.x DLL's
     IF(NOT EXISTS ${OPENSSL_SSL_LIBRARY_DLL})
	  FIND_FILE(OPENSSL_SSL_LIBRARY_DLL libssl-1_1${OPENSSL_LIBRARY_ARCH}.dll)
     ENDIF()
     IF(NOT EXISTS ${OPENSSL_CRYPTO_LIBRARY_DLL})
	  FIND_FILE(OPENSSL_CRYPTO_LIBRARY_DLL libcrypto-1_1${OPENSSL_LIBRARY_ARCH}.dll)
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

IF(APPLE)
     IF(${QT_VERSION_MAJOR} EQUAL 6)
	  FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS DBus REQUIRED)
     ENDIF()
     # macOS architectures: x86_64 or arm64
     IF(NOT CMAKE_OSX_ARCHITECTURES)
	  # Find 'lipo' utility
	  FIND_PROGRAM(LIPO_COMMAND lipo)
	  IF(LIPO_COMMAND)
	       MESSAGE(STATUS "Found lipo: ${LIPO_COMMAND}")
	       # Get architecture of Qt
	       EXECUTE_PROCESS(COMMAND ${LIPO_COMMAND} -archs ${QMAKE_LOCATION} TIMEOUT 30 OUTPUT_VARIABLE QT_ARCHS OUTPUT_STRIP_TRAILING_WHITESPACE)
	       STRING(REPLACE " " ";" QT_ARCHS ${QT_ARCHS})
	       SET(CMAKE_OSX_ARCHITECTURES "${QT_ARCHS}" CACHE STRING "macOS architectures" FORCE)
	  ELSE()
	       # If we can't find architecture of Qt, use that of the host system's CPU
	       SET(CMAKE_OSX_ARCHITECTURES "${CMAKE_HOST_SYSTEM_PROCESSOR}" CACHE STRING "macOS architectures" FORCE)
	  ENDIF()
     ENDIF()
     # macOS deployment targets
     IF(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
	  IF(${QT_VERSION_MAJOR} EQUAL 6)
	       # Qt6: by default - universal binary + minimal operating system is macOS Big Sur
	       SET(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS deployment version" FORCE)
	  ELSE()
	       SET(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS deployment version" FORCE)
	  ENDIF()
     ENDIF()
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
SET(CMAKE_AUTORCC ON) # ?
SET(CMAKE_AUTOUIC ON) # ?
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

IF(ENABLE_SHOWMYSKY)
    ADD_DEFINITIONS(-DENABLE_SHOWMYSKY)
    CPMFindPackage(NAME ShowMySky-Qt${QT_VERSION_MAJOR}
		URL https://github.com/10110111/CalcMySky/archive/refs/tags/v0.3.1.tar.gz
        URL_HASH SHA256=d284eaabcf21ad3d1f520a1d1b1b533f7c5f5a44ba92495dd38e238874e8e4d7
        EXCLUDE_FROM_ALL yes
        OPTIONS "QT_VERSION ${QT_VERSION_MAJOR}")
    IF(ShowMySky-Qt${QT_VERSION_MAJOR}_ADDED)
        # Don't want to link to ShowMySky::ShowMySky directly, so find out its
        # include dir and lib location manually.
        GET_TARGET_PROPERTY(ShowMySky_INCLUDE_DIRECTORIES
            ShowMySky::ShowMySky INTERFACE_INCLUDE_DIRECTORIES)
        SET(ShowMySky_LIBRARY $<TARGET_FILE:ShowMySky::ShowMySky>)
        MESSAGE(STATUS "Will build ShowMySky library")
        INCLUDE(GNUInstallDirs)
	IF(APPLE)
	    INSTALL(TARGETS ShowMySky LIBRARY DESTINATION Frameworks)	
	ELSE()
	    INSTALL(TARGETS ShowMySky
		    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
	ENDIF()
        # We used EXCLUDE_FROM_ALL in CPMFindPackage to avoid building
        # irrelevant binaries of CalcMySky. But we still need to build
        # libShowMySky, so add some target which would depend on it.
        # stelMain is not defined at this point yet, so can't use it.
        FILE(WRITE ${CMAKE_CURRENT_BINARY_DIR}/fake-show-my-sky-client.cpp "")
        ADD_LIBRARY(fake-show-my-sky-client SHARED
            ${CMAKE_CURRENT_BINARY_DIR}/fake-show-my-sky-client.cpp)
        ADD_DEPENDENCIES(fake-show-my-sky-client ShowMySky::ShowMySky)
    ELSE()
        GET_TARGET_PROPERTY(ShowMySky_INCLUDE_DIRECTORIES
            ShowMySky::ShowMySky INTERFACE_INCLUDE_DIRECTORIES)
        GET_TARGET_PROPERTY(ShowMySky_LIBRARY
            ShowMySky::ShowMySky LOCATION)
        IF(EXISTS ${ShowMySky_LIBRARY})
            MESSAGE(STATUS "Found ShowMySky library: ${ShowMySky_LIBRARY}")	    
        ELSE()
            MESSAGE(FATAL_ERROR "Could NOT found ShowMySky library")
        ENDIF()
    ENDIF()
ENDIF(ENABLE_SHOWMYSKY)

IF(ENABLE_SCRIPTING)
     IF(ENABLE_SCRIPT_QML)
          FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Qml REQUIRED)
     ELSE(ENABLE_SCRIPT_QML)
          FIND_PACKAGE(Qt5Script REQUIRED)
     ENDIF(ENABLE_SCRIPT_QML)
ENDIF(ENABLE_SCRIPTING)

IF(ENABLE_MEDIA)
     ADD_DEFINITIONS(-DENABLE_MEDIA)
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Multimedia REQUIRED)
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS MultimediaWidgets REQUIRED)
ENDIF()

IF(ENABLE_GPS)
     MESSAGE(STATUS "GPS: support by Qt's NMEA handling enabled.")
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Positioning REQUIRED)
     ADD_DEFINITIONS(-DENABLE_GPS)
     IF(WIN32)
          SET(ENABLE_LIBGPS 0)
     ELSEIF(NOT DEFINED ENABLE_LIBGPS)
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
	  ELSE()
               SET(ENABLE_LIBGPS 0)
               MESSAGE(STATUS "GPS: could not find GPS library - GPSD query disabled!")
	  ENDIF()
     ENDIF()
ELSE()
     SET(ENABLE_LIBGPS 0)
     MESSAGE(STATUS "GPS: disabled.")
ENDIF()

IF(ENABLE_MEDIA OR USE_PLUGIN_SCENERY3D)
     # The Scenery3D plugin requires it directly and the QtMultimediaWidgets module requires it internally
     FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS OpenGL REQUIRED)
ENDIF()

IF(ENABLE_TESTING)
    ENABLE_TESTING()
    ADD_DEFINITIONS(-DENABLE_TESTING)
    FIND_PACKAGE(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)
    MESSAGE(STATUS "Unit tests support: enabled")
ELSE(ENABLE_TESTING)
    MESSAGE(STATUS "Unit tests support: disabled")
ENDIF(ENABLE_TESTING)

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

IF(ENABLE_XLSX)
     # TODO: switch to something like vcpkg or conan, and plain find_package,
     # without the fallback of cmake downloading the dep itself.
     # But some libraries are not available there yet.

     # Version 1.4.4 exports "QXlsx", but future (unreleased as of 10 Nov 2022)
     # version exports "QXlsxQt5" (or ...Qt6) instead. Try both, but
     # download newer one, because 1.4.4 doesn't link on windows when
     # added via add_subdirectory().
     FIND_PACKAGE(QXlsx NAMES QXlsxQt${QT_VERSION_MAJOR} QXlsx)
     IF(QXlsx_FOUND)
         MESSAGE(STATUS "Using system-provided QXlsx ${QXlsx_VERSION}")
     ELSE()
         CPMAddPackage(NAME QXlsxQt${QT_VERSION_MAJOR}
             URL https://github.com/QtExcel/QXlsx/archive/refs/tags/v1.4.6.tar.gz
             URL_HASH SHA256=969a947af6a78dad9f2418dfc84e528d212ceb98e6392edf859dbff4f8828a77
             EXCLUDE_FROM_ALL ON
             SOURCE_SUBDIR QXlsx)
     ENDIF()
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

     SET(WINDOWS_PACKAGE_VERSION "${VERSION}.0")
     SET(PACKAGE_VERSION_RC "${WINDOWS_PACKAGE_VERSION}")
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
	  SET(ISS_ARCHITECTURE_SPECIFIC "")
	  SET(ISS_PACKAGE_PLATFORM "qt${QT_VERSION_MAJOR}-win32")
     ELSEIF("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
	  SET(ISS_ARCHITECTURE_SPECIFIC ";Switch to 64-bit install mode:\nArchitecturesInstallIn64BitMode=x64")
	  SET(ISS_PACKAGE_PLATFORM "qt${QT_VERSION_MAJOR}-win64")
     ENDIF()
     SET(ISS_AUTOGENERATED_WARNING "Do not edit this file! It has been automatically generated by CMake. Your changes will be lost the next time CMake is run.")

     GET_FILENAME_COMPONENT(_qt_bin_dir "${QMAKE_LOCATION}" DIRECTORY)
     FIND_PROGRAM(WINDEPLOYQT_COMMAND windeployqt HINTS "${_qt_bin_dir}")
     IF(WINDEPLOYQT_COMMAND)
	  MESSAGE(STATUS "Found windeployqt: ${WINDEPLOYQT_COMMAND}")
     ELSE()
	  MESSAGE(STATUS "Could NOT find windeployqt")
     ENDIF()

     SET(ISS_STELLARIUM_STUFF "; Additional libraries")
     IF(ENABLE_SPOUT)
          IF(SPOUT_LIBRARY_DLL) # we know the dll
               MESSAGE(STATUS "Found Spout library: ${SPOUT_LIBRARY_DLL}")
               SET(ISS_STELLARIUM_STUFF "${ISS_STELLARIUM_STUFF}\nSource: \"${SPOUT_LIBRARY_DLL}\"; DestDir: \"{app}\"; Flags: ignoreversion")
               SET(ISS_STELLARIUM_STUFF "${ISS_STELLARIUM_STUFF}\nSource: \"${SPOUT_LIBRARY_LICENCE}\"; DestDir: \"{app}\"; DestName: \"Spout_licence.txt\"; Flags: ignoreversion")
          ENDIF()
          SET(ISS_SPOUT "Name: \"{group}\\Stellarium {cm:SpoutMode}\"; Filename: \"{app}\\stellarium.exe\"; Parameters: \"--spout=sky\"; WorkingDir: \"{app}\"; IconFilename: \"{app}\\data\\stellarium.ico\"\n")
     ELSE()
          SET(ISS_SPOUT "; Spout support is disabled")
     ENDIF(ENABLE_SPOUT)
     IF(ENABLE_SHOWMYSKY AND NOT ShowMySky-Qt${QT_VERSION_MAJOR}_ADDED)
          SET(ISS_STELLARIUM_STUFF "${ISS_STELLARIUM_STUFF}\nSource: \"${ShowMySky_LIBRARY}\"; DestDir: \"{app}\"; Flags: ignoreversion")
     ENDIF()
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
     IF(STELLARIUM_RELEASE_BUILD)
	  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "stellarium-${STELLARIUM_PUBLIC_VERSION}")
     ELSE()
	  SET(CPACK_SOURCE_PACKAGE_FILE_NAME "stellarium-${STELLARIUM_PUBLIC_VERSION}-${GIT_REVISION}")
     ENDIF()
     SET(CPACK_SOURCE_GENERATOR "TBZ2;TXZ;TGZ;ZIP")
     SET(CPACK_GENERATOR "TBZ2;TXZ;TGZ;ZIP")
     SET(CPACK_PACKAGE_CHECKSUM "SHA256")
     SET(CPACK_STRIP_FILES "bin/stellarium")
     SET(CPACK_PACKAGE_EXECUTABLES "stellarium" "Stellarium")
     SET(CPACK_SOURCE_IGNORE_FILES "/.git/" "/.github/" "builds/" "installers/" "po/stellarium-landscapes-descriptions" "po/stellarium-scenery3d-descriptions" "po/stellarium-skycultures-descriptions" "po/stellarium-desktop" "po/stellarium-metainfo" "util/spout2" "util/DSSToStellarium/toastForShape$" "Stellarium.tag$" "Stellarium.kdevelop.pcs$" "/CMakeLists.txt.user$" "\\\\.gitignore$" "\\\\.clang-format$" "\\\\.yml$" "~$" "\\\\.swp$" "\\\\.#" "/#")
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

########### macOS Bundling ###############
IF(APPLE)
     MESSAGE(STATUS "macOS deployment target: ${CMAKE_OSX_DEPLOYMENT_TARGET} (${CMAKE_OSX_ARCHITECTURES})")
     SET(PACKAGE_OSX_TARGET ${CMAKE_OSX_DEPLOYMENT_TARGET})
     ADD_DEFINITIONS(-DPACKAGE_OSX_TARGET)

     CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/cmake/Info.plist.cmake ${CMAKE_SOURCE_DIR}/data/Info.plist @ONLY)

     GET_FILENAME_COMPONENT(_qt_bin_dir "${QMAKE_LOCATION}" DIRECTORY)
     FIND_PROGRAM(MACDEPLOYQT_COMMAND macdeployqt HINTS "${_qt_bin_dir}")
     IF(MACDEPLOYQT_COMMAND)
	  MESSAGE(STATUS "Found macdeployqt: ${MACDEPLOYQT_COMMAND}")
     ELSE()
	  MESSAGE(STATUS "Could NOT find macdeployqt")
     ENDIF()
ENDIF()

########### Windows packaging ###############
IF(WIN32)
     CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/stellarium.iss.cmake ${CMAKE_CURRENT_BINARY_DIR}/stellarium.iss @ONLY)
     CONFIGURE_FILE(${PROJECT_SOURCE_DIR}/cmake/stellarium-patch.iss.cmake ${CMAKE_CURRENT_BINARY_DIR}/stellarium-patch.iss @ONLY)
     FIND_PACKAGE(ISCC)
     IF(ISS_COMPILER_FOUND)
	  # Add a full installer target
          ADD_CUSTOM_TARGET(
               stellarium-installer
	       COMMAND ${ISS_COMPILER_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/stellarium.iss"
               COMMENT "Making an installer package for Windows..."
               VERBATIM
          )
	  # Add installer target
	  ADD_CUSTOM_TARGET(
	       stellarium-patch-installer
	       COMMAND ${ISS_COMPILER_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/stellarium-patch.iss"
	       COMMENT "Making an installer package for Windows..."
	       VERBATIM
	  )
     ELSE()
	  MESSAGE(STATUS "Could NOT find Inno Setup compiler. You won't be able to build setup files.")
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
ADD_SUBDIRECTORY( landscapes )
ADD_SUBDIRECTORY( skycultures )
ADD_SUBDIRECTORY( nebulae )
ADD_SUBDIRECTORY( stars )
ADD_SUBDIRECTORY( plugins )
IF(ENABLE_SCRIPTING)
     ADD_SUBDIRECTORY( scripts )
ENDIF()
IF(USE_PLUGIN_SCENERY3D)
     # add scenery3d scenes
     ADD_SUBDIRECTORY( scenery3d )
ENDIF()
IF(ENABLE_SHOWMYSKY)
     # add atmosphere models
     ADD_SUBDIRECTORY( atmosphere )
ENDIF()
IF(ENABLE_NLS)
     ADD_SUBDIRECTORY( po )
ENDIF()
