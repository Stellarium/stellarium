INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/MeteorShowers/src
    ${CMAKE_BINARY_DIR}/plugins/MeteorShowers/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(MeteorShowers_SRCS
    MeteorShower.hpp
    MeteorShower.cpp
    MeteorShowers.hpp
    MeteorShowers.cpp
    MeteorObj.hpp
    MeteorObj.cpp
    gui/MeteorShowerDialog.hpp
    gui/MeteorShowerDialog.cpp
)

SET(MeteorShowersDialog_UIS
    gui/meteorShowerDialog.ui
)

QT5_WRAP_UI(MeteorShowersDialog_UIS_H ${MeteorShowersDialog_UIS})

SET(MeteorShowers_RES ../resources/MeteorShower.qrc)
QT5_ADD_RESOURCES(MeteorShowers_RES_CXX ${MeteorShowers_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

ADD_LIBRARY(MeteorShowers-static STATIC ${MeteorShowers_SRCS} ${MeteorShowers_MOC_SRCS} ${MeteorShowers_RES_CXX} ${MeteorShowersDialog_UIS_H})
QT5_USE_MODULES(MeteorShowers-static Core Declarative Network)
SET_TARGET_PROPERTIES(MeteorShowers-static PROPERTIES OUTPUT_NAME "MeteorShowers")
TARGET_LINK_LIBRARIES(MeteorShowers-static ${extLinkerOption})
SET_TARGET_PROPERTIES(MeteorShowers-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins MeteorShowers-static)
