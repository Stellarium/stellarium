INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Pulsars/src
    ${CMAKE_BINARY_DIR}/plugins/Pulsars/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Pulsars_SRCS
  Pulsars.hpp
  Pulsars.cpp
  Pulsar.hpp
  Pulsar.cpp
  gui/PulsarsDialog.hpp
  gui/PulsarsDialog.cpp
)

SET(PulsarsDialog_UIS
  gui/pulsarsDialog.ui
)

QT5_WRAP_UI(PulsarsDialog_UIS_H ${PulsarsDialog_UIS})

SET(Pulsars_RES ../resources/Pulsars.qrc)
QT5_ADD_RESOURCES(Pulsars_RES_CXX ${Pulsars_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(Pulsars-static STATIC ${Pulsars_SRCS} ${Pulsars_RES_CXX} ${PulsarsDialog_UIS_H})
QT5_USE_MODULES(Pulsars-static Core Network Widgets)
SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES OUTPUT_NAME "Pulsars")
TARGET_LINK_LIBRARIES(Pulsars-static ${extLinkerOption})
SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Pulsars-static)
