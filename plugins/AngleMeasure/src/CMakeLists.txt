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

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_RES_CXX} ${AngleMeasure_UIS_H})
QT5_USE_MODULES(AngleMeasure-static Core Network Widgets)
# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
TARGET_LINK_LIBRARIES(AngleMeasure-static ${extLinkerOption})
SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins AngleMeasure-static)
