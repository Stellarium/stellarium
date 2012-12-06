INCLUDE_DIRECTORIES(.)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(CompassMarks_SRCS
  CompassMarks.hpp
  CompassMarks.cpp)

SET(CompassMarks_RES ../CompassMarks.qrc)
QT4_ADD_RESOURCES(CompassMarks_RES_CXX ${CompassMarks_RES})


SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(CompassMarks MODULE ${CompassMarks_SRCS} ${CompassMarks_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(CompassMarks PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(CompassMarks PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(CompassMarks ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS CompassMarks DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(CompassMarks-static STATIC ${CompassMarks_SRCS} ${CompassMarks_RES_CXX})
	# The library target "CompassMarks-static" has a default OUTPUT_NAME of "CompassMarks-static", so change it.
	SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES OUTPUT_NAME "CompassMarks")
	TARGET_LINK_LIBRARIES(CompassMarks-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins CompassMarks-static)
ENDIF()
