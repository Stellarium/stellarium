
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src 
  ${STELLARIUM_BUILD_DIR}
  ${STELLARIUM_SOURCE_DIR}
  ${STELLARIUM_SOURCE_DIR}/core
  ${STELLARIUM_SOURCE_DIR}/modules
  ${STELLARIUM_SOURCE_DIR}/stelutils
  ${STELLARIUM_SOURCE_DIR}/external
  ${STELLARIUM_SOURCE_DIR}/scripting
  ${STELLARIUM_SOURCE_DIR}/gui
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src 
  ${OPENGL_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS})

LINK_DIRECTORIES(${STELLARIUM_BUILD_DIR}/src)

SET(AngleMeasure_SRCS
  AngleMeasure.hpp
  AngleMeasure.cpp)

################# compiles resources files ############
SET(AngleMeasure_RES
  ${CMAKE_SOURCE_DIR}/AngleMeasure.qrc)
QT4_ADD_RESOURCES(AngleMeasure_RES_CXX ${AngleMeasure_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(AngleMeasure_MOC_HDRS
 AngleMeasure.hpp)
 
# After this call, AngleMeasure_MOC_SRCS = moc_AngleMeasure.cxx
QT4_WRAP_CPP(AngleMeasure_MOC_SRCS ${AngleMeasure_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_LIB)
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
ENDIF(BUILD_DYNAMIC_LIB)

############### For building the static library ######################
IF(BUILD_STATIC_LIB)
	ADD_LIBRARY(AngleMeasure-static STATIC ${AngleMeasure_SRCS} ${AngleMeasure_MOC_SRCS} ${AngleMeasure_RES_CXX})
	# The library target "AngleMeasure-static" has a default OUTPUT_NAME of "AngleMeasure-static", so change it.
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES OUTPUT_NAME "AngleMeasure")
	TARGET_LINK_LIBRARIES(AngleMeasure-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(AngleMeasure-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
ENDIF(BUILD_STATIC_LIB)

