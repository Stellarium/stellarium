INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/MeteorShowers/src
     ${CMAKE_BINARY_DIR}/plugins/MeteorShowers/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(MeteorShowers_SRCS
     MeteorShower.hpp
     MeteorShower.cpp
     MeteorShowers.hpp
     MeteorShowers.cpp
     MeteorShowersMgr.hpp
     MeteorShowersMgr.cpp
     MeteorObj.hpp
     MeteorObj.cpp
     gui/MSConfigDialog.hpp
     gui/MSConfigDialog.cpp
     gui/MSSearchDialog.hpp
     gui/MSSearchDialog.cpp
     translations.h
)

SET(MeteorShowersDialog_UIS
     gui/MSConfigDialog.ui
     gui/MSSearchDialog.ui
)

QT_WRAP_UI(MeteorShowersDialog_UIS_H ${MeteorShowersDialog_UIS})

SET(MeteorShowers_RES ../resources/MeteorShower.qrc)
QT_ADD_RESOURCES(MeteorShowers_RES_CXX ${MeteorShowers_RES})

ADD_LIBRARY(MeteorShowers-static STATIC ${MeteorShowers_SRCS} ${MeteorShowers_MOC_SRCS} ${MeteorShowers_RES_CXX} ${MeteorShowersDialog_UIS_H})
TARGET_LINK_LIBRARIES(MeteorShowers-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
SET_TARGET_PROPERTIES(MeteorShowers-static PROPERTIES OUTPUT_NAME "MeteorShowers")
SET_TARGET_PROPERTIES(MeteorShowers-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins MeteorShowers-static)

SET_TARGET_PROPERTIES(MeteorShowers-static PROPERTIES FOLDER "plugins/MeteorShowers")
