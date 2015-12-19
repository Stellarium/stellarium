
INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src/gui
)

LINK_DIRECTORIES(/src)

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


SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(SolarSystemEditor-static STATIC ${SolarSystemEditor_SRCS} ${SolarSystemEditor_RES_CXX} ${SolarSystemEditor_UIS_H})
QT5_USE_MODULES(SolarSystemEditor-static Core Network Widgets)
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES OUTPUT_NAME "SolarSystemEditor")
TARGET_LINK_LIBRARIES(SolarSystemEditor-static ${extLinkerOption})
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins SolarSystemEditor-static)
