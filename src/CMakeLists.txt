#add our external dependencies first
add_subdirectory(external)

IF(ENABLE_SPOUT)
    SET(spout_SRCS
     external/SpoutLibrary.h
     core/SpoutSender.hpp
     core/SpoutSender.cpp
	 )
ENDIF()

SET(stellarium_lib_SRCS
     core/BlueNoiseTriangleRemapped.hpp
	 core/StelSRGB.hpp
     core/Dithering.hpp
     core/Dithering.cpp
     core/SaturationShader.hpp
     core/StelAudioMgr.hpp
     core/StelAudioMgr.cpp
     core/StelVideoMgr.hpp
     core/StelVideoMgr.cpp
     core/StelGeodesicGrid.cpp
     core/StelGeodesicGrid.hpp
     core/StelMovementMgr.cpp
     core/StelMovementMgr.hpp
     core/StelObserver.cpp
     core/StelObserver.hpp
     core/StelLocation.hpp
     core/StelLocation.cpp
     core/StelLocationMgr.hpp
     core/StelLocationMgr_p.hpp
     core/StelLocationMgr.cpp
     core/StelProjector.cpp
     core/StelProjector.hpp
     core/StelProjectorClasses.cpp
     core/StelProjectorClasses.hpp
     core/StelProjectorType.hpp
     core/StelSkyDrawer.cpp
     core/StelSkyDrawer.hpp
     core/StelPainter.hpp
     core/StelPainter.cpp
     core/MultiLevelJsonBase.hpp
     core/MultiLevelJsonBase.cpp
     core/StelSkyImageTile.hpp
     core/StelSkyImageTile.cpp
     core/SphericMirrorCalculator.cpp
     core/SphericMirrorCalculator.hpp
     core/StelApp.cpp
     core/StelApp.hpp
     core/StelCore.cpp
     core/StelCore.hpp
     core/StelFileMgr.cpp
     core/StelFileMgr.hpp
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
     core/StelOpenGL.cpp
     core/StelOpenGL.hpp
     core/StelPluginInterface.hpp
     core/StelRegionObject.hpp
     core/StelSkyCultureMgr.cpp
     core/StelSkyCultureMgr.hpp
     core/StelTextureMgr.cpp
     core/StelTextureMgr.hpp
     core/StelTexture.cpp
     core/StelTexture.hpp
     core/StelTextureTypes.hpp
     core/StelToneReproducer.cpp
     core/StelToneReproducer.hpp
     core/StelSkyLayerMgr.cpp
     core/StelSkyLayerMgr.hpp
     core/StelSkyLayer.hpp
     core/StelSkyLayer.cpp
     core/StelFader.hpp
     core/StelSphereGeometry.cpp
     core/StelSphereGeometry.hpp
     core/OctahedronPolygon.cpp
     core/OctahedronPolygon.hpp
     core/StelIniParser.cpp
     core/StelIniParser.hpp
     core/StelUtils.cpp
     core/StelUtils.hpp
     core/StelTranslator.cpp
     core/StelTranslator.hpp
     core/VecMath.hpp
     core/VecMath.cpp
     core/StelJsonParser.hpp
     core/StelJsonParser.cpp
     core/SimbadSearcher.hpp
     core/SimbadSearcher.cpp
     core/StelSphericalIndex.hpp
     core/StelSphericalIndex.cpp
     core/StelVertexArray.hpp
     core/StelVertexArray.cpp
     core/StelGuiBase.hpp
     core/StelGuiBase.cpp
     core/StelViewportEffect.hpp
     core/StelViewportEffect.cpp
     core/TrailGroup.hpp
     core/TrailGroup.cpp
     core/RefractionExtinction.hpp
     core/RefractionExtinction.cpp
     core/StelToast.hpp
     core/StelToast.cpp
     core/StelToastGrid.hpp
     core/StelToastGrid.cpp
     core/StelActionMgr.hpp
     core/StelActionMgr.cpp
     core/StelProgressController.hpp
     core/StelPropertyMgr.hpp
     core/StelPropertyMgr.cpp
     core/StelOBJ.hpp
     core/StelOBJ.cpp
     core/GeomMath.hpp
     core/GeomMath.cpp
     core/StelOpenGLArray.hpp
     core/StelOpenGLArray.cpp
     core/StelHips.hpp
     core/StelHips.cpp
     core/TextureAverageComputer.hpp
     core/TextureAverageComputer.cpp

     ${spout_SRCS}

     core/planetsephems/calc_interpolated_elements.c
     core/planetsephems/calc_interpolated_elements.h
     core/planetsephems/elliptic_to_rectangular.c
     core/planetsephems/elliptic_to_rectangular.h
     core/planetsephems/elp82b.c
     core/planetsephems/elp82b.h
     core/planetsephems/gust86.c
     core/planetsephems/gust86.h
     core/planetsephems/l12.c
     core/planetsephems/l12.h
     core/planetsephems/marssat.c
     core/planetsephems/marssat.h
     core/planetsephems/pluto.c
     core/planetsephems/pluto.h
     core/planetsephems/precession.c
     core/planetsephems/precession.h
     core/planetsephems/sidereal_time.c
     core/planetsephems/sidereal_time.h
     core/planetsephems/jpl_int.h
     core/planetsephems/jpleph.h
     core/planetsephems/jpleph.cpp
     core/planetsephems/EphemWrapper.cpp
     core/planetsephems/EphemWrapper.hpp
     core/planetsephems/tass17.c
     core/planetsephems/tass17.h
     core/planetsephems/vsop87.c
     core/planetsephems/vsop87.h
     core/planetsephems/de430.cpp
     core/planetsephems/de430.hpp
     core/planetsephems/de431.cpp
     core/planetsephems/de431.hpp
     core/planetsephems/de440.cpp
     core/planetsephems/de440.hpp
     core/planetsephems/de441.cpp
     core/planetsephems/de441.hpp
     core/planetsephems/htc20b.c
     core/planetsephems/htc20b.h
     core/modules/AtmospherePreetham.cpp
     core/modules/AtmospherePreetham.hpp
     core/modules/AtmosphereShowMySky.cpp
     core/modules/AtmosphereShowMySky.hpp
     core/modules/Atmosphere.hpp
     core/modules/Asterism.cpp
     core/modules/Asterism.hpp
     core/modules/AsterismMgr.cpp
     core/modules/AsterismMgr.hpp
     core/modules/Constellation.cpp
     core/modules/Constellation.hpp
     core/modules/ConstellationMgr.cpp
     core/modules/ConstellationMgr.hpp
     core/modules/CustomObject.cpp
     core/modules/CustomObject.hpp
     core/modules/CustomObjectMgr.cpp
     core/modules/CustomObjectMgr.hpp
     core/modules/HighlightMgr.cpp
     core/modules/HighlightMgr.hpp
     core/modules/GridLinesMgr.cpp
     core/modules/GridLinesMgr.hpp
     core/modules/HipsMgr.hpp
     core/modules/HipsMgr.cpp
     core/modules/LabelMgr.hpp
     core/modules/LabelMgr.cpp
     core/modules/MarkerMgr.hpp
     core/modules/MarkerMgr.cpp
     core/modules/Landscape.cpp
     core/modules/Landscape.hpp
     core/modules/LandscapeMgr.cpp
     core/modules/LandscapeMgr.hpp
     core/modules/Meteor.cpp
     core/modules/Meteor.hpp
     core/modules/SporadicMeteor.cpp
     core/modules/SporadicMeteor.hpp
     core/modules/SporadicMeteorMgr.cpp
     core/modules/SporadicMeteorMgr.hpp
     core/modules/MilkyWay.cpp
     core/modules/MilkyWay.hpp
     core/modules/Nebula.cpp
     core/modules/Nebula.hpp
     core/modules/NebulaList.hpp
     core/modules/NebulaMgr.cpp
     core/modules/NebulaMgr.hpp
     core/modules/Orbit.cpp
     core/modules/Orbit.hpp
     core/modules/Planet.cpp
     core/modules/Planet.hpp
     core/modules/RotationElements.cpp
     core/modules/RotationElements.hpp
     core/modules/MinorPlanet.cpp
     core/modules/MinorPlanet.hpp
     core/modules/Comet.cpp
     core/modules/Comet.hpp
     core/modules/Skybright.cpp
     core/modules/Skybright.hpp
     core/modules/Skylight.cpp
     core/modules/Skylight.hpp
     core/modules/SolarSystem.cpp
     core/modules/SolarSystem.hpp
     core/modules/NomenclatureItem.cpp
     core/modules/NomenclatureItem.hpp
     core/modules/NomenclatureMgr.cpp
     core/modules/NomenclatureMgr.hpp
     core/modules/Solve.hpp
     core/modules/Star.cpp
     core/modules/Star.hpp
     core/modules/StarMgr.cpp
     core/modules/StarMgr.hpp
     core/modules/StarWrapper.cpp
     core/modules/StarWrapper.hpp
     core/modules/ToastMgr.hpp
     core/modules/ToastMgr.cpp
     core/modules/ZoneArray.cpp
     core/modules/ZodiacalLight.hpp
     core/modules/ZodiacalLight.cpp
     core/modules/ZoneArray.hpp
     core/modules/ZoneData.hpp
     core/modules/SpecialMarkersMgr.cpp
     core/modules/SpecialMarkersMgr.hpp
     core/modules/SpecificTimeMgr.cpp
     core/modules/SpecificTimeMgr.hpp
     StelMainView.hpp
     StelMainView.cpp
     StelSplashScreen.hpp
     StelSplashScreen.cpp
     StelLogger.hpp
     StelLogger.cpp
     CLIProcessor.hpp
     CLIProcessor.cpp
     translations.h
     translations_regions.h
)

