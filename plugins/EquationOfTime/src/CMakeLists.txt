
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/EquationOfTime/src
  ${CMAKE_BINARY_DIR}/plugins/EquationOfTime/src/gui
  . gui)

LINK_DIRECTORIES(/src)

SET(EOT_SRCS
  EquationOfTime.hpp
  EquationOfTime.cpp
  gui/EquationOfTimeWindow.hpp
  gui/EquationOfTimeWindow.cpp
  )

SET(EOT_UIS
  gui/equationOfTimeWindow.ui
)

QT5_WRAP_UI(EOT_UIS_H ${EOT_UIS})

################# compiles resources files ############
SET(EOT_RES ../resources/EquationOfTime.qrc)
QT5_ADD_RESOURCES(EOT_RES_CXX ${EOT_RES})

SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(EquationOfTime-static STATIC ${EOT_SRCS} ${EOT_RES_CXX} ${EOT_UIS_H})
QT5_USE_MODULES(EquationOfTime-static Core Declarative Network)
SET_TARGET_PROPERTIES(EquationOfTime-static PROPERTIES OUTPUT_NAME "EquationOfTime")
TARGET_LINK_LIBRARIES(EquationOfTime-static ${extLinkerOption})
SET_TARGET_PROPERTIES(EquationOfTime-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins EquationOfTime-static)
