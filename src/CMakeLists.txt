

INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${CMAKE_SOURCE_DIR}/src/core ${CMAKE_SOURCE_DIR}/src/gui ${CMAKE_SOURCE_DIR}/src/modules ${CMAKE_SOURCE_DIR}/src/planetsephems ${CMAKE_SOURCE_DIR}/src/stelutils ${CMAKE_SOURCE_DIR}/src/stelutils/iniparser ${FreeType2_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${CURL_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${Boost_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${ICONV_INCLUDE_DIR} ${INTL_INCLUDE_DIR} ${SDLMIXER_INCLUDE_DIR} ${QT_INCLUDE_DIR}/QtScript ${QT_QTSCRIPT_INCLUDE_DIR} ${CMAKE_BINARY_DIR}/src)

########### next target ###############

SET(stellarium_lib_SRCS
	core/Audio.cpp
	core/Audio.hpp
	core/GeodesicGrid.cpp
	core/GeodesicGrid.hpp
	core/LoadingBar.cpp
	core/LoadingBar.hpp
	core/MappingClasses.cpp
	core/MappingClasses.hpp
	core/Mapping.cpp
	core/Mapping.hpp
	core/MovementMgr.cpp
	core/MovementMgr.hpp
	core/Navigator.cpp
	core/Navigator.hpp
	core/Observer.cpp
	core/Observer.hpp
	core/Location.hpp
	core/Location.cpp
	core/LocationMgr.hpp
	core/LocationMgr.cpp
	core/Projector.cpp
	core/Projector.hpp
	core/SFont.cpp
	core/SFont.hpp
	core/SkyDrawer.cpp
	core/SkyDrawer.hpp
	core/SkyImageTile.hpp
	core/SkyImageTile.cpp
	core/SphericMirrorCalculator.cpp
	core/SphericMirrorCalculator.hpp
	core/StelApp.cpp
	core/StelApp.hpp
	core/StelCore.cpp
	core/StelCore.hpp
	core/StelFileMgr.cpp
	core/StelFileMgr.hpp
	core/StelFontMgr.cpp
	core/StelFontMgr.hpp
	core/StelAppGraphicsScene.cpp
	core/StelAppGraphicsScene.hpp
	core/StelLocaleMgr.cpp
	core/StelLocaleMgr.hpp
	core/StelModule.cpp
	core/StelModule.hpp
	core/StelModuleMgr.cpp
	core/StelModuleMgr.hpp
	core/StelObject.cpp
	core/StelObject.hpp
	core/StelObjectMgr.cpp
	core/StelObjectMgr.hpp
	core/StelObjectModule.cpp
	core/StelObjectModule.hpp
	core/StelObjectType.hpp
	core/StelPluginInterface.hpp
	core/StelSkyCultureMgr.cpp
	core/StelSkyCultureMgr.hpp
	core/StelTextureMgr.cpp
	core/StelTextureMgr.hpp
	core/STexture.cpp
	core/STexture.hpp
	core/STextureTypes.hpp
	core/ToneReproducer.cpp
	core/ToneReproducer.hpp
	core/ViewportDistorter.cpp
	core/ViewportDistorter.hpp
	core/StelStyle.hpp
	
	stelutils/bytes.h
	stelutils/callback_helpers.hpp
	stelutils/callbacks.hpp
	stelutils/Fader.hpp
	stelutils/fixx11h.h
	stelutils/gettext.h
	stelutils/GLee.c
	stelutils/GLee.h
	stelutils/Grid.cpp
	stelutils/Grid.hpp
	stelutils/GridObject.hpp
	stelutils/SimpleGrid.cpp
	stelutils/SimpleGrid.hpp
	stelutils/SphereGeometry.cpp
	stelutils/SphereGeometry.hpp
	stelutils/StelIniParser.cpp
	stelutils/StelIniParser.hpp
	stelutils/StelUtils.cpp
	stelutils/StelUtils.hpp
	stelutils/TextEntryDateTimeValidator.cpp
	stelutils/TextEntryDateTimeValidator.hpp
	stelutils/Translator.cpp
	stelutils/Translator.hpp
	stelutils/TreeGrid.cpp
	stelutils/TreeGrid.hpp
	stelutils/typeface.cpp
	stelutils/typeface.h
	stelutils/vecmath.h
	stelutils/QtJsonParser.hpp
	stelutils/QtJsonParser.cpp
	
	planetsephems/calc_interpolated_elements.c
	planetsephems/calc_interpolated_elements.h
	planetsephems/elliptic_to_rectangular.c
	planetsephems/elliptic_to_rectangular.h
	planetsephems/elp82b.c
	planetsephems/elp82b.h
	planetsephems/gust86.c
	planetsephems/gust86.h
	planetsephems/l1.c
	planetsephems/l1.h
	planetsephems/marssat.c
	planetsephems/marssat.h
	planetsephems/misc_stellplanet.c
	planetsephems/misc_stellplanet.h
	planetsephems/pluto.c
	planetsephems/pluto.h
	planetsephems/sideral_time.c
	planetsephems/sideral_time.h
	planetsephems/stellplanet.c
	planetsephems/stellplanet.h
	planetsephems/tass17.c
	planetsephems/tass17.h
	planetsephems/vsop87.c
	planetsephems/vsop87.h
	
	modules/Atmosphere.cpp
	modules/Atmosphere.hpp
	modules/Constellation.cpp
	modules/Constellation.hpp
	modules/ConstellationMgr.cpp
	modules/ConstellationMgr.hpp
	modules/GridLinesMgr.cpp
	modules/GridLinesMgr.hpp
	modules/Landscape.cpp
	modules/Landscape.hpp
	modules/LandscapeMgr.cpp
	modules/LandscapeMgr.hpp
	modules/Meteor.cpp
	modules/Meteor.hpp
	modules/MeteorMgr.cpp
	modules/MeteorMgr.hpp
	modules/MilkyWay.cpp
	modules/MilkyWay.hpp
	modules/Nebula.cpp
	modules/Nebula.hpp
	modules/NebulaMgr.cpp
	modules/NebulaMgr.hpp
	modules/Orbit.cpp
	modules/Orbit.hpp
	modules/Planet.cpp
	modules/Planet.hpp
	modules/SkyBackground.cpp
	modules/SkyBackground.hpp
	modules/Skybright.cpp
	modules/Skybright.hpp
	modules/Skylight.cpp
	modules/Skylight.hpp
	modules/SolarSystem.cpp
	modules/SolarSystem.hpp
	modules/Solve.hpp
	modules/Star.cpp
	modules/Star.hpp
	modules/StarMgr.cpp
	modules/StarMgr.hpp
	modules/StarWrapper.cpp
	modules/StarWrapper.hpp
	modules/StringArray.cpp
	modules/StringArray.hpp
	modules/Telescope.cpp
	modules/Telescope.hpp
	modules/TelescopeMgr.cpp
	modules/TelescopeMgr.hpp
#	modules/TextUI.cpp
#	modules/TextUI.hpp
	modules/ZoneArray.cpp
	modules/ZoneArray.hpp
	modules/ZoneData.hpp
	
	gui/StelGui.hpp
	gui/StelGui.cpp
	gui/StelGuiItems.hpp
	gui/StelGuiItems.cpp
	gui/HelpDialog.hpp
	gui/HelpDialog.cpp
	gui/Dialog.hpp
	gui/Dialog.cpp
	gui/MapLabel.hpp
	gui/MapLabel.cpp
	gui/AngleSpinBox.hpp
	gui/AngleSpinBox.cpp
	gui/LocationDialog.hpp
	gui/LocationDialog.cpp
	gui/DateTimeDialog.hpp
	gui/DateTimeDialog.cpp
	gui/ViewDialog.hpp
	gui/ViewDialog.cpp
	gui/SearchDialog.hpp
	gui/SearchDialog.cpp
	gui/ConfigurationDialog.hpp
	gui/ConfigurationDialog.cpp
	gui/StelDialog.hpp
	gui/StelDialog.cpp
	
	QtScriptMgr.cpp
	QtScriptMgr.hpp
	StelMainGraphicsView.hpp
	StelMainGraphicsView.cpp
	StelMainWindow.hpp
	StelMainWindow.cpp
	translations.h
)

IF(MACOSX)
  SET(stellarium_lib_SRCS 
    ${stellarium_lib_SRCS}
    stelutils/MacosxDirs.cpp
    stelutils/MacosxDirs.hpp)
ENDIF(MACOSX)

SET(stellarium_exe_SRCS
	main.cpp
)

################# compiles .ui files ############
SET(stellarium_UIS
  gui/locationDialogGui.ui
  gui/helpDialogGui.ui
  gui/dateTimeDialogGui.ui
  gui/viewDialog.ui
  gui/searchDialogGui.ui
  gui/configurationDialog.ui
  )
QT4_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})

