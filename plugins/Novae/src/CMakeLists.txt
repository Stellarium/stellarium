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
     gui/NovaeDialog.hpp
     gui/NovaeDialog.cpp
)

SET(NovaeDialog_UIS
     gui/novaeDialog.ui
)

QT_WRAP_UI(NovaeDialog_UIS_H ${NovaeDialog_UIS})

SET(Novae_RES ../resources/Novae.qrc)
QT_ADD_RESOURCES(Novae_RES_CXX ${Novae_RES})

ADD_LIBRARY(Novae-static STATIC ${Novae_SRCS} ${Novae_RES_CXX} ${NovaeDialog_UIS_H})
TARGET_LINK_LIBRARIES(Novae-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
SET_TARGET_PROPERTIES(Novae-static PROPERTIES OUTPUT_NAME "Novae")
SET_TARGET_PROPERTIES(Novae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Novae-static)

SET_TARGET_PROPERTIES(Novae-static PROPERTIES FOLDER "plugins/Novae")
