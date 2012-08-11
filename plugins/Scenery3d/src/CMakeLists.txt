INCLUDE_DIRECTORIES(
	.
	gui
	${CMAKE_BINARY_DIR}/plugins/Scenery3d/src
)

LINK_DIRECTORIES(/src)

SET(Scenery3d_SRCS
	AABB.hpp
	AABB.cpp
	Frustum.hpp
	Frustum.cpp
	Line.hpp
	Line.cpp
	Plane.hpp
	Plane.cpp
	SPolygon.hpp
	SPolygon.cpp
	Polyhedron.hpp
	Polyhedron.cpp
    Scenery3d.hpp
    Scenery3d.cpp
    Scenery3dMgr.hpp
    Scenery3dMgr.cpp
    gui/Scenery3dDialog.hpp
    gui/Scenery3dDialog.cpp
    OBJ.hpp
    OBJ.cpp
    Util.hpp
    Util.cpp
    Heightmap.hpp
    Heightmap.cpp
)

################# compiles resources files ############
SET(Scenery3d_RES ../Scenery3d.qrc)
QT4_ADD_RESOURCES(Scenery3d_RES_CXX ${Scenery3d_RES})

SET(Scenery3d_MOC_HDRS
    Scenery3dMgr.hpp
    gui/Scenery3dDialog.hpp
)

################# compiles .ui files ############
SET(Scenery3d_UIS
  gui/scenery3dDialog.ui
)
QT4_WRAP_UI(Scenery3d_UIS_H ${Scenery3d_UIS})

QT4_WRAP_CPP(Scenery3d_MOC_SRCS ${Scenery3d_MOC_HDRS})

INCLUDE(${QT_USE_FILE})

SET(extLinkerOption ${QT_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
        ADD_LIBRARY(Scenery3d
                MODULE ${Scenery3d_SRCS} ${Scenery3d_MOC_SRCS} ${Scenery3d_UIS_H} ${Scenery3d_RES_CXX}
	)

	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Scenery3d
			PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib"
		)
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(Scenery3d
			PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition"
		)
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain)
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Scenery3d ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Scenery3d DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Scenery3d-static
                STATIC ${Scenery3d_SRCS} ${Scenery3d_MOC_SRCS} ${Scenery3d_UIS_H} ${Scenery3d_RES_CXX}
	)
	SET_TARGET_PROPERTIES(Scenery3d-static PROPERTIES OUTPUT_NAME "Scenery3d")
	TARGET_LINK_LIBRARIES(Scenery3d-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(Scenery3d-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Scenery3d-static)
ENDIF()

