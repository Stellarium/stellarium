INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/MosaicCamera/src
     ${CMAKE_BINARY_DIR}/plugins/MosaicCamera/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(MosaicCamera_SRCS
     MosaicCamera.hpp
     MosaicCamera.cpp
     gui/MosaicCameraDialog.hpp
     gui/MosaicCameraDialog.cpp
)

SET(MosaicCameraDialog_UIS
     gui/mosaicCameraDialog.ui
)

SET(MosaicCamera_Qt_Libraries Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)

ADD_LIBRARY(MosaicCamera-static STATIC ${MosaicCamera_SRCS} ${MosaicCameraDialog_UIS_H})
# The library target "MosaicCamera-static" has a default OUTPUT_NAME of "MosaicCamera-static", so change it.
SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES OUTPUT_NAME "MosaicCamera")
TARGET_LINK_LIBRARIES(MosaicCamera-static ${StelMain} ${MosaicCamera_Qt_Libraries})
SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins MosaicCamera-static)

SET_TARGET_PROPERTIES(MosaicCamera-static PROPERTIES FOLDER "plugins/MosaicCamera")
