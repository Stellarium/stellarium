INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}/plugins/Exoplanets/src
     ${CMAKE_BINARY_DIR}/plugins/Exoplanets/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Exoplanets_SRCS
     Exoplanets.hpp
     Exoplanets.cpp
     Exoplanet.hpp
     Exoplanet.cpp
     gui/ExoplanetsDialog.hpp
     gui/ExoplanetsDialog.cpp
)

SET(ExoplanetsDialog_UIS
     gui/exoplanetsDialog.ui
)
QT5_WRAP_UI(ExoplanetsDialog_UIS_H ${ExoplanetsDialog_UIS})

SET(Exoplanets_RES ../resources/Exoplanets.qrc)
QT5_ADD_RESOURCES(Exoplanets_RES_CXX ${Exoplanets_RES})

ADD_LIBRARY(Exoplanets-static STATIC ${Exoplanets_SRCS} ${Exoplanets_RES_CXX} ${ExoplanetsDialog_UIS_H})
TARGET_LINK_LIBRARIES(Exoplanets-static Qt5::Core Qt5::Network Qt5::Widgets qcustomplot_stel)
SET_TARGET_PROPERTIES(Exoplanets-static PROPERTIES OUTPUT_NAME "Exoplanets")
SET_TARGET_PROPERTIES(Exoplanets-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins Exoplanets-static)

SET_TARGET_PROPERTIES(Exoplanets-static PROPERTIES FOLDER "plugins/Exoplanets")

