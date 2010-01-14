INCLUDE_DIRECTORIES(. sgp4sdp4 gui ${CMAKE_BINARY_DIR}/plugins/Satellites/src ${CMAKE_BINARY_DIR}/plugins/Satellites/src/gui)

LINK_DIRECTORIES(/src)

SET(Satellites_SRCS
  sgp4sdp4/sgp4sdp4.c
  sgp4sdp4/sgp4sdp4.h
  sgp4sdp4/sgp_in.c
  sgp4sdp4/sgp_math.c
  sgp4sdp4/sgp_obs.c
  sgp4sdp4/sgp_time.c
  sgp4sdp4/solar.c
  Satellite.hpp
  Satellite.cpp
  Satellites.hpp
  Satellites.cpp
  gui/StelDialogPlugin.hpp
  gui/StelDialogPlugin.cpp
  gui/SatellitesDialog.hpp
  gui/SatellitesDialog.cpp
)

SET(SatellitesDialog_UIS
  gui/satellitesDialog.ui
)
QT4_WRAP_UI(SatellitesDialog_UIS_H ${SatellitesDialog_UIS})

SET(Satellites_RES ../Satellites.qrc)
QT4_ADD_RESOURCES(Satellites_RES_CXX ${Satellites_RES})

# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(Satellites_MOC_HDRS
  Satellites.hpp
  gui/StelDialogPlugin.hpp
  gui/SatellitesDialog.hpp
)

# After this call, Satellites_MOC_SRCS = moc_Satellites.cxx
QT4_WRAP_CPP(Satellites_MOC_SRCS ${Satellites_MOC_HDRS})

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES})

############### For building the dynamic library ######################
IF(BUILD_DYNAMIC_PLUGINS)
	ADD_LIBRARY(Satellites MODULE ${Satellites_SRCS} ${Satellites_MOC_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
	IF(APPLE)
		FIND_LIBRARY(OPENGL_LIBRARY OpenGL)
		MARK_AS_ADVANCED(OPENGL_LIBRARY)
		SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-undefined dynamic_lookup" SUFFIX ".dylib")
	ENDIF()

	IF(WIN32)
		SET_TARGET_PROPERTIES(Satellites PROPERTIES LINK_FLAGS "-enable-runtime-pseudo-reloc -Wl,--allow-multiple-definition" )
		SET(StelMain stelMain)
	ELSE(WIN32)
		SET(StelMain )
	ENDIF(WIN32)

	TARGET_LINK_LIBRARIES(Satellites ${StelMain} ${extLinkerOption})
	INSTALL(TARGETS Satellites DESTINATION "modules/Satellites")
ENDIF()

############### For building the static library ######################
IF(BUILD_STATIC_PLUGINS)
	ADD_LIBRARY(Satellites-static STATIC ${Satellites_SRCS} ${Satellites_MOC_SRCS} ${Satellites_RES_CXX} ${SatellitesDialog_UIS_H})
	# The library target "Satellites-static" has a default OUTPUT_NAME of "Satellites-static", so change it.
	SET_TARGET_PROPERTIES(Satellites-static PROPERTIES OUTPUT_NAME "Satellites")
	TARGET_LINK_LIBRARIES(Satellites-static ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(Satellites-static PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
	ADD_DEPENDENCIES(AllStaticPlugins Satellites-static)
ENDIF()







