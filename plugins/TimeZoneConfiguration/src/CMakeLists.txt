
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/TimeZoneConfiguration/src
  ${CMAKE_BINARY_DIR}/plugins/TimeZoneConfiguration/src/gui
  . gui)

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

QT4_WRAP_UI(TimeZoneConfiguration_UIS_H ${TimeZoneConfiguration_UIS})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

#Dynamic library
IF(BUILD_DYNAMIC_PLUGINS)
 ADD_LIBRARY(TimeZoneConfiguration MODULE ${TimeZoneConfiguration_SRCS} ${TimeZoneConfiguration_RES_CXX} ${TimeZoneConfiguration_UIS_H})

 IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(TimeZoneConfiguration PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
 ENDIF(APPLE)

 IF(WIN32)
  SET_TARGET_PROPERTIES(TimeZoneConfiguration PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
 ELSE(WIN32)
  SET(StelMain )
 ENDIF(WIN32)

 TARGET_LINK_LIBRARIES(TimeZoneConfiguration ${StelMain} ${extLinkerOption})

 INSTALL(TARGETS TimeZoneConfiguration DESTINATION "modules/TimeZoneConfiguration")
ENDIF()

#Static library
IF(BUILD_STATIC_PLUGINS)
 ADD_LIBRARY(TimeZoneConfiguration-static STATIC ${TimeZoneConfiguration_SRCS} ${TimeZoneConfiguration_RES_CXX} ${TimeZoneConfiguration_UIS_H})
 SET_TARGET_PROPERTIES(TimeZoneConfiguration-static PROPERTIES OUTPUT_NAME "TimeZoneConfiguration")
 TARGET_LINK_LIBRARIES(TimeZoneConfiguration-static ${extLinkerOption})
 SET_TARGET_PROPERTIES(TimeZoneConfiguration-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
 ADD_DEPENDENCIES(AllStaticPlugins TimeZoneConfiguration-static)
ENDIF()
