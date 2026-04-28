INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/TimeNavigator/src
     ${CMAKE_BINARY_DIR}/plugins/TimeNavigator/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(TIMENAVIGATOR_SRCS
     TimeNavigator.hpp
     TimeNavigator.cpp
     PlanetaryEventsMgr.hpp
     PlanetaryEventsMgr.cpp
     gui/TimeNavigatorDialog.hpp
     gui/TimeNavigatorDialog.cpp
)

SET(TIMENAVIGATOR_UIS
     gui/timeNavigatorDialog.ui
)

################# compiles resources files ############
SET(TIMENAVIGATOR_RES ../resources/TimeNavigator.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(TIMENAVIGATOR_UIS_H ${TIMENAVIGATOR_UIS})
     QT5_ADD_RESOURCES(TIMENAVIGATOR_RES_CXX ${TIMENAVIGATOR_RES})
ELSE()
     QT_WRAP_UI(TIMENAVIGATOR_UIS_H ${TIMENAVIGATOR_UIS})
     QT_ADD_RESOURCES(TIMENAVIGATOR_RES_CXX ${TIMENAVIGATOR_RES})
ENDIF()

ADD_LIBRARY(TimeNavigator-static STATIC ${TIMENAVIGATOR_SRCS} ${TIMENAVIGATOR_RES_CXX} ${TIMENAVIGATOR_UIS_H})
TARGET_LINK_LIBRARIES(TimeNavigator-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
SET_TARGET_PROPERTIES(TimeNavigator-static PROPERTIES OUTPUT_NAME "TimeNavigator")
SET_TARGET_PROPERTIES(TimeNavigator-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TimeNavigator-static)

SET_TARGET_PROPERTIES(TimeNavigator-static PROPERTIES FOLDER "plugins/TimeNavigator")
