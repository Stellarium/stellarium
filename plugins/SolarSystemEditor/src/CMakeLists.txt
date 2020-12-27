
INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(SolarSystemEditor_SRCS
     SolarSystemEditor.hpp
     SolarSystemEditor.cpp
     gui/SolarSystemManagerWindow.hpp
     gui/SolarSystemManagerWindow.cpp
     gui/MpcImportWindow.hpp
     gui/MpcImportWindow.cpp
     gui/ManualImportWindow.hpp
     gui/ManualImportWindow.cpp
)

SET(SolarSystemEditor_UIS
     gui/solarSystemManagerWindow.ui
     gui/mpcImportWindow.ui
     gui/manualImportWindow.ui
)

QT5_WRAP_UI(SolarSystemEditor_UIS_H ${SolarSystemEditor_UIS})

ADD_LIBRARY(SolarSystemEditor-static STATIC ${SolarSystemEditor_SRCS} ${SolarSystemEditor_RES_CXX} ${SolarSystemEditor_UIS_H})
TARGET_LINK_LIBRARIES(SolarSystemEditor-static Qt5::Core Qt5::Network Qt5::Widgets)
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES OUTPUT_NAME "SolarSystemEditor")
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins SolarSystemEditor-static)

SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES FOLDER "plugins/SolarSystemEditor")
