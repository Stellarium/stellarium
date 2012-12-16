
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src
  ${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src/gui
  . gui)

LINK_DIRECTORIES(/src)

SET(SolarSystemEditor_SRCS
  SolarSystemEditor.hpp
  SolarSystemEditor.cpp
  gui/SolarSystemManagerWindow.hpp
  gui/SolarSystemManagerWindow.cpp
  gui/MpcImportWindow.hpp
  gui/MpcImportWindow.cpp
  gui/ManualImportWindow.hpp
  gui/ManualImportWindow.cpp
  )

SET(SolarSystemEditor_UIS
  gui/solarSystemManagerWindow.ui
  gui/mpcImportWindow.ui
  gui/manualImportWindow.ui
)

QT4_WRAP_UI(SolarSystemEditor_UIS_H ${SolarSystemEditor_UIS})


SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

#Dynamic library
IF(BUILD_DYNAMIC_PLUGINS)
 ADD_LIBRARY(SolarSystemEditor MODULE ${SolarSystemEditor_SRCS} ${SolarSystemEditor_RES_CXX} ${SolarSystemEditor_UIS_H})

 IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(SolarSystemEditor PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
 ENDIF(APPLE)

 IF(WIN32)
  SET_TARGET_PROPERTIES(SolarSystemEditor PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
 ELSE(WIN32)
  SET(StelMain )
 ENDIF(WIN32)

 TARGET_LINK_LIBRARIES(SolarSystemEditor ${StelMain} ${extLinkerOption})

 INSTALL(TARGETS SolarSystemEditor DESTINATION "modules/SolarSystemEditor")
ENDIF()

#Static library
IF(BUILD_STATIC_PLUGINS)
 ADD_LIBRARY(SolarSystemEditor-static STATIC ${SolarSystemEditor_SRCS} ${SolarSystemEditor_RES_CXX} ${SolarSystemEditor_UIS_H})
 SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES OUTPUT_NAME "SolarSystemEditor")
 TARGET_LINK_LIBRARIES(SolarSystemEditor-static ${extLinkerOption})
 SET_TARGET_PROPERTIES(SolarSystemEditor-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
 ADD_DEPENDENCIES(AllStaticPlugins SolarSystemEditor-static)
ENDIF()
