
INCLUDE_DIRECTORIES(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_BINARY_DIR}/plugins/CometsAsteroidsImporter/src
  ${CMAKE_BINARY_DIR}/plugins/CometsAsteroidsImporter/src/gui
  . gui)

LINK_DIRECTORIES(/src)

SET(CAImporter_SRCS
  CAImporter.hpp
  CAImporter.cpp
  gui/SolarSystemManagerWindow.hpp
  gui/SolarSystemManagerWindow.cpp
  )

SET(CAImporter_UIS
  gui/solarSystemManagerWindow.ui
)

QT4_WRAP_UI(CAImporter_UIS_H ${CAImporter_UIS})

#SET(CAImporter_RES ../resources/resources.qrc)
#QT4_ADD_RESOURCES(CAImporter_RES_CXX ${CAImporter_RES})

SET(CAImporter_MOC_HDRS
  gui/SolarSystemManagerWindow.hpp
  CAImporter.hpp
  )

QT4_WRAP_CPP(CAImporter_MOC_SRCS ${CAImporter_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

#Dynamic library
IF(BUILD_DYNAMIC_PLUGINS)
 ADD_LIBRARY(CAImporter MODULE ${CAImporter_SRCS} ${CAImporter_MOC_SRCS} ${CAImporter_RES_CXX} ${CAImporter_UIS_H})

 IF(APPLE)
  FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
  MARK_AS_ADVANCED(OPENGL_LIBRARY)
  SET_TARGET_PROPERTIES(CAImporter PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
 ENDIF(APPLE)

 IF(WIN32)
  SET_TARGET_PROPERTIES(CAImporter PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
  SET(StelMain stelMain)
 ELSE(WIN32)
  SET(StelMain )
 ENDIF(WIN32)

 TARGET_LINK_LIBRARIES(CAImporter ${StelMain} ${extLinkerOption})

 INSTALL(TARGETS CAImporter DESTINATION "modules/CAImporter")
ENDIF()

#Static library
IF(BUILD_STATIC_PLUGINS)
 ADD_LIBRARY(CAImporter-static STATIC ${CAImporter_SRCS} ${CAImporter_MOC_SRCS} ${CAImporter_RES_CXX} ${CAImporter_UIS_H})
 SET_TARGET_PROPERTIES(CAImporter-static PROPERTIES OUTPUT_NAME "CAImporter")
 TARGET_LINK_LIBRARIES(CAImporter-static ${extLinkerOption})
 IF(WIN32)
	 SET_TARGET_PROPERTIES(CAImporter-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
 ELSE()
	 SET_TARGET_PROPERTIES(CAImporter-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
 ENDIF()
 ADD_DEPENDENCIES(AllStaticPlugins CAImporter-static)
ENDIF()
