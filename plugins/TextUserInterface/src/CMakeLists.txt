INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(/src)

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
SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(TextUserInterface MODULE ${TextUserInterface_SRCS} ${TextUserInterface_MOC_SRCS})
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
	INSTALL(TARGETS TextUserInterface DESTINATION "modules/TextUserInterface")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(TextUserInterface-static STATIC ${TextUserInterface_SRCS} ${TextUserInterface_MOC_SRCS})
	# The library target "TextUserInterface-static" has a default OUTPUT_NAME of "TextUserInterface-static", so change it.
	SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES OUTPUT_NAME "TextUserInterface")
	TARGET_LINK_LIBRARIES(TextUserInterface-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(TextUserInterface-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins TextUserInterface-static)
ENDIF()