################# compiles resources files ############
SET(stellarium_RES
  ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
QT4_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})

################# compiles moc files ############
# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(stellarium_MOC_HDRS
	core/StelModuleMgr.hpp
	core/Observer.hpp
	core/StelApp.hpp
	core/Navigator.hpp
	core/LocationMgr.hpp
	core/Projector.hpp
	core/StelAppGraphicsScene.hpp
	core/STexture.hpp
	core/MovementMgr.hpp
	core/SkyImageTile.hpp
	core/SkyDrawer.hpp
	modules/ConstellationMgr.hpp
	modules/LandscapeMgr.hpp
	modules/GridLinesMgr.hpp
	modules/MilkyWay.hpp
	modules/NebulaMgr.hpp
	modules/SkyBackground.hpp
	modules/SolarSystem.hpp
	modules/MeteorMgr.hpp
	modules/StarMgr.hpp
	gui/StelGui.hpp
	gui/StelGuiItems.hpp
	gui/HelpDialog.hpp
	gui/Dialog.hpp
	gui/MapLabel.hpp
	gui/AngleSpinBox.hpp
	gui/LocationDialog.hpp
	gui/DateTimeDialog.hpp
	gui/ViewDialog.hpp
	gui/SearchDialog.hpp
	gui/ConfigurationDialog.hpp
	gui/StelDialog.hpp
	StelMainGraphicsView.hpp
	StelMainWindow.hpp
	QtScriptMgr.hpp
)

