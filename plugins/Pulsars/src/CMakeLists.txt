INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Pulsars/src
     ${CMAKE_BINARY_DIR}/plugins/Pulsars/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

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

SET(Pulsars_RES ../resources/Pulsars.qrc)

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(PulsarsDialog_UIS_H ${PulsarsDialog_UIS})
     QT5_ADD_RESOURCES(Pulsars_RES_CXX ${Pulsars_RES})
ELSE()
     QT_WRAP_UI(PulsarsDialog_UIS_H ${PulsarsDialog_UIS})
     QT_ADD_RESOURCES(Pulsars_RES_CXX ${Pulsars_RES})
ENDIF()

ADD_LIBRARY(Pulsars-static STATIC ${Pulsars_SRCS} ${Pulsars_RES_CXX} ${PulsarsDialog_UIS_H})
TARGET_LINK_LIBRARIES(Pulsars-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES OUTPUT_NAME "Pulsars")
SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Pulsars-static)

SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES FOLDER "plugins/Pulsars")
