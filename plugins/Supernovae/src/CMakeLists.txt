INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Supernovae/src
     ${CMAKE_BINARY_DIR}/plugins/Supernovae/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(Supernovae_SRCS
     Supernovae.hpp
     Supernovae.cpp
     Supernova.hpp
     Supernova.cpp
)

IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND Supernovae_SRCS
     gui/SupernovaeDialog.hpp
     gui/SupernovaeDialog.cpp
)

SET(SupernovaeDialog_UIS
     gui/supernovaeDialog.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

SET(Supernovae_RES ../resources/Supernovae.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(SupernovaeDialog_UIS_H ${SupernovaeDialog_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(Supernovae_RES_CXX ${Supernovae_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(SupernovaeDialog_UIS_H ${SupernovaeDialog_UIS})
     ENDIF()
     QT_ADD_RESOURCES(Supernovae_RES_CXX ${Supernovae_RES})
ENDIF()

ADD_LIBRARY(Supernovae-static STATIC ${Supernovae_SRCS} ${Supernovae_RES_CXX} ${SupernovaeDialog_UIS_H})
TARGET_LINK_LIBRARIES(Supernovae-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)

IF(ENABLE_TESTING)
    ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES OUTPUT_NAME "Supernovae")
SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES FOLDER "plugins/Supernovae")