IF(ENABLE_PCH)
     SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
         stelMain_pch.hpp
     )
ENDIF(ENABLE_PCH)

IF(ENABLE_SCRIPTING)
     SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
          scripting/StelScriptOutput.hpp
          scripting/StelScriptOutput.cpp
          scripting/V3d.cpp
          scripting/V3d.hpp
          scripting/StelScriptMgr.cpp
          scripting/StelScriptMgr.hpp
          scripting/ScreenImageMgr.hpp
          scripting/ScreenImageMgr.cpp
          scripting/StelMainScriptAPI.cpp
          scripting/StelMainScriptAPI.hpp
          scripting/StelMainScriptAPIProxy.cpp
          scripting/StelMainScriptAPIProxy.hpp
     )
ENDIF()

SET(stellarium_exe_SRCS main.cpp)

################# compiles resources files ############
SET(stellarium_RES ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
IF(${QT_VERSION_MAJOR} EQUAL "5")
     QT5_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})
ELSE()
     QT_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})
ENDIF()
#############################################################################################
############################# Standard GUI plugin compilation ###############################
#############################################################################################
IF(STELLARIUM_GUI_MODE STREQUAL "Standard")
     SET(stellarium_gui_SRCS
          gui/StelGui.hpp
          gui/StelGui.cpp
          gui/SkyGui.hpp
          gui/SkyGui.cpp
          gui/StelStyle.hpp
          gui/StelGuiItems.hpp
          gui/StelGuiItems.cpp
          gui/HelpDialog.hpp
          gui/HelpDialog.cpp
          gui/ShortcutLineEdit.hpp
          gui/ShortcutLineEdit.cpp
          gui/ShortcutsDialog.hpp
          gui/ShortcutsDialog.cpp
          gui/Dialog.hpp
          gui/Dialog.cpp
          gui/MapWidget.hpp
          gui/MapWidget.cpp
          gui/AngleSpinBox.hpp
          gui/AngleSpinBox.cpp
          gui/LocationDialog.hpp
          gui/LocationDialog.cpp
          gui/DateTimeDialog.hpp
          gui/DateTimeDialog.cpp
          gui/ViewDialog.hpp
          gui/ViewDialog.cpp
          gui/LightPollutionWidget.hpp
          gui/LightPollutionWidget.cpp
          gui/LogarithmicSpinBox.hpp
          gui/LogarithmicSpinBox.cpp
          gui/SearchDialog.hpp
          gui/SearchDialog.cpp
          gui/ConfigurationDialog.hpp
          gui/ConfigurationDialog.cpp
          gui/AddRemoveLandscapesDialog.hpp
          gui/AddRemoveLandscapesDialog.cpp
          gui/AtmosphereDialog.hpp
          gui/AtmosphereDialog.cpp
          gui/ConfigureDSOColorsDialog.hpp
          gui/ConfigureDSOColorsDialog.cpp
          gui/ConfigureOrbitColorsDialog.hpp
          gui/ConfigureOrbitColorsDialog.cpp
          gui/GreatRedSpotDialog.hpp
          gui/GreatRedSpotDialog.cpp
          gui/CustomDeltaTEquationDialog.hpp
          gui/CustomDeltaTEquationDialog.cpp
          gui/ConfigureScreenshotsDialog.hpp
          gui/ConfigureScreenshotsDialog.cpp
          gui/AstroCalcDialog.hpp
          gui/AstroCalcDialog.cpp
          gui/AstroCalcExtraEphemerisDialog.hpp
          gui/AstroCalcExtraEphemerisDialog.cpp
          gui/AstroCalcCustomStepsDialog.hpp
          gui/AstroCalcCustomStepsDialog.cpp
          gui/AstroCalcChart.hpp
          gui/AstroCalcChart.cpp
          gui/ObsListDialog.hpp
          gui/ObsListDialog.cpp
          gui/StelDialog.hpp
          gui/StelDialog_p.hpp
          gui/StelDialog.cpp
          gui/StelDialogSeparate.hpp
          gui/StelDialogSeparate.cpp
          gui/TonemappingDialog.hpp
          gui/TonemappingDialog.cpp
          gui/SkylightDialog.hpp
          gui/SkylightDialog.cpp
          gui/ContributorsList.hpp
          gui/StelCloseButton.hpp
          gui/StelCloseButton.cpp
          gui/ExternalStepSpinBox.hpp
     )

     ################# compiles .ui files ############
     SET(stellarium_UIS
          gui/locationDialogGui.ui
          gui/helpDialogGui.ui
          gui/shortcutsDialog.ui
          gui/dateTimeDialogGui.ui
          gui/viewDialog.ui
          gui/lightPollutionWidget.ui
          gui/searchDialogGui.ui
          gui/configurationDialog.ui
          gui/atmosphereDialog.ui
          gui/greatRedSpotDialog.ui
          gui/customDeltaTEquationDialog.ui
          gui/configureScreenshotsDialog.ui
          gui/astroCalcDialog.ui
          gui/astroCalcExtraEphemerisDialog.ui
          gui/astroCalcCustomStepsDialog.ui
          gui/obsListDialog.ui
          gui/addRemoveLandscapesDialog.ui
          gui/dsoColorsDialog.ui
          gui/orbitColorsDialog.ui
          gui/tonemappingDialog.ui
          gui/skylightDialog.ui
     )

     IF(ENABLE_SCRIPTING)
          IF(ENABLE_SCRIPT_CONSOLE)
               SET(stellarium_gui_SRCS ${stellarium_gui_SRCS}
                    gui/ScriptConsole.hpp
                    gui/ScriptConsole.cpp
                    gui/StelScriptSyntaxHighlighter.hpp
                    gui/StelScriptSyntaxHighlighter.cpp
               )
               SET(stellarium_UIS ${stellarium_UIS}
                    gui/scriptConsole.ui
               )
          ENDIF()
     ENDIF()

     IF(${QT_VERSION_MAJOR} EQUAL "5")
         QT5_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})
     ELSE()
         QT_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})
    ENDIF()

     # compiles resources files
     SET(stellarium_gui_RES ${CMAKE_SOURCE_DIR}/data/gui/guiRes.qrc)
     IF(${QT_VERSION_MAJOR} EQUAL "5")
         QT5_ADD_RESOURCES(stellarium_gui_RES_CXX ${stellarium_gui_RES})
     ELSE()
         QT_ADD_RESOURCES(stellarium_gui_RES_CXX ${stellarium_gui_RES})
     ENDIF()

     SET(stellarium_lib_SRCS ${stellarium_lib_SRCS} ${stellarium_gui_SRCS} ${stellarium_UIS_H} ${stellarium_gui_RES_CXX})
