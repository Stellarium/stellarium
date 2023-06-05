
INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src
     ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src/gui
)

LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

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

SET(SolarSystemEditor_RES ../resources/SolarSystemEditor.qrc)

IF (${QT_VERSION_MAJOR} EQUAL "5")
     QT5_WRAP_UI(SolarSystemEditor_UIS_H ${SolarSystemEditor_UIS})
     QT5_ADD_RESOURCES(SolarSystemEditor_RES_CXX ${SolarSystemEditor_RES})
ELSE()
     QT_WRAP_UI(SolarSystemEditor_UIS_H ${SolarSystemEditor_UIS})
     QT_ADD_RESOURCES(SolarSystemEditor_RES_CXX ${SolarSystemEditor_RES})
ENDIF()

IF(ENABLE_TESTING)
     ADD_SUBDIRECTORY(test)
ENDIF(ENABLE_TESTING)

ADD_LIBRARY(SolarSystemEditor-static STATIC ${SolarSystemEditor_SRCS} ${SolarSystemEditor_RES_CXX} ${SolarSystemEditor_UIS_H})
TARGET_LINK_LIBRARIES(SolarSystemEditor-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets)
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES OUTPUT_NAME "SolarSystemEditor")
SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins SolarSystemEditor-static)

SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES FOLDER "plugins/SolarSystemEditor")
