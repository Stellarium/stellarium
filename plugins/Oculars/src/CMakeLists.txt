INCLUDE_DIRECTORIES(
	.
	gui
	${CMAKE_BINARY_DIR}/plugins/Oculars/src
)

LINK_DIRECTORIES(/src)

SET(Oculars_SRCS
  CCD.hpp
  CCD.cpp
  Lens.hpp
  Lens.cpp
  Ocular.hpp
  Ocular.cpp
  Oculars.hpp
  Oculars.cpp
  Telescope.hpp
  Telescope.cpp
  gui/OcularDialog.hpp
  gui/OcularDialog.cpp
  gui/PropertyBasedTableModel.hpp
  gui/PropertyBasedTableModel.cpp
  gui/OcularsGuiPanel.hpp
  gui/OcularsGuiPanel.cpp
)

SET(Oculars_RES ../resources/Oculars.qrc)
QT5_ADD_RESOURCES(Oculars_RES_CXX ${Oculars_RES})

################# compiles .ui files ############
SET(Oculars_UIS
  gui/ocularDialog.ui
)
QT5_WRAP_UI(Oculars_UIS_H ${Oculars_UIS})

SET(extLinkerOption ${OPENGL_LIBRARIES})


get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
 message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Oculars
		MODULE ${Oculars_SRCS} ${Oculars_RES_CXX} ${Oculars_UIS_H}
	)

	IF(APPLE)
		SET_TARGET_PROPERTIES(Oculars
			PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib"
		)
	ENDIF(APPLE)

	IF(WIN32)
		SET_TARGET_PROPERTIES(Oculars
                        PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition"
		)
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain)
	ENDIF(WIN32)
	TARGET_LINK_LIBRARIES(Oculars ${StelMain} ${extLinkerOption})
	QT5_USE_MODULES(Oculars Core Declarative Network OpenGL Svg Widgets)
	INSTALL(TARGETS Oculars DESTINATION "modules/${PACKAGE}")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Oculars-static
		STATIC ${Oculars_SRCS} ${Oculars_RES_CXX} ${Oculars_UIS_H}
	)
	SET_TARGET_PROPERTIES(Oculars-static PROPERTIES OUTPUT_NAME "Oculars")
	QT5_USE_MODULES(Oculars-static Core Declarative Network OpenGL Svg Widgets)


	TARGET_LINK_LIBRARIES(Oculars-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(Oculars-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Oculars-static)
ENDIF()
