
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/TimeZoneManager/src
  ${CMAKE_BINARY_DIR}/plugins/TimeZoneManager/src/gui
  . gui)

LINK_DIRECTORIES(/src)

SET(TimeZoneManager_SRCS
  TimeZoneManager.hpp
  TimeZoneManager.cpp
  gui/TimeZoneManagerWindow.hpp
  gui/TimeZoneManagerWindow.cpp
  gui/DefineTimeZoneWindow.hpp
  gui/DefineTimeZoneWindow.cpp
  )

SET(TimeZoneManager_UIS
  gui/timeZoneManagerWindow.ui
  gui/defineTimeZone.ui
)

QT4_WRAP_UI(TimeZoneManager_UIS_H ${TimeZoneManager_UIS})

#SET(TimeZoneManager_RES ../resources/resources.qrc)
#QT4_ADD_RESOURCES(TimeZoneManager_RES_CXX ${TimeZoneManager_RES})

SET(TimeZoneManager_MOC_HDRS
  gui/TimeZoneManagerWindow.hpp
  gui/DefineTimeZoneWindow.hpp
  TimeZoneManager.hpp
  )

QT4_WRAP_CPP(TimeZoneManager_MOC_SRCS ${TimeZoneManager_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

#Dynamic library
IF(BUILD_DYNAMIC_PLUGINS)
 ADD_LIBRARY(TimeZoneManager MODULE ${TimeZoneManager_SRCS} ${TimeZoneManager_MOC_SRCS} ${TimeZoneManager_RES_CXX} ${TimeZoneManager_UIS_H})

 IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(TimeZoneManager PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
 ENDIF(APPLE)

 IF(WIN32)
  SET_TARGET_PROPERTIES(TimeZoneManager PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
 ELSE(WIN32)
  SET(StelMain )
 ENDIF(WIN32)

 TARGET_LINK_LIBRARIES(TimeZoneManager ${StelMain} ${extLinkerOption})

 INSTALL(TARGETS TimeZoneManager DESTINATION "modules/TimeZoneManager")
ENDIF()

#Static library
IF(BUILD_STATIC_PLUGINS)
 ADD_LIBRARY(TimeZoneManager-static STATIC ${TimeZoneManager_SRCS} ${TimeZoneManager_MOC_SRCS} ${TimeZoneManager_RES_CXX} ${TimeZoneManager_UIS_H})
 SET_TARGET_PROPERTIES(TimeZoneManager-static PROPERTIES OUTPUT_NAME "TimeZoneManager")
 TARGET_LINK_LIBRARIES(TimeZoneManager-static ${extLinkerOption})
 IF(WIN32)
	 SET_TARGET_PROPERTIES(TimeZoneManager-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
 ELSE()
	 SET_TARGET_PROPERTIES(TimeZoneManager-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
 ENDIF()
 ADD_DEPENDENCIES(AllStaticPlugins TimeZoneManager-static)
ENDIF()
