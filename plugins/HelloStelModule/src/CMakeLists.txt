
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src
  ${STELLARIUM_BUILD_DIR}
  ${STELLARIUM_SOURCE_DIR}
  ${STELLARIUM_SOURCE_DIR}/core
  ${STELLARIUM_SOURCE_DIR}/modules
  ${STELLARIUM_SOURCE_DIR}/stelutils
  ${STELLARIUM_SOURCE_DIR}/external
  ${STELLARIUM_SOURCE_DIR}/scripting
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src
  ${OPENGL_INCLUDE_DIR})

LINK_DIRECTORIES(${STELLARIUM_BUILD_DIR}/src)

SET(HelloStelModule_SRCS
  HelloStelModule.hpp
  HelloStelModule.cpp)

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(HelloStelModule_MOC_HDRS
 HelloStelModule.hpp)

# After this call, HelloStelModule_MOC_SRCS = moc_HelloStelModule.cxx
QT4_WRAP_CPP(HelloStelModule_MOC_SRCS ${HelloStelModule_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_LIB)
	ADD_LIBRARY(HelloStelModule MODULE ${HelloStelModule_SRCS} ${HelloStelModule_MOC_SRCS})

	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(HelloStelModule PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)
	
	IF(WIN32)
		SET_TARGET_PROPERTIES(HelloStelModule PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)
	
	TARGET_LINK_LIBRARIES(HelloStelModule ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS HelloStelModule DESTINATION "modules/${PACKAGE}")
ENDIF(BUILD_DYNAMIC_LIB)

############### For building the static library ######################
IF(BUILD_STATIC_LIB)
	ADD_LIBRARY(HelloStelModule-static STATIC ${HelloStelModule_SRCS} ${HelloStelModule_MOC_SRCS})
	# The library target "HelloStelModule-static" has a default OUTPUT_NAME of "HelloStelModule-static", so change it.
	SET_TARGET_PROPERTIES(HelloStelModule-static PROPERTIES OUTPUT_NAME "HelloStelModule")
	TARGET_LINK_LIBRARIES(HelloStelModule-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(HelloStelModule-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
ENDIF(BUILD_STATIC_LIB)


