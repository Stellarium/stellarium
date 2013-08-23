INCLUDE_DIRECTORIES(
    .
    gui
    ${CMAKE_BINARY_DIR}/plugins/Novae/src
    ${CMAKE_BINARY_DIR}/plugins/Novae/src/gui
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Novae_SRCS
    Novae.hpp
    Novae.cpp
    Nova.hpp
    Nova.cpp
    gui/NovaeDialog.hpp
    gui/NovaeDialog.cpp
)

SET(NovaeDialog_UIS
    gui/novaeDialog.ui
)

QT4_WRAP_UI(NovaeDialog_UIS_H ${NovaeDialog_UIS})

SET(Novae_RES ../resources/Novae.qrc)
QT4_ADD_RESOURCES(Novae_RES_CXX ${Novae_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Novae MODULE ${Novae_SRCS} ${Novae_RES_CXX} ${NovaeDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Novae PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
                SET_TARGET_PROPERTIES(Novae PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Novae ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Novae DESTINATION "modules/Novae")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Novae-static STATIC ${Novae_SRCS} ${Novae_RES_CXX} ${NovaeDialog_UIS_H})
	SET_TARGET_PROPERTIES(Novae-static PROPERTIES OUTPUT_NAME "Novae")
	TARGET_LINK_LIBRARIES(Novae-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Novae-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Novae-static)
ENDIF()
