
INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/DistanceCalculator/src
     ${CMAKE_BINARY_DIR}/plugins/DistanceCalculator/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(DISTANCECALCULATOR_SRCS
     DistanceCalculator.hpp
     DistanceCalculator.cpp
     gui/DistanceCalculatorWindow.hpp
     gui/DistanceCalculatorWindow.cpp
)

SET(DISTANCECALCULATOR_UIS
     gui/distanceCalculatorWindow.ui
)

QT5_WRAP_UI(DISTANCECALCULATOR_UIS_H ${DISTANCECALCULATOR_UIS})

ADD_LIBRARY(DistanceCalculator-static STATIC ${DISTANCECALCULATOR_SRCS} ${DISTANCECALCULATOR_RES_CXX} ${DISTANCECALCULATOR_UIS_H})
TARGET_LINK_LIBRARIES(DistanceCalculator-static Qt5::Core Qt5::Widgets)
SET_TARGET_PROPERTIES(DistanceCalculator-static PROPERTIES OUTPUT_NAME "DistanceCalculator")
SET_TARGET_PROPERTIES(DistanceCalculator-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins DistanceCalculator-static)
