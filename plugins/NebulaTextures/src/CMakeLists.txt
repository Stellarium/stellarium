INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/NebulaTextures/src
    ${CMAKE_BINARY_DIR}/plugins/NebulaTextures/src/gui
)

# LINK_DIRECTORIES(${BUILD_DIR}/src)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(NebulaTextures_SRCS
     NebulaTextures.hpp
     NebulaTextures.cpp
     gui/NebulaTexturesDialog.hpp
     gui/NebulaTexturesDialog.cpp
)

SET(NebulaTextures_UIS
     gui/nebulaTexturesDialog.ui
)

################# compiles resources files ############
SET(NebulaTextures_RES ../resources/NebulaTextures.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(NebulaTextures_UIS_H ${NebulaTextures_UIS})
     QT5_ADD_RESOURCES(NebulaTextures_RES_CXX ${NebulaTextures_RES})
ELSE()
     QT_WRAP_UI(NebulaTextures_UIS_H ${NebulaTextures_UIS})
     QT_ADD_RESOURCES(NebulaTextures_RES_CXX ${NebulaTextures_RES})
ENDIF()

ADD_LIBRARY(NebulaTextures-static STATIC ${NebulaTextures_SRCS} ${NebulaTextures_RES_CXX} ${NebulaTextures_UIS_H})

# The library target "NebulaTextures-static" has a default OUTPUT_NAME of "NebulaTextures-static", so change it.
TARGET_LINK_LIBRARIES(NebulaTextures-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets) # Gui
SET_TARGET_PROPERTIES(NebulaTextures-static PROPERTIES OUTPUT_NAME "NebulaTextures")
SET_TARGET_PROPERTIES(NebulaTextures-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins NebulaTextures-static)

SET_TARGET_PROPERTIES(NebulaTextures-static PROPERTIES FOLDER "plugins/NebulaTextures")