# After this call, stellarium_MOC_SRCS = moc_stellarium.cxx
QT4_WRAP_CPP(stellarium_MOC_SRCS ${stellarium_MOC_HDRS})

# Dummy target depending on ${stellarium_UIS_H} to trigger ui compilations
ADD_CUSTOM_TARGET(GenerateUiHeaders ALL DEPENDS ${stellarium_UIS_H})

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
 IF(APPLE)
  SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/works/OpenGL.framework/Versions/A/Libraries/libGL.dylib -framework Cocoa")
 ENDIF(APPLE)
ENDIF(GENERATE_STELMAINLIB)

INSTALL(TARGETS stellarium DESTINATION bin)

## tests

SET(tests_testdates_SRCS
	stelutils/StelUtils.cpp
	stelutils/StelUtils.hpp
	tests/testdates.cpp
	stelutils/TextEntryDateTimeValidator.hpp
	stelutils/TextEntryDateTimeValidator.cpp)
ADD_EXECUTABLE(testdates ${tests_testdates_SRCS})
TARGET_LINK_LIBRARIES(testdates ${extLinkerOption})
ADD_TEST(test_testdates ${EXECUTABLE_OUTPUT_PATH}/testdates)

SET(tests_testSphereGeometry_SRCS
	tests/testSphereGeometry.cpp
	stelutils/SphereGeometry.hpp
	stelutils/SphereGeometry.cpp
	stelutils/StelUtils.cpp
	stelutils/StelUtils.hpp)
ADD_EXECUTABLE(testSphereGeometry ${tests_testSphereGeometry_SRCS})
TARGET_LINK_LIBRARIES(testSphereGeometry ${extLinkerOption})
ADD_TEST(test_testSphereGeometry ${EXECUTABLE_OUTPUT_PATH}/testSphereGeometry)
