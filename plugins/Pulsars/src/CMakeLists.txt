INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Pulsars/src
    ${CMAKE_BINARY_DIR}/plugins/Pulsars/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Pulsars_SRCS
  Pulsars.hpp
  Pulsars.cpp
  Pulsar.hpp
  Pulsar.cpp
  gui/PulsarsDialog.hpp
  gui/PulsarsDialog.cpp
)

SET(PulsarsDialog_UIS
  gui/pulsarsDialog.ui
)

QT4_WRAP_UI(PulsarsDialog_UIS_H ${PulsarsDialog_UIS})

SET(Pulsars_RES ../resources/Pulsars.qrc)
QT4_ADD_RESOURCES(Pulsars_RES_CXX ${Pulsars_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Pulsars MODULE ${Pulsars_SRCS} ${Pulsars_RES_CXX} ${PulsarsDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Pulsars PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Pulsars PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Pulsars ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Pulsars DESTINATION "modules/Pulsars")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Pulsars-static STATIC ${Pulsars_SRCS} ${Pulsars_RES_CXX} ${PulsarsDialog_UIS_H})
	SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES OUTPUT_NAME "Pulsars")
	TARGET_LINK_LIBRARIES(Pulsars-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Pulsars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Pulsars-static)
ENDIF()
