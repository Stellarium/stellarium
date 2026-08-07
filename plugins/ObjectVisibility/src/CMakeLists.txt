INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(ObjectVisibility_SRCS
     ObjectVisibility.hpp
     ObjectVisibility.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND ObjectVisibility_SRCS
     gui/ObjectVisibilityDialog.hpp
     gui/ObjectVisibilityDialog.cpp
     gui/ObjectVisibilityMapWidget.hpp
     gui/ObjectVisibilityMapWidget.cpp
)

SET(ObjectVisibility_UIS
     gui/objectVisibilityDialog.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

################# compiles resources files ############
SET(ObjectVisibility_RES ../ObjectVisibility.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(ObjectVisibility_UIS_H ${ObjectVisibility_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(ObjectVisibility_RES_CXX ${ObjectVisibility_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
           QT_WRAP_UI(ObjectVisibility_UIS_H ${ObjectVisibility_UIS})
     ENDIF()
     QT_ADD_RESOURCES(ObjectVisibility_RES_CXX ${ObjectVisibility_RES})
ENDIF()

ADD_LIBRARY(ObjectVisibility-static STATIC
            ${ObjectVisibility_SRCS}
            ${ObjectVisibility_RES_CXX}
            ${ObjectVisibility_UIS_H})

TARGET_LINK_LIBRARIES(ObjectVisibility-static
                      Qt${QT_VERSION_MAJOR}::Core
                      Qt${QT_VERSION_MAJOR}::Widgets
                      Qt${QT_VERSION_MAJOR}::OpenGL)

SET_TARGET_PROPERTIES(ObjectVisibility-static PROPERTIES
                      OUTPUT_NAME "ObjectVisibility")
SET_TARGET_PROPERTIES(ObjectVisibility-static PROPERTIES
                      COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins ObjectVisibility-static)

SET_TARGET_PROPERTIES(ObjectVisibility-static PROPERTIES
                      FOLDER "plugins/ObjectVisibility")
