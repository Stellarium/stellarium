INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Exoplanets/src
    ${CMAKE_BINARY_DIR}/plugins/Exoplanets/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Exoplanets_SRCS
  Exoplanets.hpp
  Exoplanets.cpp
  Exoplanet.hpp
  Exoplanet.cpp
  gui/ExoplanetsDialog.hpp
  gui/ExoplanetsDialog.cpp
)

SET(ExoplanetsDialog_UIS
  gui/exoplanetsDialog.ui
)
QT4_WRAP_UI(ExoplanetsDialog_UIS_H ${ExoplanetsDialog_UIS})

SET(Exoplanets_RES ../resources/Exoplanets.qrc)
QT4_ADD_RESOURCES(Exoplanets_RES_CXX ${Exoplanets_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Exoplanets MODULE ${Exoplanets_SRCS} ${Exoplanets_RES_CXX} ${ExoplanetsDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Exoplanets PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Exoplanets PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Exoplanets ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Exoplanets DESTINATION "modules/Exoplanets")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Exoplanets-static STATIC ${Exoplanets_SRCS} ${Exoplanets_RES_CXX} ${ExoplanetsDialog_UIS_H})
	SET_TARGET_PROPERTIES(Exoplanets-static PROPERTIES OUTPUT_NAME "Exoplanets")
	TARGET_LINK_LIBRARIES(Exoplanets-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Exoplanets-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Exoplanets-static)
ENDIF()
