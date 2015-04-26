INCLUDE_DIRECTORIES(. gui)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(RemoteControl_SRCS
  RemoteControl.hpp
  RemoteControl.cpp
  gui/RemoteControlDialog.hpp
  gui/RemoteControlDialog.cpp
)

SET(RemoteControl_UIS
  gui/remoteControlDialog.ui
)

QT5_WRAP_UI(RemoteControl_UIS_H ${RemoteControl_UIS})

################# compiles resources files ############
SET(RemoteControl_RES ../RemoteControl.qrc)
QT5_ADD_RESOURCES(RemoteControl_RES_CXX ${RemoteControl_RES})

SET(extLinkerOption ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES})

ADD_LIBRARY(RemoteControl-static STATIC ${RemoteControl_SRCS} ${RemoteControl_RES_CXX} ${RemoteControl_UIS_H})
QT5_USE_MODULES(RemoteControl-static Core Network Widgets)
# The library target "RemoteControl-static" has a default OUTPUT_NAME of "RemoteControl-static", so change it.
SET_TARGET_PROPERTIES(RemoteControl-static PROPERTIES OUTPUT_NAME "RemoteControl")
TARGET_LINK_LIBRARIES(RemoteControl-static ${extLinkerOption})
SET_TARGET_PROPERTIES(RemoteControl-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins RemoteControl-static)