ENDIF()

#############################################################################################
################################ No GUI plugin compilation ##################################
#############################################################################################
IF (STELLARIUM_GUI_MODE STREQUAL "None")
    ADD_DEFINITIONS(-DNO_GUI)
ENDIF()


###############################################################################################
################## Static plugins need to be known by the file StelApp.cpp at compilation time
################## so add the definitions to the compilation.
################## Also add the full path to the .a libraries at compilation time.

### demo plugins ###

FOREACH(NAME ${STELLARIUM_PLUGINS})
     STRING(TOUPPER ${NAME} NAME_UP)
     IF(USE_PLUGIN_${NAME_UP})
          SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${NAME}-static)
          ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_${NAME_UP})
     ENDIF()
ENDFOREACH()

IF(USE_PLUGIN_TELESCOPECONTROL)
     # WinSock library
     IF(WIN32)
          SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ws2_32)
     ENDIF()
ENDIF()


#############################################################################################
################################ Build main program #########################################
#############################################################################################

IF(WIN32)
    # This library is required for some native methods used in a fix in main.cpp
     SET(winMMLib winmm)
ENDIF()

IF(WIN32 AND ENABLE_WINDOWS_EXE_ICON)
     IF(RC_COMPILER_PATH)
          ADD_CUSTOM_COMMAND(
               OUTPUT ${CMAKE_BINARY_DIR}/stellarium-rc.o
               COMMAND ${RC_COMPILER_PATH} ${CMAKE_BINARY_DIR}/stellarium.rc ${CMAKE_BINARY_DIR}/stellarium-rc.o)
          SET(stellarium_exe_SRCS ${stellarium_exe_SRCS} ${CMAKE_BINARY_DIR}/stellarium-rc.o)
     ELSE()
          IF(MSVC)
               SET(stellarium_exe_SRCS ${stellarium_exe_SRCS} ${CMAKE_BINARY_DIR}/stellarium.rc)
          ENDIF()
     ENDIF()
