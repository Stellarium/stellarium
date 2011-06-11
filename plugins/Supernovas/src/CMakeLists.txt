INCLUDE_DIRECTORIES(.)

LINK_DIRECTORIES(${BUILD_DIR}/src)

SET(Supernovas_SRCS
  Supernovas.hpp
  Supernovas.cpp
  Supernova.hpp
  Supernova.cpp)

SET(Supernovas_RES ../resources/Supernovas.qrc)
QT4_ADD_RESOURCES(Supernovas_RES_CXX ${Supernovas_RES})

SET(Supernovas_MOC_HDRS
 Supernovas.hpp)

QT4_WRAP_CPP(Supernovas_MOC_SRCS ${Supernovas_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Supernovas MODULE ${Supernovas_SRCS} ${Supernovas_MOC_SRCS} ${Supernovas_RES_CXX})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Supernovas PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Supernovas PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Supernovas ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Supernovas DESTINATION "modules/Supernovas")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Supernovas-static STATIC ${Supernovas_SRCS} ${Supernovas_MOC_SRCS} ${Supernovas_RES_CXX})
	SET_TARGET_PROPERTIES(Supernovas-static PROPERTIES OUTPUT_NAME "Supernovas")
	TARGET_LINK_LIBRARIES(Supernovas-static ${extLinkerOption})
	SET_TARGET_PROPERTIES(Supernovas-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Supernovas-static)
ENDIF()
