# Centralized platform detection for platform-specific build configuration.
# The OpenHarmony toolchain identifies the target through CMAKE_SYSTEM_NAME.
SET(STELLARIUM_PLATFORM_OHOS FALSE)
IF(CMAKE_SYSTEM_NAME STREQUAL "OHOS")
     SET(STELLARIUM_PLATFORM_OHOS TRUE)
ENDIF()
