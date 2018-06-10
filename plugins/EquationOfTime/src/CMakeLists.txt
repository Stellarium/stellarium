INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/EquationOfTime/src
     ${CMAKE_BINARY_DIR}/plugins/EquationOfTime/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(EQUATIONOFTIME_SRCS
     EquationOfTime.hpp
     EquationOfTime.cpp
     gui/EquationOfTimeWindow.hpp
     gui/EquationOfTimeWindow.cpp
)

SET(EQUATIONOFTIME_UIS
     gui/equationOfTimeWindow.ui
)

QT5_WRAP_UI(EQUATIONOFTIME_UIS_H ${EQUATIONOFTIME_UIS})

################# compiles resources files ############
SET(EQUATIONOFTIME_RES ../resources/EquationOfTime.qrc)
QT5_ADD_RESOURCES(EQUATIONOFTIME_RES_CXX ${EQUATIONOFTIME_RES})

ADD_LIBRARY(EquationOfTime-static STATIC ${EQUATIONOFTIME_SRCS} ${EQUATIONOFTIME_RES_CXX} ${EQUATIONOFTIME_UIS_H})
TARGET_LINK_LIBRARIES(EquationOfTime-static Qt5::Core Qt5::Widgets)
SET_TARGET_PROPERTIES(EquationOfTime-static PROPERTIES OUTPUT_NAME "EquationOfTime")
SET_TARGET_PROPERTIES(EquationOfTime-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins EquationOfTime-static)

SET_TARGET_PROPERTIES(EquationOfTime-static PROPERTIES FOLDER "plugins/EquationOfTime")
