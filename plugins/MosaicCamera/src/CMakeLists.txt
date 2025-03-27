INCLUDE_DIRECTORIES(
    . 
    gui
    ${CMAKE_BINARY_DIR}/plugins/MosaicCamera/src
    ${CMAKE_BINARY_DIR}/plugins/MosaicCamera/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(MosaicCamera_SRCS
    MosaicCamera.hpp
    MosaicCamera.cpp
    gui/MosaicCameraDialog.hpp
    gui/MosaicCameraDialog.cpp
)

SET(MosaicCamera_UIS
    gui/mosaicCameraDialog.ui
)

################# compiles resources files ############
SET(MosaicCamera_RES ../resources/MosaicCamera.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
    QT5_WRAP_UI(MosaicCamera_UIS_H ${MosaicCamera_UIS})
    QT5_ADD_RESOURCES(MosaicCamera_RES_CXX ${MosaicCamera_RES})
ELSE()
    QT_WRAP_UI(MosaicCamera_UIS_H ${MosaicCamera_UIS})
    QT_ADD_RESOURCES(MosaicCamera_RES_CXX ${MosaicCamera_RES})
ENDIF()

SET(MosaicCamera_Qt_Libraries
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Widgets
)

ADD_LIBRARY(MosaicCamera-static STATIC ${MosaicCamera_SRCS} ${MosaicCamera_RES_CXX} ${MosaicCamera_UIS_H})
TARGET_LINK_LIBRARIES(MosaicCamera-static ${MosaicCamera_Qt_Libraries})
# The library target "MosaicCamera-static" has a default OUTPUT_NAME of "MosaicCamera-static", so change it.
SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES OUTPUT_NAME "MosaicCamera")
SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins MosaicCamera-static)

SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES FOLDER "plugins/MosaicCamera")
