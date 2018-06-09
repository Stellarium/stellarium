INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/PointerCoordinates/src
     ${CMAKE_BINARY_DIR}/plugins/PointerCoordinates/src/gui
)

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

ADD_LIBRARY(PointerCoordinates-static STATIC ${POINTERCOORDINATES_SRCS} ${POINTERCOORDINATES_RES_CXX} ${POINTERCOORDINATES_UIS_H})
TARGET_LINK_LIBRARIES(PointerCoordinates-static Qt5::Core Qt5::Widgets)
SET_TARGET_PROPERTIES(PointerCoordinates-static PROPERTIES OUTPUT_NAME "PointerCoordinates")
SET_TARGET_PROPERTIES(PointerCoordinates-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins PointerCoordinates-static)

SET_TARGET_PROPERTIES(PointerCoordinates-static PROPERTIES FOLDER "plugins/PointerCoordinates")

