INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Supernovae/src
    ${CMAKE_BINARY_DIR}/plugins/Supernovae/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Supernovae_SRCS
    Supernovae.hpp
    Supernovae.cpp
    Supernova.hpp
    Supernova.cpp
    gui/SupernovaeDialog.hpp
    gui/SupernovaeDialog.cpp
)

SET(SupernovaeDialog_UIS
    gui/supernovaeDialog.ui
)

QT4_WRAP_UI(SupernovaeDialog_UIS_H ${SupernovaeDialog_UIS})

SET(Supernovae_RES ../resources/Supernovae.qrc)
QT4_ADD_RESOURCES(Supernovae_RES_CXX ${Supernovae_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Supernovae MODULE ${Supernovae_SRCS} ${Supernovae_RES_CXX} ${SupernovaeDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Supernovae PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Supernovae PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Supernovae ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Supernovae DESTINATION "modules/Supernovae")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Supernovae-static STATIC ${Supernovae_SRCS} ${Supernovae_RES_CXX} ${SupernovaeDialog_UIS_H})
	SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES OUTPUT_NAME "Supernovae")
	TARGET_LINK_LIBRARIES(Supernovae-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Supernovae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Supernovae-static)
ENDIF()
