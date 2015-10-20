INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(HelloStelModule_SRCS
     HelloStelModule.hpp
     HelloStelModule.cpp
)


SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(HelloStelModule-static STATIC ${HelloStelModule_SRCS})
# The library target "HelloStelModule-static" has a default OUTPUT_NAME of "HelloStelModule-static", so change it.
QT5_USE_MODULES(HelloStelModule-static Core Gui)
SET_TARGET_PROPERTIES(HelloStelModule-static PROPERTIES OUTPUT_NAME "HelloStelModule")
TARGET_LINK_LIBRARIES(HelloStelModule-static ${extLinkerOption})
SET_TARGET_PROPERTIES(HelloStelModule-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins HelloStelModule-static)

