INCLUDE_DIRECTORIES(. gsatellite gui ${CMAKE_BINARY_DIR}/plugins/Satellites/src ${CMAKE_BINARY_DIR}/plugins/Satellites/src/gui)

LINK_DIRECTORIES(/src)

SET(Satellites_SRCS
  gsatellite/gException.hpp
  gsatellite/gSatTEME.cpp
  gsatellite/gSatTEME.hpp
  gsatellite/mathUtils.cpp
  gsatellite/mathUtils.hpp
  gsatellite/gTime.cpp
  gsatellite/gTime.hpp
  gsatellite/gTimeSpan.cpp
  gsatellite/gVector.cpp
  gsatellite/gVector.hpp
  gsatellite/gVectorTempl.hpp
  gsatellite/sgp4ext.cpp
  gsatellite/sgp4ext.h
  gsatellite/sgp4io.cpp
  gsatellite/sgp4io.h
  gsatellite/sgp4unit.cpp
  gsatellite/sgp4unit.h
  gsatellite/stdsat.h

  gSatWrapper.hpp
  gSatWrapper.cpp
  Satellite.hpp
  Satellite.cpp
  Satellites.hpp
  Satellites.cpp
  SatellitesListModel.hpp
  SatellitesListModel.cpp
  SatellitesListFilterModel.hpp
  SatellitesListFilterModel.cpp
  gui/SatellitesDialog.hpp
  gui/SatellitesDialog.cpp
  gui/SatellitesImportDialog.hpp
  gui/SatellitesImportDialog.cpp
)

SET(SatellitesDialog_UIS
  gui/satellitesDialog.ui
  gui/satellitesImportDialog.ui
)
QT5_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})

SET(Satellites_RES ../resources/Satellites.qrc)
QT5_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})

SET(extLinkerOption ${OPENGL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Satellites MODULE ${Satellites_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
	QT5_USE_MODULES(Satellites Core Declarative Network)
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
	  SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-Wl,--enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
	  SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Satellites ${StelMain} ${extLinkerOption})
	QT5_USE_MODULES(Satellites-static Core Gui  Network OpenGL Widgets)
	INSTALL(TARGETS Satellites DESTINATION "modules/Satellites")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Satellites-static STATIC ${Satellites_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
	QT5_USE_MODULES(Satellites-static Core Declarative Network)
	# The library target "Satellites-static" has a default OUTPUT_NAME of "Satellites-static", so change it.
	SET_TARGET_PROPERTIES(Satellites-static PROPERTIES OUTPUT_NAME "Satellites")
	TARGET_LINK_LIBRARIES(Satellites-static ${StelMain} ${extLinkerOption})
	QT5_USE_MODULES(Satellites-static Core Gui  Network OpenGL Widgets)
	SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN -Wno-unused-parameter")
	ADD_DEPENDENCIES(AllStaticPlugins Satellites-static)
ENDIF()







