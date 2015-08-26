INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Supernovae/src
    ${CMAKE_BINARY_DIR}/plugins/Supernovae/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Supernovae_SRCS
    Supernovae.hpp
    Supernovae.cpp
    Supernova.hpp
    Supernova.cpp
    gui/SupernovaeDialog.hpp
    gui/SupernovaeDialog.cpp
)

SET(SupernovaeDialog_UIS
    gui/supernovaeDialog.ui
)

QT5_WRAP_UI(SupernovaeDialog_UIS_H ${SupernovaeDialog_UIS})

SET(Supernovae_RES ../resources/Supernovae.qrc)
QT5_ADD_RESOURCES(Supernovae_RES_CXX ${Supernovae_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(Supernovae-static STATIC ${Supernovae_SRCS} ${Supernovae_RES_CXX} ${SupernovaeDialog_UIS_H})
QT5_USE_MODULES(Supernovae-static Core Network Widgets)
SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES OUTPUT_NAME "Supernovae")
TARGET_LINK_LIBRARIES(Supernovae-static ${extLinkerOption})
SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Supernovae-static)
