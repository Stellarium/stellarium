INCLUDE_DIRECTORIES(.)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(RendererStatistics_SRCS
  RendererStatistics.hpp
  RendererStatistics.cpp)

SET(RendererStatistics_RES ../resources/RendererStatistics.qrc)
QT4_ADD_RESOURCES(RendererStatistics_RES_CXX ${RendererStatistics_RES})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(RendererStatistics MODULE ${RendererStatistics_SRCS} ${RendererStatistics_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(RendererStatistics PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(RendererStatistics PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(RendererStatistics ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS RendererStatistics DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(RendererStatistics-static STATIC ${RendererStatistics_SRCS} ${RendererStatistics_RES_CXX})
	SET_TARGET_PROPERTIES(RendererStatistics-static PROPERTIES OUTPUT_NAME "RendererStatistics")
	TARGET_LINK_LIBRARIES(RendererStatistics-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(RendererStatistics-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins RendererStatistics-static)
ENDIF()
