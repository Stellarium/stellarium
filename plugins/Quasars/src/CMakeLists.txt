INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Quasars/src
    ${CMAKE_BINARY_DIR}/plugins/Quasars/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Quasars_SRCS
    Quasars.hpp
    Quasars.cpp
    Quasar.hpp
    Quasar.cpp
    gui/QuasarsDialog.hpp
    gui/QuasarsDialog.cpp
)

SET(QuasarsDialog_UIS
    gui/quasarsDialog.ui
)

QT4_WRAP_UI(QuasarsDialog_UIS_H ${QuasarsDialog_UIS})

SET(Quasars_RES ../resources/Quasars.qrc)
QT4_ADD_RESOURCES(Quasars_RES_CXX ${Quasars_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Quasars MODULE ${Quasars_SRCS} ${Quasars_RES_CXX} ${QuasarsDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Quasars PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Quasars PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Quasars ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Quasars DESTINATION "modules/Quasars")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Quasars-static STATIC ${Quasars_SRCS} ${Quasars_RES_CXX} ${QuasarsDialog_UIS_H})
	SET_TARGET_PROPERTIES(Quasars-static PROPERTIES OUTPUT_NAME "Quasars")
	TARGET_LINK_LIBRARIES(Quasars-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Quasars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Quasars-static)
ENDIF()
