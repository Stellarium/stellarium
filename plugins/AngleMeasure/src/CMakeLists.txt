INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(AngleMeasure_SRCS
     AngleMeasure.hpp
     AngleMeasure.cpp
     gui/AngleMeasureDialog.hpp
     gui/AngleMeasureDialog.cpp
)

SET(AngleMeasure_UIS
     gui/angleMeasureDialog.ui
)

QT5_WRAP_UI(AngleMeasure_UIS_H ${AngleMeasure_UIS})

################# compiles resources files ############
SET(AngleMeasure_RES ../AngleMeasure.qrc)
QT5_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})

ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_RES_CXX} ${AngleMeasure_UIS_H})
TARGET_LINK_LIBRARIES(AngleMeasure-static Qt5::Core Qt5::Widgets)
# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins AngleMeasure-static)

SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES FOLDER "plugins/AngleMeasure")
