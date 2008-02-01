ADD_SUBDIRECTORY( planetsephems ) 
ADD_SUBDIRECTORY( stelutils ) 

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/planetsephems ${CMAKE_SOURCE_DIR}/src/stelutils ${CMAKE_SOURCE_DIR}/intl ${CMAKE_SOURCE_DIR}/src/stelutils/iniparser ${FreeType2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${INTL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${QT_INCLUDE_DIR}/QtScript ${QT_QTSCRIPT_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/src)

########### next target ###############

SET(stellarium_lib_SRCS
   Projector.cpp
   Projector.hpp
   Mapping.hpp
   Mapping.cpp
   MappingClasses.hpp
   MappingClasses.cpp
   SphericMirrorCalculator.cpp
   SphericMirrorCalculator.hpp
   ViewportDistorter.cpp
   ViewportDistorter.hpp
   Navigator.cpp
   Navigator.hpp
   Observer.cpp
   Observer.hpp
   GeodesicGrid.cpp
   GeodesicGrid.hpp
   StelObject.cpp
   StelObject.hpp
   StelObjectType.hpp
   ZoneData.hpp
   Star.cpp
   Star.hpp
   StarWrapper.cpp
   StarWrapper.hpp
   ZoneArray.cpp
   ZoneArray.hpp
   ZoneData.hpp
   StringArray.cpp
   StringArray.hpp
   StarMgr.cpp
   StarMgr.hpp
   Nebula.cpp
   Nebula.hpp
   NebulaMgr.cpp
   NebulaMgr.hpp
   Constellation.cpp
   Constellation.hpp
   ConstellationMgr.cpp
   ConstellationMgr.hpp
   Meteor.cpp
   Meteor.hpp
   MeteorMgr.cpp
   MeteorMgr.hpp
   Planet.cpp
   Planet.hpp
   orbit.cpp
   orbit.h
   SolarSystem.cpp
   SolarSystem.hpp
   Telescope.cpp
   Telescope.hpp
   TelescopeMgr.cpp
   TelescopeMgr.hpp
   Atmosphere.cpp
   Atmosphere.hpp
   image.cpp
   image.h
   image_mgr.cpp
   image_mgr.h
   Landscape.cpp
   Landscape.hpp
   LoadingBar.cpp
   LoadingBar.hpp
   script.cpp
   script.h
   script_mgr.cpp
   script_mgr.h
   s_gui.cpp
   s_gui.h
   Skybright.cpp
   Skybright.hpp
   Skylight.cpp
   Skylight.hpp
   solve.h
   StelApp.cpp
   StelApp.hpp
   audio.cpp
   audio.h
   command_interface.cpp
   command_interface.h
   stel_command_interface.cpp
   stel_command_interface.h
   StelCore.cpp
   StelCore.hpp
   stel_ui_conf.cpp
   stel_ui.cpp
   stel_ui.h
   stel_ui_tuiconf.cpp
   s_tui.cpp
   s_tui.h
   StelPluginInterface.hpp
   ToneReproducer.cpp
   ToneReproducer.hpp
   translations.h
   StelObjectModule.hpp
   StelObjectModule.cpp
   StelModule.hpp
   StelModule.cpp
   StelModuleMgr.cpp
   StelModuleMgr.hpp
   StelFontMgr.hpp
   StelFontMgr.cpp
   StelLocaleMgr.hpp
   StelLocaleMgr.cpp
   StelSkyCultureMgr.hpp
   StelSkyCultureMgr.cpp
   GeodesicGridDrawer.h
   GeodesicGridDrawer.cpp
   StelTextureMgr.hpp
   StelTextureMgr.cpp
   AstroImage.hpp
   AstroImage.cpp
   LandscapeMgr.hpp
   LandscapeMgr.cpp
   GridLinesMgr.hpp
   GridLinesMgr.cpp
   MilkyWay.hpp
   MilkyWay.cpp
   MovementMgr.hpp
   MovementMgr.cpp
   StelObjectMgr.hpp
   StelObjectMgr.cpp
   StelFileMgr.hpp
   StelFileMgr.cpp
   SkyDrawer.hpp
   SkyDrawer.cpp

   QtScriptMgr.hpp
   QtScriptMgr.cpp

   stelutils/bytes.h
   stelutils/gettext.h
   stelutils/StelIniParser.cpp
   stelutils/StelIniParser.hpp
   stelutils/SFont.hpp
   stelutils/SFont.cpp
   stelutils/STexture.hpp
   stelutils/STexture.cpp
   stelutils/typeface.cpp
   stelutils/typeface.h
   stelutils/StelUtils.cpp
   stelutils/StelUtils.hpp
   stelutils/Translator.cpp
   stelutils/Translator.hpp
   stelutils/vecmath.h
   stelutils/callbacks.hpp
   stelutils/callback_helpers.hpp
   stelutils/SphereGeometry.hpp
   stelutils/SphereGeometry.cpp
   stelutils/Fader.hpp
   stelutils/STextureTypes.hpp
   stelutils/GLee.c
   stelutils/GLee.h
   stelutils/StelKey.hpp
   stelutils/StelTypes.hpp
   stelutils/fixx11h.h

   stelutils/Grid.cpp
   stelutils/Grid.hpp
   stelutils/SimpleGrid.cpp
   stelutils/SimpleGrid.hpp
   stelutils/TreeGrid.hpp
   stelutils/TreeGrid.cpp
   stelutils/GridObject.hpp

   planetsephems/vsop87.c
   planetsephems/vsop87.h
   planetsephems/pluto.c
   planetsephems/pluto.h
   planetsephems/l1.c
   planetsephems/l1.h
   planetsephems/tass17.c
   planetsephems/tass17.h
   planetsephems/gust86.c
   planetsephems/gust86.h
   planetsephems/marssat.c
   planetsephems/marssat.h
   planetsephems/elp82b.c
   planetsephems/elp82b.h
   planetsephems/elliptic_to_rectangular.c
   planetsephems/elliptic_to_rectangular.h
   planetsephems/misc_stellplanet.c
   planetsephems/misc_stellplanet.h
   planetsephems/calc_interpolated_elements.c
   planetsephems/calc_interpolated_elements.h
   planetsephems/sideral_time.c
   planetsephems/sideral_time.h
   planetsephems/stellplanet.c
   planetsephems/stellplanet.h
   
   StelMainWindow.hpp
   StelMainWindow.cpp
   StelGLWidget.hpp
   StelGLWidget.cpp
)

################# compiles .ui files ############
SET(stellarium_UIS
  mainGui.ui)
QT4_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})

