INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Novae/src
     ${CMAKE_BINARY_DIR}/plugins/Novae/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Novae_SRCS
     Novae.hpp
     Novae.cpp
     Nova.hpp
     Nova.cpp
)

IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND Novae_SRCS
     gui/NovaeDialog.hpp
     gui/NovaeDialog.cpp
)

SET(NovaeDialog_UIS
     gui/novaeDialog.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

SET(Novae_RES ../resources/Novae.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(NovaeDialog_UIS_H ${NovaeDialog_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(Novae_RES_CXX ${Novae_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(NovaeDialog_UIS_H ${NovaeDialog_UIS})
     ENDIF()
     QT_ADD_RESOURCES(Novae_RES_CXX ${Novae_RES})
ENDIF()

ADD_LIBRARY(Novae-static STATIC ${Novae_SRCS} ${Novae_RES_CXX} ${NovaeDialog_UIS_H})
TARGET_LINK_LIBRARIES(Novae-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
SET_TARGET_PROPERTIES(Novae-static PROPERTIES OUTPUT_NAME "Novae")
SET_TARGET_PROPERTIES(Novae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Novae-static)

SET_TARGET_PROPERTIES(Novae-static PROPERTIES FOLDER "plugins/Novae")
