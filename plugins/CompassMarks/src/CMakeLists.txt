INCLUDE_DIRECTORIES(.)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(CompassMarks_SRCS
     CompassMarks.hpp
     CompassMarks.cpp
)

SET(CompassMarks_RES ../CompassMarks.qrc)
QT5_ADD_RESOURCES(CompassMarks_RES_CXX ${CompassMarks_RES})
SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(CompassMarks-static STATIC ${CompassMarks_SRCS} ${CompassMarks_RES_CXX})
QT5_USE_MODULES(CompassMarks-static Core Network Widgets)
# The library target "CompassMarks-static" has a default OUTPUT_NAME of "CompassMarks-static", so change it.
SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES OUTPUT_NAME "CompassMarks")
TARGET_LINK_LIBRARIES(CompassMarks-static ${extLinkerOption})
SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins CompassMarks-static)