################# compiles resources files ############
SET(stellarium_RES
  ../data/mainRes.qrc)
QT4_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})

################# compiles moc files ############
# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(stellarium_MOC_HDRS
	QtScriptMgr.hpp
	StelModule.hpp
	StelModuleMgr.hpp
	ConstellationMgr.hpp
	LandscapeMgr.hpp
	GridLinesMgr.hpp
	MilkyWay.hpp
	MovementMgr.hpp
	NebulaMgr.hpp
	SolarSystem.hpp
	MeteorMgr.hpp
	Observer.hpp
	StarMgr.hpp
	StelApp.hpp
	Navigator.hpp
	StelGLWidget.hpp
	StelMainWindow.hpp
	stelutils/STexture.hpp
)

# After this call, stellarium_MOC_SRCS = moc_stellarium.cxx
QT4_WRAP_CPP(stellarium_MOC_SRCS ${stellarium_MOC_HDRS} ${stellarium_UIS_H})

SET(stellarium_exe_SRCS
   main.cpp
)

IF(MACOSX)
  SET(stellarium_lib_SRCS 
    ${stellarium_lib_SRCS}
    MacosxDirs.cpp
    MacosxDirs.hpp)
ENDIF(MACOSX)

SET(extLinkerOption ${QT_LIBRARIES} ${QT_QTSCRIPT_LIBRARY} ${FreeType2_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL_LIBRARY} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} ${SDLMIXER_LIBRARY})

IF(GENERATE_STELMAINLIB)
 ADD_LIBRARY(stelMain SHARED ${stellarium_lib_SRCS} ${stellarium_MOC_SRCS} ${stellarium_RES_CXX})
 TARGET_LINK_LIBRARIES(stelMain ${extLinkerOption})
 INSTALL(TARGETS stelMain DESTINATION lib)
 ADD_EXECUTABLE(stellarium ${stellarium_exe_SRCS})
 TARGET_LINK_LIBRARIES(stellarium stelMain ${extLinkerOption})
ELSE(GENERATE_STELMAINLIB)
 ADD_EXECUTABLE(stellarium ${stellarium_lib_SRCS} ${stellarium_MOC_SRCS} ${stellarium_RES_CXX} ${stellarium_exe_SRCS})
 TARGET_LINK_LIBRARIES(stellarium ${extLinkerOption})
ENDIF(GENERATE_STELMAINLIB)

INSTALL(TARGETS stellarium DESTINATION bin)