ENDIF()

# Setup Qt5/6 dependencies
SET(STELLARIUM_QT_LIBRARIES Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Concurrent Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::GuiPrivate ${STEL_GLES_LIBS} Qt${QT_VERSION_MAJOR}::Network Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Charts)
IF(APPLE AND Qt6_FOUND)
     SET(STELLARIUM_QT_LIBRARIES ${STELLARIUM_QT_LIBRARIES} Qt${QT_VERSION_MAJOR}::DBus)
ENDIF()
IF(ENABLE_MEDIA)
     SET(STELLARIUM_QT_LIBRARIES ${STELLARIUM_QT_LIBRARIES} Qt${QT_VERSION_MAJOR}::Multimedia Qt${QT_VERSION_MAJOR}::MultimediaWidgets)
ENDIF()
IF(ENABLE_SCRIPTING)
    IF(ENABLE_SCRIPT_QML)
         SET(STELLARIUM_QT_LIBRARIES ${STELLARIUM_QT_LIBRARIES} Qt${QT_VERSION_MAJOR}::Qml)
    ELSE(ENABLE_SCRIPT_QML)
         SET(STELLARIUM_QT_LIBRARIES ${STELLARIUM_QT_LIBRARIES} Qt5::Script)
    ENDIF(ENABLE_SCRIPT_QML)
ENDIF()
IF(ENABLE_GPS)
     SET(STELLARIUM_QT_LIBRARIES ${STELLARIUM_QT_LIBRARIES} Qt${QT_VERSION_MAJOR}::Positioning Qt${QT_VERSION_MAJOR}::SerialPort)
