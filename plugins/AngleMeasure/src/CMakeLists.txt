INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${CMAKE_BINARY_DIR}/src)

SET(AngleMeasure_SRCS
     AngleMeasure.hpp
     AngleMeasure.cpp
)
IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
LIST(APPEND AngleMeasure_SRCS
     gui/AngleMeasureDialog.hpp
     gui/AngleMeasureDialog.cpp
)

SET(AngleMeasure_UIS
     gui/angleMeasureDialog.ui
)
ENDIF (STELLARIUM_GUI_MODE STREQUAL "Standard")

################# compiles resources files ############
SET(AngleMeasure_RES ../AngleMeasure.qrc)
IF (${QT_VERSION_MAJOR} EQUAL "5")
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT5_WRAP_UI(AngleMeasure_UIS_H ${AngleMeasure_UIS})
     ENDIF()
     QT5_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})
ELSE()
     IF (STELLARIUM_GUI_MODE STREQUAL "Standard")
          QT_WRAP_UI(AngleMeasure_UIS_H ${AngleMeasure_UIS})
     ENDIF()
     QT_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})
ENDIF()

ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_RES_CXX} ${AngleMeasure_UIS_H})
TARGET_LINK_LIBRARIES(AngleMeasure-static Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::OpenGL)
# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins AngleMeasure-static)

SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES FOLDER "plugins/AngleMeasure")
