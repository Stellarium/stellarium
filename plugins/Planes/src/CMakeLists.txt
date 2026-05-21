INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_SOURCE_DIR}/src
     ${CMAKE_SOURCE_DIR}/src/gui
     ${CMAKE_BINARY_DIR}/plugins/Planes/src
     ${CMAKE_BINARY_DIR}/plugins/Planes/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Planes_SRCS
     AircraftObject.hpp
     AircraftObject.cpp
     AircraftRecord.hpp
     Planes.hpp
     Planes.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND Planes_SRCS
     gui/PlanesDialog.hpp
     gui/PlanesDialog.cpp
)

SET(Planes_UIS
     gui/planesDialog.ui
)
ENDIF()

SET(Planes_RES ../resources/Planes.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(Planes_UIS_H ${Planes_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(Planes_RES_CXX ${Planes_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(Planes_UIS_H ${Planes_UIS})
     ENDIF()
     QT_ADD_RESOURCES(Planes_RES_CXX ${Planes_RES})
ENDIF()

SET(Planes_Qt_Libraries
     Qt${QT_VERSION_MAJOR}::Core
     Qt${QT_VERSION_MAJOR}::Gui
     Qt${QT_VERSION_MAJOR}::Network
     Qt${QT_VERSION_MAJOR}::OpenGL
     Qt${QT_VERSION_MAJOR}::Widgets
)

ADD_LIBRARY(Planes-static STATIC ${Planes_SRCS} ${Planes_RES_CXX} ${Planes_UIS_H})
SET_TARGET_PROPERTIES(Planes-static PROPERTIES OUTPUT_NAME "Planes")
TARGET_LINK_LIBRARIES(Planes-static ${Planes_Qt_Libraries})
SET_TARGET_PROPERTIES(Planes-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Planes-static)

SET_TARGET_PROPERTIES(Planes-static PROPERTIES FOLDER "plugins/Planes")
