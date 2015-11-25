
INCLUDE_DIRECTORIES(
     .
     gui
     ${CMAKE_BINARY_DIR}
     ${CMAKE_BINARY_DIR}/plugins/TimeZoneConfiguration/src
     ${CMAKE_BINARY_DIR}/plugins/TimeZoneConfiguration/src/gui
)

LINK_DIRECTORIES(/src)

SET(TimeZoneConfiguration_SRCS
     TimeZoneConfiguration.hpp
     TimeZoneConfiguration.cpp
     gui/TimeZoneConfigurationWindow.hpp
     gui/TimeZoneConfigurationWindow.cpp
     gui/DefineTimeZoneWindow.hpp
     gui/DefineTimeZoneWindow.cpp
)

SET(TimeZoneConfiguration_UIS
     gui/timeZoneConfigurationWindow.ui
     gui/defineTimeZone.ui
)

QT5_WRAP_UI(TimeZoneConfiguration_UIS_H ${TimeZoneConfiguration_UIS})

SET(extLinkerOption ${OPENGL_LIBRARIES})

ADD_LIBRARY(TimeZoneConfiguration-static STATIC ${TimeZoneConfiguration_SRCS} ${TimeZoneConfiguration_RES_CXX} ${TimeZoneConfiguration_UIS_H})
QT5_USE_MODULES(TimeZoneConfiguration-static Core Network Gui Widgets)
SET_TARGET_PROPERTIES(TimeZoneConfiguration-static PROPERTIES OUTPUT_NAME "TimeZoneConfiguration")
TARGET_LINK_LIBRARIES(TimeZoneConfiguration-static ${extLinkerOption})
SET_TARGET_PROPERTIES(TimeZoneConfiguration-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
ADD_DEPENDENCIES(AllStaticPlugins TimeZoneConfiguration-static)
