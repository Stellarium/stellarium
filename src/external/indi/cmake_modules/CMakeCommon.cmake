
include(CheckCCompilerFlag)

IF (NOT ${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu99")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF ()

# Ccache support
IF (ANDROID OR UNIX OR APPLE)
    FIND_PROGRAM(CCACHE_FOUND ccache)
    SET(CCACHE_SUPPORT OFF CACHE BOOL "Enable ccache support")
    IF ((CCACHE_FOUND OR ANDROID) AND CCACHE_SUPPORT MATCHES ON)
        SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        SET_PROPERTY(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    ENDIF ()
ENDIF ()

# Add security (hardening flags)
IF (UNIX OR APPLE OR ANDROID)
    # Older compilers are predefining _FORTIFY_SOURCE, so defining it causes a
    # warning, which is then considered an error. Second issue is that for
    # these compilers, _FORTIFY_SOURCE must be used while optimizing, else
    # causes a warning, which also results in an error. And finally, CMake is
    # not using optimization when testing for libraries, hence breaking the build.
    CHECK_C_COMPILER_FLAG("-Werror -D_FORTIFY_SOURCE=2" COMPATIBLE_FORTIFY_SOURCE)
    IF (${COMPATIBLE_FORTIFY_SOURCE})
        SET(SEC_COMP_FLAGS "-D_FORTIFY_SOURCE=2")
    ENDIF ()
    SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -fstack-protector-all -fPIE")
    # Make sure to add optimization flag. Some systems require this for _FORTIFY_SOURCE.
    IF (NOT CMAKE_BUILD_TYPE MATCHES "MinSizeRel" AND NOT CMAKE_BUILD_TYPE MATCHES "Release" AND NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -O1")
    ENDIF ()
    IF (NOT ANDROID AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" AND NOT APPLE)
        SET(SEC_COMP_FLAGS "${SEC_COMP_FLAGS} -Wa,--noexecstack")
    ENDIF ()
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SEC_COMP_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SEC_COMP_FLAGS}")
    SET(SEC_LINK_FLAGS "")
    IF (NOT APPLE)
        SET(SEC_LINK_FLAGS "${SEC_LINK_FLAGS} -Wl,-z,nodump -Wl,-z,noexecstack -Wl,-z,relro -Wl,-z,now")
    ENDIF ()
    IF (NOT ANDROID AND NOT APPLE)
        SET(SEC_LINK_FLAGS "${SEC_LINK_FLAGS} -pie")
    ENDIF ()
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SEC_LINK_FLAGS}")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SEC_LINK_FLAGS}")
ENDIF ()

# Warning, debug and linker flags
SET(FIX_WARNINGS OFF CACHE BOOL "Enable strict compilation mode to turn compiler warnings to errors")
IF (UNIX OR APPLE)
    SET(COMP_FLAGS "")
    SET(LINKER_FLAGS "")
    # Verbose warnings and turns all to errors
    SET(COMP_FLAGS "${COMP_FLAGS} -Wall -Wextra")
    IF (FIX_WARNINGS)
        SET(COMP_FLAGS "${COMP_FLAGS} -Werror")
    ENDIF ()
    # Omit problematic warnings
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-but-set-variable")
    ENDIF ()
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 6.9.9)
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-format-truncation")
    ENDIF ()
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
        SET(COMP_FLAGS "${COMP_FLAGS} -Wno-nonnull -Wno-deprecated-declarations")
    ENDIF ()

    # Minimal debug info with Clang
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
        SET(COMP_FLAGS "${COMP_FLAGS} -gline-tables-only")
    ELSE ()
        SET(COMP_FLAGS "${COMP_FLAGS} -g")
    ENDIF ()

    # Note: The following flags are problematic on older systems with gcc 4.8
    IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9.9))
        IF ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
            SET(COMP_FLAGS "${COMP_FLAGS} -Wno-unused-command-line-argument")
        ENDIF ()
        FIND_PROGRAM(LDGOLD_FOUND ld.gold)
        SET(LDGOLD_SUPPORT OFF CACHE BOOL "Enable ld.gold support")
        # Optional ld.gold is 2x faster than normal ld
        IF (LDGOLD_FOUND AND LDGOLD_SUPPORT MATCHES ON AND NOT APPLE AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES arm)
            SET(LINKER_FLAGS "${LINKER_FLAGS} -fuse-ld=gold")
            # Use Identical Code Folding
            SET(COMP_FLAGS "${COMP_FLAGS} -ffunction-sections")
            SET(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--icf=safe")
            # Compress the debug sections
            # Note: Before valgrind 3.12.0, patch should be applied for valgrind (https://bugs.kde.org/show_bug.cgi?id=303877)
            IF (NOT APPLE AND NOT ANDROID AND NOT CMAKE_SYSTEM_PROCESSOR MATCHES arm AND NOT CMAKE_CXX_CLANG_TIDY)
                SET(COMP_FLAGS "${COMP_FLAGS} -Wa,--compress-debug-sections")
                SET(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--compress-debug-sections=zlib")
            ENDIF ()
        ENDIF ()
    ENDIF ()

    # Apply the flags
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMP_FLAGS}")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMP_FLAGS}")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${LINKER_FLAGS}")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${LINKER_FLAGS}")
ENDIF ()

# Sanitizer support
SET(CLANG_SANITIZERS OFF CACHE BOOL "Clang's sanitizer support")
IF (CLANG_SANITIZERS AND
    ((UNIX AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang") OR (APPLE AND "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")))
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address,undefined -fno-omit-frame-pointer")
ENDIF ()

# Unity Build support
include(UnityBuild)
