ADD_SUBDIRECTORY( planetsephems ) 
ADD_SUBDIRECTORY( stelutils ) 

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/planetsephems ${CMAKE_SOURCE_DIR}/src/stelutils ${CMAKE_SOURCE_DIR}/intl ${CMAKE_SOURCE_DIR}/src/stelutils/iniparser ${FreeType2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR})

########### next target ###############

SET(stellarium_SRCS
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
   grid.cpp
   grid.h
   GeodesicGrid.cpp
   GeodesicGrid.hpp
   StelObject.cpp
   StelObject.hpp
   StelObjectType.hpp
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
   main.cpp
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
   StelAppSdl.hpp
   StelAppSdl.cpp
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
   TreeGrid.hpp
   TreeGrid.cpp
   StelFileMgr.hpp
   StelFileMgr.cpp
   StelAppQt4.hpp
   StelAppQt4.cpp

   stelutils/bytes.h
   stelutils/gettext.h
   stelutils/InitParser.cpp
   stelutils/InitParser.hpp
   stelutils/SFont.hpp
   stelutils/SFont.cpp
   stelutils/STexture.cpp
   stelutils/STexture.hpp
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
   stelutils/spgrid.hpp
   stelutils/GLee.c
   stelutils/GLee.h

   stelutils/iniparser/dictionary.c
   stelutils/iniparser/dictionary.h
   stelutils/iniparser/iniparser.c
   stelutils/iniparser/iniparser.h
   stelutils/iniparser/strlib.c
   stelutils/iniparser/strlib.h

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
)

ADD_EXECUTABLE(stellarium ${stellarium_SRCS})

TARGET_LINK_LIBRARIES(stellarium ${QT_LIBRARIES} ${FreeType2_LIBRARIES} ${CURL_LIBRARIES} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${SDL_LIBRARY} -lboost_filesystem -lboost_thread ${STLPORT_LIBRARY})

INSTALL(TARGETS stellarium DESTINATION bin)


########### install files ###############


#@LIBINTL@

