
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/src 
  ${STELLARIUM_BUILD_DIR}
  ${STELLARIUM_SOURCE_DIR}
  ${STELLARIUM_SOURCE_DIR}/core
  ${STELLARIUM_SOURCE_DIR}/gui
  ${STELLARIUM_SOURCE_DIR}/modules
  ${STELLARIUM_SOURCE_DIR}/stelutils
  ${STELLARIUM_SOURCE_DIR}/external
  ${STELLARIUM_SOURCE_DIR}/scripting
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/src 
  ${OPENGL_INCLUDE_DIR})

LINK_DIRECTORIES(${STELLARIUM_BUILD_DIR}/src)

SET(CompassMarks_SRCS
  CompassMarks.hpp
  CompassMarks.cpp)

SET(CompassMarks_RES
  ${CMAKE_SOURCE_DIR}/CompassMarks.qrc)
QT4_ADD_RESOURCES(CompassMarks_RES_CXX ${CompassMarks_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(CompassMarks_MOC_HDRS
 CompassMarks.hpp)
 
# After this call, CompassMarks_MOC_SRCS = moc_CompassMarks.cxx
QT4_WRAP_CPP(CompassMarks_MOC_SRCS ${CompassMarks_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_LIB)
	ADD_LIBRARY(CompassMarks MODULE ${CompassMarks_SRCS} ${CompassMarks_MOC_SRCS} ${CompassMarks_RES_CXX})
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
ENDIF(BUILD_DYNAMIC_LIB)

############### For building the static library ######################
IF(BUILD_STATIC_LIB)
	ADD_LIBRARY(CompassMarks-static STATIC ${CompassMarks_SRCS} ${CompassMarks_MOC_SRCS} ${CompassMarks_RES_CXX})
	# The library target "CompassMarks-static" has a default OUTPUT_NAME of "CompassMarks-static", so change it.
	SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES OUTPUT_NAME "CompassMarks")
	TARGET_LINK_LIBRARIES(CompassMarks-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(CompassMarks-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
ENDIF(BUILD_STATIC_LIB)
