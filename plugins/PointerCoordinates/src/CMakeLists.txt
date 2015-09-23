INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/PointerCoordinates/src
  ${CMAKE_BINARY_DIR}/plugins/PointerCoordinates/src/gui
  . gui)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(POINTERCOORDINATES_SRCS
  PointerCoordinates.hpp
  PointerCoordinates.cpp
  gui/PointerCoordinatesWindow.hpp
  gui/PointerCoordinatesWindow.cpp
  )

SET(POINTERCOORDINATES_UIS
  gui/pointerCoordinatesWindow.ui
)

QT5_WRAP_UI(POINTERCOORDINATES_UIS_H ${POINTERCOORDINATES_UIS})

################# compiles resources files ############
SET(POINTERCOORDINATES_RES ../resources/PointerCoordinates.qrc)
QT5_ADD_RESOURCES(POINTERCOORDINATES_RES_CXX ${POINTERCOORDINATES_RES})

SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(PointerCoordinates-static STATIC ${POINTERCOORDINATES_SRCS} ${POINTERCOORDINATES_RES_CXX} ${POINTERCOORDINATES_UIS_H})
QT5_USE_MODULES(PointerCoordinates-static Core Network Widgets)
SET_TARGET_PROPERTIES(PointerCoordinates-static PROPERTIES OUTPUT_NAME "PointerCoordinates")
TARGET_LINK_LIBRARIES(PointerCoordinates-static ${extLinkerOption})
SET_TARGET_PROPERTIES(PointerCoordinates-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins PointerCoordinates-static)