ENDIF()

IF(ENABLE_SPOUT AND SPOUT_LIBRARY_DLL)
     # automatically copy the spout .dll to the binary directory, if possible
     CONFIGURE_FILE(${SPOUT_LIBRARY_DLL} ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)
ENDIF()

SET(STELMAIN_DEPS ${ZLIB_LIBRARIES} glues_stel ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${STELLARIUM_QT_LIBRARIES} ${SPOUT_LIBRARY})
IF(ENABLE_LIBGPS)
     SET(STELMAIN_DEPS ${STELMAIN_DEPS} ${GPS_LIBRARY})
ENDIF()
IF(ENABLE_XLSX)
     SET(STELMAIN_DEPS ${STELMAIN_DEPS} QXlsx::QXlsx)
ENDIF()

IF(HAIKU)
     SET(STELMAIN_DEPS ${STELMAIN_DEPS} network)
ENDIF()

# Main executable/library setup
IF(GENERATE_STELMAINLIB)
     ADD_LIBRARY(stelMain SHARED ${stellarium_lib_SRCS} ${stellarium_RES_CXX})
     TARGET_LINK_LIBRARIES(stelMain ${STELMAIN_DEPS})
     INSTALL(TARGETS stelMain DESTINATION lib)
     # WIN32 creates a windows app without console, no effect on non-win platforms
     ADD_EXECUTABLE(stellarium WIN32 ${stellarium_exe_SRCS})
     #stelMain should be after the plug-ins, otherwise the build crashes
     TARGET_LINK_LIBRARIES(stellarium stelMain ${winMMLib})
ELSE()
     ADD_LIBRARY(stelMain STATIC ${stellarium_lib_SRCS} ${stellarium_RES_CXX})
     TARGET_LINK_LIBRARIES(stelMain ${STELMAIN_DEPS})
     ADD_EXECUTABLE(stellarium WIN32 ${stellarium_exe_SRCS})
     TARGET_LINK_LIBRARIES(stellarium ${winMMLib} ${STELMAIN_DEPS} stelMain)

     IF(APPLE)
	  SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -framework Cocoa -framework OpenGL")
     ENDIF()
ENDIF()
if(ENABLE_PCH)
    # prepend an include to each cpp file which itself includes the most frequently used includes.
     target_precompile_headers(stelMain PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:${CMAKE_CURRENT_SOURCE_DIR}/stelMain_pch.hpp>)
     IF(CMAKE_VERSION VERSION_GREATER_EQUAL 3.19)
          SET(CMAKE_PCH_INSTANTIATE_TEMPLATES ON)
     ENDIF()
ENDIF(ENABLE_PCH)
IF(ENABLE_SHOWMYSKY)
    TARGET_INCLUDE_DIRECTORIES(stelMain PRIVATE "${ShowMySky_INCLUDE_DIRECTORIES}")
ENDIF()

SET_TARGET_PROPERTIES(stellarium PROPERTIES ENABLE_EXPORTS TRUE)

ADD_DEPENDENCIES(stellarium AllStaticPlugins)

# Adding manifest
IF(MSVC)
     ADD_CUSTOM_COMMAND(TARGET stellarium
          POST_BUILD
          COMMAND "mt.exe" -nologo -manifest ${PROJECT_SOURCE_DIR}/data/stellarium.exe.manifest -outputresource:$<TARGET_FILE:stellarium>
          COMMENT "Adding manifest..."
          VERBATIM
     )
ENDIF(MSVC)

