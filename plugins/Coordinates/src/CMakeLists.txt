INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/Coordinates/src
  ${CMAKE_BINARY_DIR}/plugins/Coordinates/src/gui
  . gui)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(COORDINATES_SRCS
  Coordinates.hpp
  Coordinates.cpp
  gui/CoordinatesWindow.hpp
  gui/CoordinatesWindow.cpp
  )

SET(COORDINATES_UIS
  gui/coordinatesWindow.ui
)

QT5_WRAP_UI(COORDINATES_UIS_H ${COORDINATES_UIS})

################# compiles resources files ############
SET(COORDINATES_RES ../resources/Coordinates.qrc)
QT5_ADD_RESOURCES(COORDINATES_RES_CXX ${COORDINATES_RES})

SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(Coordinates-static STATIC ${COORDINATES_SRCS} ${COORDINATES_RES_CXX} ${COORDINATES_UIS_H})
QT5_USE_MODULES(Coordinates-static Core Declarative Network)
SET_TARGET_PROPERTIES(Coordinates-static PROPERTIES OUTPUT_NAME "Coordinates")
TARGET_LINK_LIBRARIES(Coordinates-static ${extLinkerOption})
SET_TARGET_PROPERTIES(Coordinates-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Coordinates-static)
