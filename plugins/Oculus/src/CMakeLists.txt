SET(OCULUS_SDK_PATH "" CACHE PATH "Path to Oculus SDK")


IF (IS_DIRECTORY ${OCULUS_SDK_PATH})

    MESSAGE(STATUS "Oculus SDK found.")

    INCLUDE_DIRECTORIES(${OCULUS_SDK_PATH}/LibOVR/Include
         .
         ${CMAKE_BINARY_DIR}/plugins/Oculus/src
    )

    LINK_DIRECTORIES(/src)

    SET(Oculus_SRCS
         Oculus.hpp
         Oculus.cpp
         ${OCULUS_SDK_PATH}/LibOVR/Shim/OVR_CAPI_Util.cpp
    )

    SET(extLinkerOption ${OPENGL_LIBRARIES})

    ADD_LIBRARY(Oculus-static STATIC ${Oculus_SRCS})
    SET_TARGET_PROPERTIES(Oculus-static PROPERTIES OUTPUT_NAME "Oculus")

    TARGET_LINK_LIBRARIES(Oculus-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
    TARGET_LINK_LIBRARIES(Oculus-static ${OCULUS_SDK_PATH}/LibOVR/Lib/Windows/x64/Release/VS2017/libOVR.lib ws2_32)

    SET_TARGET_PROPERTIES(Oculus-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
    ADD_DEPENDENCIES(AllStaticPlugins Oculus-static)
    SET_TARGET_PROPERTIES(Oculus-static PROPERTIES FOLDER "plugins/Oculus")

ELSE(IS_DIRECTORY ${OCULUS_SDK_PATH})
    MESSAGE(STATUS "Oculus SDK not found! Building without plugin. Set OCULUS_SDK_PATH in CMakeCache.txt!")

ENDIF (IS_DIRECTORY ${OCULUS_SDK_PATH})