IF(APPLE)
     INSTALL(TARGETS stellarium DESTINATION MacOS)
     INSTALL(CODE
	  "
	  EXECUTE_PROCESS(
	       COMMAND \"${MACDEPLOYQT_COMMAND}\"
		    \"\${CMAKE_CURRENT_BINARY_DIR}/Stellarium.app\"
	       WORKING_DIRECTORY \"\${CMAKE_CURRENT_BINARY_DIR}\"
	       OUTPUT_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/macdeployqt.log\"
	       ERROR_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/macdeployqt.err\"
	  )
	  MESSAGE(STATUS \"Running macdeployqt...\")
	  "
     )
     # for backward compatibility
     IF(ENABLE_SHOWMYSKY AND EXISTS ${ShowMySky_LIBRARY})
	  GET_FILENAME_COMPONENT(_sms_lib_dir "${ShowMySky_LIBRARY}" DIRECTORY)
	  INSTALL(DIRECTORY ${_sms_lib_dir}/ DESTINATION Frameworks
	       PATTERN "*.dylib"
	       PATTERN "ShowMySky*" EXCLUDE
	  )
     ENDIF()
     # workaround for https://bugreports.qt.io/browse/QTBUG-105984
     IF(ENABLE_MEDIA AND (${QT_VERSION} VERSION_GREATER_EQUAL "6.4.0"))
	  GET_FILENAME_COMPONENT(_qt_bin_dir "${MACDEPLOYQT_COMMAND}" DIRECTORY)
	  INSTALL(FILES ${_qt_bin_dir}/../plugins/multimedia/libdarwinmediaplugin.dylib DESTINATION PlugIns/multimedia)
     ENDIF()
     INSTALL(CODE
	  "
	  EXECUTE_PROCESS(
	       COMMAND \"../../util/macfix.sh\"
	  )
	  MESSAGE(STATUS \"Fixing Qt' paths for macOS bundle...\")
	  "
     )
ELSE()
     INSTALL(TARGETS stellarium DESTINATION bin)
ENDIF()
IF(WIN32)
     IF((${QT_VERSION_MAJOR} EQUAL "5") AND (Qt5Core_VERSION VERSION_LESS "5.15"))
          SET(VIRTUAL_KBD_SETTING "")
     ELSE()
          SET(VIRTUAL_KBD_SETTING "--no-virtualkeyboard")
     ENDIF()

     INSTALL(CODE
	  "
	  EXECUTE_PROCESS(
	       COMMAND \"${WINDEPLOYQT_COMMAND}\"
		    --dir \"\${CMAKE_INSTALL_PREFIX}/qtstuff\"
		    --no-translations
		    --verbose 1
		    ${VIRTUAL_KBD_SETTING}
		    --compiler-runtime
		    \"\${CMAKE_INSTALL_PREFIX}/bin/stellarium.exe\"
	       WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/bin\"
	       OUTPUT_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/windeployqt.log\"
	       ERROR_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/windeployqt.err\"
	  )
	  MESSAGE(STATUS \"Running windeployqt...\")
	  "
     )
     IF(GENERATE_STELMAINLIB)
          INSTALL(CODE
	       "
	       EXECUTE_PROCESS(
		    COMMAND \"${WINDEPLOYQT_COMMAND}\"
			 --dir \"\${CMAKE_INSTALL_PREFIX}/qtstuff\"
			 --no-translations
			 ${VIRTUAL_KBD_SETTING}
			 --compiler-runtime
			 \"\${CMAKE_INSTALL_PREFIX}/lib/libstelMain.dll\"
		    WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/bin\"
		    OUTPUT_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/windeployqt-dll.log\"
		    ERROR_FILE \"\${CMAKE_CURRENT_BINARY_DIR}/windeployqt-dll.err\"
	       )
	  "
     )
     ENDIF()
     # workaround for https://bugreports.qt.io/browse/QTBUG-105984
     IF(ENABLE_MEDIA AND (${QT_VERSION} VERSION_GREATER_EQUAL "6.4.0"))
	  GET_FILENAME_COMPONENT(_qt_bin_dir "${WINDEPLOYQT_COMMAND}" DIRECTORY)
	  INSTALL(FILES ${_qt_bin_dir}/../plugins/multimedia/windowsmediaplugin.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/qtstuff/multimedia)
     ENDIF()
ENDIF()

SET_TARGET_PROPERTIES(stelMain PROPERTIES FOLDER "src/core")
SET_TARGET_PROPERTIES(stellarium PROPERTIES FOLDER "src")

#############################################################################################
################################## Build unit tests #########################################
#############################################################################################

IF(ENABLE_TESTING)

    SET(TESTS_LIBRARIES ${ZLIB_LIBRARIES} Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Test stelMain)

    # Custom target used to build all tests at once
    ADD_CUSTOM_TARGET(buildTests)
    SET_TARGET_PROPERTIES(buildTests PROPERTIES FOLDER "src/tests")

    SET(tests_testDates_SRCS
        tests/testDates.hpp
        tests/testDates.cpp
    )
    ADD_EXECUTABLE(testDates ${tests_testDates_SRCS})
    TARGET_LINK_LIBRARIES(testDates ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testDates)
    ADD_TEST(testDates testDates)
    SET_TARGET_PROPERTIES(testDates PROPERTIES FOLDER "src/tests")

    SET(tests_testStelFileMgr_SRCS
        tests/testStelFileMgr.hpp
        tests/testStelFileMgr.cpp
    )
    ADD_EXECUTABLE(testStelFileMgr ${tests_testStelFileMgr_SRCS})
    TARGET_LINK_LIBRARIES(testStelFileMgr ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelFileMgr)
    ADD_TEST(testStelFileMgr testStelFileMgr)
    SET_TARGET_PROPERTIES(testStelFileMgr PROPERTIES FOLDER "src/tests")

    SET(tests_testStelSphereGeometry_SRCS
        tests/testStelSphereGeometry.hpp
        tests/testStelSphereGeometry.cpp
    )
    ADD_EXECUTABLE(testStelSphereGeometry ${tests_testStelSphereGeometry_SRCS})
    TARGET_LINK_LIBRARIES(testStelSphereGeometry ${TESTS_LIBRARIES} glues_stel)
    ADD_DEPENDENCIES(buildTests testStelSphereGeometry)
    ADD_TEST(testStelSphereGeometry testStelSphereGeometry)
    SET_TARGET_PROPERTIES(testStelSphereGeometry PROPERTIES FOLDER "src/tests")

    SET(tests_testStelSphericalIndex_SRCS
        tests/testStelSphericalIndex.hpp
        tests/testStelSphericalIndex.cpp
    )
    ADD_EXECUTABLE(testStelSphericalIndex ${tests_testStelSphericalIndex_SRCS})
    TARGET_LINK_LIBRARIES(testStelSphericalIndex ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelSphericalIndex)
    ADD_TEST(testStelSphericalIndex testStelSphericalIndex)
    SET_TARGET_PROPERTIES(testStelSphericalIndex PROPERTIES FOLDER "src/tests")

    SET(tests_testStelJsonParser_SRCS
        tests/testStelJsonParser.hpp
        tests/testStelJsonParser.cpp
    )
    ADD_EXECUTABLE(testStelJsonParser ${tests_testStelJsonParser_SRCS})
    TARGET_LINK_LIBRARIES(testStelJsonParser ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelJsonParser)
    ADD_TEST(testStelJsonParser testStelJsonParser)
    SET_TARGET_PROPERTIES(testStelJsonParser PROPERTIES FOLDER "src/tests")

    SET(tests_testStelIniParser_SRCS
        tests/testStelIniParser.hpp
        tests/testStelIniParser.cpp
    )
    ADD_EXECUTABLE(testStelIniParser ${tests_testStelIniParser_SRCS})
    TARGET_LINK_LIBRARIES(testStelIniParser ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelIniParser)
    ADD_TEST(testStelIniParser testStelIniParser)
    SET_TARGET_PROPERTIES(testStelIniParser PROPERTIES FOLDER "src/tests")

    SET(tests_testStelVertexArray_SRCS
        tests/testStelVertexArray.hpp
        tests/testStelVertexArray.cpp
    )
    ADD_EXECUTABLE(testStelVertexArray ${tests_testStelVertexArray_SRCS})
    TARGET_LINK_LIBRARIES(testStelVertexArray ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelVertexArray)
    ADD_TEST(testStelVertexArray testStelVertexArray)
    SET_TARGET_PROPERTIES(testStelVertexArray PROPERTIES FOLDER "src/tests")

    SET(tests_testDeltaT_SRCS
        tests/testDeltaT.hpp
        tests/testDeltaT.cpp
    )
    ADD_EXECUTABLE(testDeltaT ${tests_testDeltaT_SRCS})
    TARGET_LINK_LIBRARIES(testDeltaT ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testDeltaT)
    ADD_TEST(testDeltaT testDeltaT)
    SET_TARGET_PROPERTIES(testDeltaT PROPERTIES FOLDER "src/tests")

    SET(tests_testConversions_SRCS
        tests/testConversions.hpp
        tests/testConversions.cpp
    )
    ADD_EXECUTABLE(testConversions ${tests_testConversions_SRCS})
    TARGET_LINK_LIBRARIES(testConversions ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testConversions)
    ADD_TEST(testConversions testConversions)
    SET_TARGET_PROPERTIES(testConversions PROPERTIES FOLDER "src/tests")

    SET(tests_testComputations_SRCS
        tests/testComputations.hpp
        tests/testComputations.cpp
    )
    ADD_EXECUTABLE(testComputations ${tests_testComputations_SRCS})
    TARGET_LINK_LIBRARIES(testComputations ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testComputations)
    ADD_TEST(testComputations testComputations)
    SET_TARGET_PROPERTIES(testComputations PROPERTIES FOLDER "src/tests")

    SET(tests_testComparisons_SRCS
        tests/testComparisons.hpp
        tests/testComparisons.cpp
    )
    ADD_EXECUTABLE(testComparisons ${tests_testComparisons_SRCS})
    TARGET_LINK_LIBRARIES(testComparisons ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testComparisons)
    ADD_TEST(testComparisons testComparisons)
    SET_TARGET_PROPERTIES(testComparisons PROPERTIES FOLDER "src/tests")


    SET(tests_testExtinction_SRCS
        tests/testExtinction.hpp
        tests/testExtinction.cpp
    )
    ADD_EXECUTABLE(testExtinction ${tests_testExtinction_SRCS})
    TARGET_LINK_LIBRARIES(testExtinction ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testExtinction)
    ADD_TEST(testExtinction testExtinction)
    SET_TARGET_PROPERTIES(testExtinction PROPERTIES FOLDER "src/tests")

    SET(tests_testRefraction_SRCS
        tests/testRefraction.hpp
        tests/testRefraction.cpp
    )
    ADD_EXECUTABLE(testRefraction ${tests_testRefraction_SRCS})
    TARGET_LINK_LIBRARIES(testRefraction ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testRefraction)
    ADD_TEST(testRefraction testRefraction)
    SET_TARGET_PROPERTIES(testRefraction PROPERTIES FOLDER "src/tests")

    SET(tests_testAirmass_SRCS
        tests/testAirmass.hpp
        tests/testAirmass.cpp
    )
    ADD_EXECUTABLE(testAirmass ${tests_testAirmass_SRCS})
    TARGET_LINK_LIBRARIES(testAirmass ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testAirmass)
    ADD_TEST(testAirmass testAirmass)
    SET_TARGET_PROPERTIES(testAirmass PROPERTIES FOLDER "src/tests")

    SET(tests_testGeomMath_SRCS
        tests/testGeomMath.hpp
        tests/testGeomMath.cpp
    )
    ADD_EXECUTABLE(testGeomMath ${tests_testGeomMath_SRCS})
    TARGET_LINK_LIBRARIES(testGeomMath ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testGeomMath)
    ADD_TEST(testGeomMath testGeomMath)
    SET_TARGET_PROPERTIES(testGeomMath PROPERTIES FOLDER "src/tests")

    SET(tests_testVecMath_SRCS
        tests/testVecMath.hpp
        tests/testVecMath.cpp
    )
    ADD_EXECUTABLE(testVecMath ${tests_testVecMath_SRCS})
    TARGET_LINK_LIBRARIES(testVecMath ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testVecMath)
    ADD_TEST(testVecMath testVecMath)
    SET_TARGET_PROPERTIES(testVecMath PROPERTIES FOLDER "src/tests")

    IF(ENABLE_SCRIPTING)
        SET(tests_testJavaScripting_SRCS
            tests/testJavaScripting.hpp
            tests/testJavaScripting.cpp
        )
        ADD_EXECUTABLE(testJavaScripting ${tests_testJavaScripting_SRCS})
        TARGET_LINK_LIBRARIES(testJavaScripting ${TESTS_LIBRARIES})
        ADD_DEPENDENCIES(buildTests testJavaScripting)
        ADD_TEST(testJavaScripting testJavaScripting)
        SET_TARGET_PROPERTIES(testJavaScripting PROPERTIES FOLDER "src/tests")
    ENDIF(ENABLE_SCRIPTING)
	
    SET(tests_testStelProjector_SRCS
        tests/testStelProjector.hpp
        tests/testStelProjector.cpp
    )
    ADD_EXECUTABLE(testStelProjector ${tests_testStelProjector_SRCS})
    TARGET_LINK_LIBRARIES(testStelProjector ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelProjector)
    ADD_TEST(testStelProjector testStelProjector)
    SET_TARGET_PROPERTIES(testStelProjector PROPERTIES FOLDER "src/tests")

    SET(tests_testPrecession_SRCS
        tests/testPrecession.hpp
        tests/testPrecession.cpp
    )
    ADD_EXECUTABLE(testPrecession ${tests_testPrecession_SRCS})
    TARGET_LINK_LIBRARIES(testPrecession ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testPrecession)
    ADD_TEST(testPrecession testPrecession)
    SET_TARGET_PROPERTIES(testPrecession PROPERTIES FOLDER "src/tests")

    SET(tests_testSiderealTime_SRCS
        tests/testSiderealTime.hpp
        tests/testSiderealTime.cpp
    )
    ADD_EXECUTABLE(testSiderealTime ${tests_testSiderealTime_SRCS})
    TARGET_LINK_LIBRARIES(testSiderealTime ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testSiderealTime)
    ADD_TEST(testSiderealTime testSiderealTime)
    SET_TARGET_PROPERTIES(testSiderealTime PROPERTIES FOLDER "src/tests")

    SET(tests_testEphemeris_SRCS
        tests/testEphemeris.hpp
        tests/testEphemeris.cpp
    )
    ADD_EXECUTABLE(testEphemeris ${tests_testEphemeris_SRCS})
    TARGET_LINK_LIBRARIES(testEphemeris ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testEphemeris)
    ADD_TEST(testEphemeris testEphemeris)
    SET_TARGET_PROPERTIES(testEphemeris PROPERTIES FOLDER "src/tests")
    SET_TESTS_PROPERTIES(testEphemeris PROPERTIES
        ENVIRONMENT "STELLARIUM_DATA_ROOT=${PROJECT_SOURCE_DIR}")

    SET(tests_testStelSkyCultureMgr_SRCS
        tests/testStelSkyCultureMgr.hpp
        tests/testStelSkyCultureMgr.cpp
    )
    ADD_EXECUTABLE(testStelSkyCultureMgr ${tests_testStelSkyCultureMgr_SRCS})
    TARGET_LINK_LIBRARIES(testStelSkyCultureMgr ${TESTS_LIBRARIES})
    ADD_DEPENDENCIES(buildTests testStelSkyCultureMgr)
    ADD_TEST(testStelSkyCultureMgr testStelSkyCultureMgr)
    SET_TARGET_PROPERTIES(testStelSkyCultureMgr PROPERTIES FOLDER "src/tests")
    SET_TESTS_PROPERTIES(testStelSkyCultureMgr PROPERTIES
        ENVIRONMENT "STELLARIUM_DATA_ROOT=${PROJECT_SOURCE_DIR}")

ENDIF (ENABLE_TESTING)
