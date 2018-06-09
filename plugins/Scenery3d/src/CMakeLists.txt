INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Scenery3d/src
     ${CMAKE_BINARY_DIR}/plugins/Scenery3d/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Scenery3d_SRCS
     Frustum.hpp
     Frustum.cpp
     GLFuncs.hpp
     Plane.hpp
     Plane.cpp
     SPolygon.hpp
     SPolygon.cpp
     Polyhedron.hpp
     Polyhedron.cpp
     S3DEnum.hpp
     SceneInfo.hpp
     SceneInfo.cpp
     S3DScene.hpp
     S3DScene.cpp
     Scenery3d.hpp
     Scenery3d.cpp
     Scenery3dRemoteControlService.hpp
     Scenery3dRemoteControlService.cpp
     S3DRenderer.hpp
     S3DRenderer.cpp
     gui/Scenery3dDialog.hpp
     gui/Scenery3dDialog_p.hpp
     gui/Scenery3dDialog.cpp
     gui/StoredViewDialog.hpp
     gui/StoredViewDialog_p.hpp
     gui/StoredViewDialog.cpp
     ShaderManager.hpp
     ShaderManager.cpp
     Heightmap.hpp
     Heightmap.cpp
)

# CMake < 3.0 does not AUTOMOC Q_GADGET which S3DEnum.hpp uses, so we have to manually add it
# Wrap it in an IF to prevent some linker warnings about symbols defined twice (on MSVC13 at least)
IF("${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION}" LESS 3.0)
     QT5_WRAP_CPP(Scenery3d_MOC_SRCS S3DEnum.hpp)
ENDIF()


################# compiles resources files ############
SET(Scenery3d_RES ../resources/Scenery3d.qrc)
QT5_ADD_RESOURCES(Scenery3d_RES_CXX ${Scenery3d_RES})

################# compiles .ui files ############
SET(Scenery3d_UIS
     gui/scenery3dDialog.ui
     gui/storedViewDialog.ui
)
QT5_WRAP_UI(Scenery3d_UIS_H ${Scenery3d_UIS})

############### For building the static library ######################
ADD_LIBRARY(Scenery3d-static STATIC ${Scenery3d_SRCS} ${Scenery3d_RES_CXX} ${Scenery3d_UIS_H} ${Scenery3d_MOC_SRCS} )
TARGET_LINK_LIBRARIES(Scenery3d-static Qt5::Core Qt5::Concurrent Qt5::Gui ${STEL_GLES_LIBS} Qt5::Widgets)
SET_TARGET_PROPERTIES(Scenery3d-static PROPERTIES OUTPUT_NAME "Scenery3d")
SET_TARGET_PROPERTIES(Scenery3d-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Scenery3d-static)

SET_TARGET_PROPERTIES(Scenery3d-static PROPERTIES FOLDER "plugins/Scenery3d")
