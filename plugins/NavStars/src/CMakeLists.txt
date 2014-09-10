INCLUDE_DIRECTORIES(
    .
    ${CMAKE_BINARY_DIR}/plugins/NavStars/src
)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(NavStars_SRCS
  NavStars.hpp
  NavStars.cpp
)

SET(NavStars_RES ../resources/NavStars.qrc)
QT4_ADD_RESOURCES(NavStars_RES_CXX ${NavStars_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(NavStars MODULE ${NavStars_SRCS} ${NavStars_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(NavStars PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
        SET_TARGET_PROPERTIES(NavStars PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(NavStars ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS NavStars DESTINATION "modules/NavStars")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(NavStars-static STATIC ${NavStars_SRCS} ${NavStars_RES_CXX})
	SET_TARGET_PROPERTIES(NavStars-static PROPERTIES OUTPUT_NAME "NavStars")
	TARGET_LINK_LIBRARIES(NavStars-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(NavStars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins NavStars-static)
ENDIF()
