INCLUDE_DIRECTORIES(.)
LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(AngleMeasure_SRCS
  AngleMeasure.hpp
  AngleMeasure.cpp)

################# compiles resources files ############
SET(AngleMeasure_RES ../AngleMeasure.qrc)
QT4_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(AngleMeasure_MOC_HDRS
 AngleMeasure.hpp)

# After this call, AngleMeasure_MOC_SRCS = moc_AngleMeasure.cxx
QT4_WRAP_CPP(AngleMeasure_MOC_SRCS ${AngleMeasure_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(AngleMeasure MODULE ${AngleMeasure_SRCS} ${AngleMeasure_MOC_SRCS} ${AngleMeasure_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(AngleMeasure PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(AngleMeasure PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(AngleMeasure ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS AngleMeasure DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_MOC_SRCS} ${AngleMeasure_RES_CXX})
	# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
	TARGET_LINK_LIBRARIES(AngleMeasure-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins AngleMeasure-static)
ENDIF()

