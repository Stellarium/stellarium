
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

SET(TextUserInterface_SRCS
    DummyDialog.hpp
    DummyDialog.cpp
    TuiNode.hpp
    TuiNode.cpp
    TuiNodeActivate.hpp
    TuiNodeActivate.cpp
    TuiNodeEditable.hpp
    TuiNodeEditable.cpp
    TuiNodeBool.hpp
    TuiNodeBool.cpp
    TuiNodeInt.hpp
    TuiNodeInt.cpp
    TuiNodeDouble.hpp
    TuiNodeDouble.cpp
    TuiNodeFloat.hpp
    TuiNodeFloat.cpp
    TuiNodeDateTime.hpp
    TuiNodeDateTime.cpp
    TuiNodeColor.hpp
    TuiNodeColor.cpp
    TuiNodeEnum.hpp
    TuiNodeEnum.cpp
    TextUserInterface.hpp
    TextUserInterface.cpp)

SET(TextUserInterface_RES
  ${CMAKE_SOURCE_DIR}/TextUserInterface.qrc)
QT4_ADD_RESOURCES(TextUserInterface_RES_CXX ${TextUserInterface_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(TextUserInterface_MOC_HDRS
    DummyDialog.hpp
    TuiNode.hpp
    TuiNodeActivate.hpp
    TuiNodeEditable.hpp
    TuiNodeBool.hpp
    TuiNodeInt.hpp
    TuiNodeDouble.hpp
    TuiNodeFloat.hpp
    TuiNodeDateTime.hpp
    TuiNodeColor.hpp
    TuiNodeEnum.hpp
    TextUserInterface.hpp)
 
# After this call, TextUserInterface_MOC_SRCS = moc_TextUserInterface.cxx
QT4_WRAP_CPP(TextUserInterface_MOC_SRCS ${TextUserInterface_MOC_HDRS})
SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_LIB)
	ADD_LIBRARY(TextUserInterface MODULE ${TextUserInterface_SRCS} ${TextUserInterface_MOC_SRCS} ${TextUserInterface_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(TextUserInterface PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF(APPLE)
	IF(WIN32)
		SET_TARGET_PROPERTIES(TextUserInterface PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)
	TARGET_LINK_LIBRARIES(TextUserInterface ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS TextUserInterface DESTINATION "modules/${PACKAGE}")
ENDIF(BUILD_DYNAMIC_LIB)

############### For building the static library ######################
IF(BUILD_STATIC_LIB)
	ADD_LIBRARY(TextUserInterface-static STATIC ${TextUserInterface_SRCS} ${TextUserInterface_MOC_SRCS} ${TextUserInterface_RES_CXX})
	# The library target "TextUserInterface-static" has a default OUTPUT_NAME of "TextUserInterface-static", so change it.
	SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES OUTPUT_NAME "TextUserInterface")
	TARGET_LINK_LIBRARIES(TextUserInterface-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
ENDIF(BUILD_STATIC_LIB)

