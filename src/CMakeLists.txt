SET(glues_lib_SRCS
	core/external/glues_stel/source/glues_error.c
	core/external/glues_stel/source/glues_error.h
	core/external/glues_stel/source/glues.h
	core/external/glues_stel/source/libtess/dict.h
	core/external/glues_stel/source/libtess/dict.c
	core/external/glues_stel/source/libtess/dict-list.h
	core/external/glues_stel/source/libtess/geom.c
	core/external/glues_stel/source/libtess/geom.h
	core/external/glues_stel/source/libtess/memalloc.c
	core/external/glues_stel/source/libtess/memalloc.h
	core/external/glues_stel/source/libtess/mesh.c
	core/external/glues_stel/source/libtess/mesh.h
	core/external/glues_stel/source/libtess/normal.c
	core/external/glues_stel/source/libtess/normal.h
	core/external/glues_stel/source/libtess/priorityq.c
	core/external/glues_stel/source/libtess/priorityq.h
	core/external/glues_stel/source/libtess/priorityq-heap.h
	core/external/glues_stel/source/libtess/priorityq-heap.i
	core/external/glues_stel/source/libtess/priorityq-sort.h
	core/external/glues_stel/source/libtess/render.c
	core/external/glues_stel/source/libtess/render.h
	core/external/glues_stel/source/libtess/sweep.c
	core/external/glues_stel/source/libtess/sweep.h
	core/external/glues_stel/source/libtess/tess.c
	core/external/glues_stel/source/libtess/tess.h
	core/external/glues_stel/source/libtess/tessmono.c
	core/external/glues_stel/source/libtess/tessmono.h
)

SET(zlib_SRCS
	core/external/zlib/adler32.c
	core/external/zlib/compress.c
	core/external/zlib/crc32.c
	core/external/zlib/crc32.h
	core/external/zlib/deflate.c
	core/external/zlib/deflate.h
	core/external/zlib/gzclose.c
	core/external/zlib/gzguts.h
	core/external/zlib/gzlib.c
	core/external/zlib/gzread.c
	core/external/zlib/gzwrite.c
	core/external/zlib/infback.c
	core/external/zlib/inffast.c
	core/external/zlib/inffast.h
	core/external/zlib/inffixed.h
	core/external/zlib/inflate.c
	core/external/zlib/inflate.h
	core/external/zlib/inftrees.c
	core/external/zlib/inftrees.h
	core/external/zlib/trees.c
	core/external/zlib/trees.h
	core/external/zlib/uncompr.c
	core/external/zlib/zconf.h
	core/external/zlib/zlib.h
	core/external/zlib/zutil.c
	core/external/zlib/zutil.h
)

SET(stellarium_lib_SRCS
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
	core/StelSkyPolygon.hpp
	core/StelSkyPolygon.cpp
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
	core/StelActionMgr.hpp
	core/StelActionMgr.cpp
	core/StelProgressController.hpp

	${gl_lib_SRCS}

	${glues_lib_SRCS}

	core/planetsephems/calc_interpolated_elements.c
	core/planetsephems/calc_interpolated_elements.h
	core/planetsephems/elliptic_to_rectangular.c
	core/planetsephems/elliptic_to_rectangular.h
	core/planetsephems/elp82b.c
	core/planetsephems/elp82b.h
	core/planetsephems/gust86.c
	core/planetsephems/gust86.h
	core/planetsephems/l1.c
	core/planetsephems/l1.h
	core/planetsephems/marssat.c
	core/planetsephems/marssat.h
	core/planetsephems/pluto.c
	core/planetsephems/pluto.h
	core/planetsephems/sidereal_time.c
	core/planetsephems/sidereal_time.h
	core/planetsephems/stellplanet.c
	core/planetsephems/stellplanet.h
	core/planetsephems/tass17.c
	core/planetsephems/tass17.h
	core/planetsephems/vsop87.c
	core/planetsephems/vsop87.h

	core/modules/Atmosphere.cpp
	core/modules/Atmosphere.hpp
	core/modules/Constellation.cpp
	core/modules/Constellation.hpp
	core/modules/ConstellationMgr.cpp
	core/modules/ConstellationMgr.hpp
	core/modules/GridLinesMgr.cpp
	core/modules/GridLinesMgr.hpp
	core/modules/LabelMgr.hpp
	core/modules/LabelMgr.cpp
	core/modules/Landscape.cpp
	core/modules/Landscape.hpp
	core/modules/LandscapeMgr.cpp
	core/modules/LandscapeMgr.hpp
	core/modules/Meteor.cpp
	core/modules/Meteor.hpp
	core/modules/MeteorMgr.cpp
	core/modules/MeteorMgr.hpp
	core/modules/MilkyWay.cpp
	core/modules/MilkyWay.hpp
	core/modules/Nebula.cpp
	core/modules/Nebula.hpp
	core/modules/NebulaMgr.cpp
	core/modules/NebulaMgr.hpp
	core/modules/Orbit.cpp
	core/modules/Orbit.hpp
	core/modules/Planet.cpp
	core/modules/Planet.hpp
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
	core/modules/Solve.hpp
	core/modules/Star.cpp
	core/modules/Star.hpp
	core/modules/StarMgr.cpp
	core/modules/StarMgr.hpp
	core/modules/StarWrapper.cpp
	core/modules/StarWrapper.hpp
	core/modules/ZoneArray.cpp
	core/modules/ZodiacalLight.hpp
	core/modules/ZodiacalLight.cpp
	core/modules/ZoneArray.hpp
	core/modules/ZoneData.hpp
	StelMainView.hpp
	StelMainView.cpp
	StelLogger.hpp
	StelLogger.cpp
	CLIProcessor.hpp
	CLIProcessor.cpp
	translations.h
)

SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
        core/external/qtcompress/qzip.cpp
        core/external/qtcompress/qzipreader.h
        core/external/qtcompress/qzipwriter.h
)

IF(ENABLE_SCRIPTING)
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
		scripting/StelScriptOutput.hpp
		scripting/StelScriptOutput.cpp
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

IF(WIN32)
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
		${zlib_SRCS}
	)
ENDIF()

SET(stellarium_exe_SRCS main.cpp)

################# compiles resources files ############
SET(stellarium_RES ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
QT5_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})


#############################################################################################
############################# Standard GUI plugin compilation ###############################
#############################################################################################
IF(GUI_MODE STREQUAL "Standard")
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
		gui/AddRemoveLandscapesDialog.hpp
		gui/AddRemoveLandscapesDialog.cpp
		gui/AtmosphereDialog.hpp
		gui/AtmosphereDialog.cpp
		gui/CustomDeltaTEquationDialog.hpp
		gui/CustomDeltaTEquationDialog.cpp
		gui/StelDialog.hpp
		gui/StelDialog.cpp)

	################# compiles .ui files ############
	SET(stellarium_UIS
	gui/locationDialogGui.ui
	gui/helpDialogGui.ui
	gui/shortcutsDialog.ui
	gui/dateTimeDialogGui.ui
	gui/viewDialog.ui
	gui/searchDialogGui.ui
	gui/configurationDialog.ui
	gui/AtmosphereDialog.ui
	gui/CustomDeltaTEquationDialog.ui
	gui/addRemoveLandscapesDialog.ui)

        IF(ENABLE_SCRIPTING)
            IF(ENABLE_SCRIPT_CONSOLE)
		SET(stellarium_gui_SRCS ${stellarium_gui_SRCS}
			gui/ScriptConsole.hpp
			gui/ScriptConsole.cpp
			gui/StelScriptSyntaxHighlighter.hpp
			gui/StelScriptSyntaxHighlighter.cpp)
		SET(stellarium_UIS ${stellarium_UIS}
			gui/scriptConsole.ui)
            ENDIF()
        ENDIF()

	QT5_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})

	# compiles resources files
	SET(stellarium_gui_RES ${CMAKE_SOURCE_DIR}/data/gui/guiRes.qrc)
	QT5_ADD_RESOURCES(stellarium_gui_RES_CXX ${stellarium_gui_RES})

	SET(extLinkerOption ${extLinkerOption} ${QT_QTSCRIPT_LIBRARY} ${OPENGL_LIBRARIES})
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS} ${stellarium_gui_SRCS} ${stellarium_UIS_H} ${stellarium_gui_RES_CXX})
ENDIF()

#############################################################################################
################################ No GUI plugin compilation ##################################
#############################################################################################
IF (GUI_MODE STREQUAL "None")
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

IF(USE_PLUGIN_LOGBOOK)
	SET(QT_USE_QTSQL TRUE)
ENDIF()

IF(USE_PLUGIN_TELESCOPECONTROL)
# WinSock library
    IF(WIN32)
        SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ws2_32)
    ENDIF()
ENDIF()


#############################################################################################
################################ Build main program #########################################
#############################################################################################

SET(extLinkerOption ${extLinkerOption} ${OPENGL_LIBRARIES} ${ZLIB_LIBRARIES})
IF(WIN32 AND NOT MSVC)
# TODO: Test if winsock is still necessary if the Telescope Control plug-in is disabled.
# Because including it here seems to duplicate STELLARIUM_STATIC_PLUGINS_LIBRARIES
	SET(extLinkerOption ${extLinkerOption} libwinmm.a -lws2_32)
ENDIF()

IF(WIN32 AND ENABLE_WINDOWS_EXE_ICON)
    IF(RC_COMPILER_PATH)
        ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/stellarium-rc.o
                           COMMAND ${RC_COMPILER_PATH} ${CMAKE_SOURCE_DIR}/data/stellarium.rc ${CMAKE_BINARY_DIR}/stellarium-rc.o
                           WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data)
        SET(stellarium_exe_SRCS ${stellarium_exe_SRCS} ${CMAKE_BINARY_DIR}/stellarium-rc.o)
    ELSE()
        IF(MSVC)
            SET(stellarium_exe_SRCS ${stellarium_exe_SRCS} ${CMAKE_SOURCE_DIR}/data/stellarium.rc)
        ENDIF()
    ENDIF()
ENDIF()

IF(GENERATE_STELMAINLIB)
    ADD_LIBRARY(stelMain SHARED ${stellarium_lib_SRCS} ${stellarium_RES_CXX})
    TARGET_LINK_LIBRARIES(stelMain ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
    QT5_USE_MODULES(stelMain Core Concurrent Declarative Gui Network OpenGL Script Widgets)
    IF(ENABLE_SOUND)
        QT5_USE_MODULES(stelMain Multimedia)
    ENDIF()
    INSTALL(TARGETS stelMain DESTINATION lib)
    ADD_EXECUTABLE(stellarium ${stellarium_exe_SRCS})
    #stelMain should be after the plug-ins, otherwise the build crashes
    TARGET_LINK_LIBRARIES(stellarium ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} stelMain ${extLinkerOption})
ELSE()
    ADD_EXECUTABLE(stellarium ${stellarium_lib_SRCS} ${stellarium_RES_CXX} ${stellarium_exe_SRCS})
    TARGET_LINK_LIBRARIES(stellarium ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
    # This will add the ANGLE lib if the Qt version supports it.
    TARGET_LINK_LIBRARIES(stellarium ${Qt5Gui_LIBRARIES} ${Qt5Gui_OPENGL_LIBRARIES})

    QT5_USE_MODULES(stellarium Core Concurrent Declarative Gui Network OpenGL Script Widgets)
    IF(ENABLE_SOUND)
        QT5_USE_MODULES(stellarium Multimedia)
    ENDIF()

    IF(APPLE)
        SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -framework Cocoa -framework OpenGL")
    ENDIF()
ENDIF()

ADD_DEPENDENCIES(stellarium AllStaticPlugins)

INSTALL(TARGETS stellarium DESTINATION bin)



#############################################################################################
################################## Build unit tests #########################################
#############################################################################################

SET(STELLARIUM_TESTS)
MACRO(ADD_TEST NAME)
	SET(STELLARIUM_TESTS ${STELLARIUM_TESTS} ${NAME})
ENDMACRO()

SET(extLinkerOptionTest ${QT_QTCORE_LIBRARY} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTOPENGL_LIBRARY} ${OPENGL_LIBRARIES} ${ZLIB_LIBRARIES})

# Custom target used to build all tests at once
ADD_CUSTOM_TARGET(buildTests)

SET(tests_testDates_SRCS
	tests/testDates.hpp
	tests/testDates.cpp
	core/StelUtils.hpp
	core/StelUtils.cpp)
IF(WIN32)
	# StelUtils required zlib sources
	SET(tests_testDates_SRCS ${tests_testDates_SRCS} ${zlib_SRCS})
ENDIF()
ADD_EXECUTABLE(testDates EXCLUDE_FROM_ALL ${tests_testDates_SRCS})
QT5_USE_MODULES(testDates Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testDates ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testDates)
ADD_TEST(testDates)

SET(tests_testStelFileMgr_SRCS
	tests/testStelFileMgr.hpp
	tests/testStelFileMgr.cpp
	core/StelFileMgr.hpp
	core/StelFileMgr.cpp)
ADD_EXECUTABLE(testStelFileMgr EXCLUDE_FROM_ALL ${tests_testStelFileMgr_SRCS})
QT5_USE_MODULES(testStelFileMgr Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testStelFileMgr ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelFileMgr)
ADD_TEST(testStelFileMgr)

SET(tests_testStelSphereGeometry_SRCS
	tests/testStelSphereGeometry.hpp
	tests/testStelSphereGeometry.cpp
	core/StelSphereGeometry.hpp
	core/StelSphereGeometry.cpp
	core/StelVertexArray.hpp
	core/StelVertexArray.cpp
	core/OctahedronPolygon.hpp
	core/OctahedronPolygon.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp
	core/StelUtils.hpp
	core/StelUtils.cpp
	core/StelProjector.hpp
	core/StelProjector.cpp
	core/StelFileMgr.hpp
	core/StelFileMgr.cpp
	core/StelTranslator.hpp
	core/StelTranslator.cpp
	${glues_lib_SRCS})
IF(WIN32)
	# StelUtils required zlib sources
	SET(tests_testStelSphereGeometry_SRCS ${tests_testStelSphereGeometry_SRCS} ${zlib_SRCS})
ENDIF()
ADD_EXECUTABLE(testStelSphereGeometry EXCLUDE_FROM_ALL ${tests_testStelSphereGeometry_SRCS})
QT5_USE_MODULES(testStelSphereGeometry Core Gui OpenGL Test)
TARGET_LINK_LIBRARIES(testStelSphereGeometry ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelSphereGeometry)
ADD_TEST(testStelSphereGeometry)

#SET(tests_testStelSphericalIndex_SRCS
#	tests/testStelSphericalIndex.hpp
#	tests/testStelSphericalIndex.cpp
#	core/StelSphericalIndex.hpp
#	core/StelSphericalIndex.cpp
#	core/StelSphereGeometry.hpp
#	core/StelSphereGeometry.cpp
#	core/StelVertexArray.hpp
#	core/StelVertexArray.cpp
#	core/OctahedronPolygon.hpp
#	core/OctahedronPolygon.cpp
#	core/StelJsonParser.hpp
#	core/StelJsonParser.cpp
#	core/StelUtils.cpp
#	core/StelUtils.hpp
#	core/StelProjector.cpp
#	core/StelProjector.hpp
#	core/StelTranslator.cpp
#	core/StelTranslator.hpp
#	${glues_lib_SRCS})
#ADD_EXECUTABLE(testStelSphericalIndex EXCLUDE_FROM_ALL ${tests_testStelSphericalIndex_SRCS})
#QT5_USE_MODULES(testStelSphericalIndex Core Gui Widgets OpenGL Script Declarative Test)
#TARGET_LINK_LIBRARIES(testStelSphericalIndex ${extLinkerOptionTest})
#ADD_DEPENDENCIES(buildTests testStelSphericalIndex)

SET(tests_testStelJsonParser_SRCS
	tests/testStelJsonParser.hpp
	tests/testStelJsonParser.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp)
ADD_EXECUTABLE(testStelJsonParser EXCLUDE_FROM_ALL ${tests_testStelJsonParser_SRCS})
QT5_USE_MODULES(testStelJsonParser Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testStelJsonParser ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelJsonParser)
ADD_TEST(testStelJsonParser)

SET(tests_testStelVertexArray_SRCS
	tests/testStelVertexArray.hpp
	tests/testStelVertexArray.cpp
	core/StelVertexArray.hpp)
ADD_EXECUTABLE(testStelVertexArray EXCLUDE_FROM_ALL ${tests_testStelVertexArray_SRCS})
QT5_USE_MODULES(testStelVertexArray Core Gui Test OpenGL)
TARGET_LINK_LIBRARIES(testStelVertexArray ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelVertexArray)
ADD_TEST(testStelVertexArray)

SET(tests_testDeltaT_SRCS
	tests/testDeltaT.hpp
	tests/testDeltaT.cpp
	core/StelUtils.hpp
	core/StelUtils.cpp)
IF(WIN32)
	# StelUtils required zlib sources
	SET(tests_testDeltaT_SRCS ${tests_testDeltaT_SRCS} ${zlib_SRCS})
ENDIF()
ADD_EXECUTABLE(testDeltaT EXCLUDE_FROM_ALL ${tests_testDeltaT_SRCS})
QT5_USE_MODULES(testDeltaT Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testDeltaT ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testDeltaT)
ADD_TEST(testDeltaT)

SET(tests_testConversions_SRCS
	tests/testConversions.hpp
	tests/testConversions.cpp
	core/StelUtils.hpp
	core/StelUtils.cpp)
IF(WIN32)
	# StelUtils required zlib sources
	SET(tests_testConversions_SRCS ${tests_testConversions_SRCS} ${zlib_SRCS})
ENDIF()
ADD_EXECUTABLE(testConversions EXCLUDE_FROM_ALL ${tests_testConversions_SRCS})
QT5_USE_MODULES(testConversions Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testConversions ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testConversions)
ADD_TEST(testConversions)

SET(tests_testExtinction_SRCS
        tests/testExtinction.hpp
        tests/testExtinction.cpp
        core/RefractionExtinction.hpp
        core/RefractionExtinction.cpp)
ADD_EXECUTABLE(testExtinction EXCLUDE_FROM_ALL ${tests_testExtinction_SRCS})
QT5_USE_MODULES(testExtinction Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testExtinction ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testExtinction)
ADD_TEST(testExtinction)

SET(tests_testRefraction_SRCS
        tests/testRefraction.hpp
        tests/testRefraction.cpp
        core/RefractionExtinction.hpp
        core/RefractionExtinction.cpp
        core/StelUtils.hpp
        core/StelUtils.cpp)
IF(WIN32)
        # StelUtils required zlib sources
        SET(tests_testRefraction_SRCS ${tests_testRefraction_SRCS} ${zlib_SRCS})
ENDIF()
ADD_EXECUTABLE(testRefraction EXCLUDE_FROM_ALL ${tests_testRefraction_SRCS})
QT5_USE_MODULES(testRefraction Core Gui Widgets Script Declarative Test)
TARGET_LINK_LIBRARIES(testRefraction ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testRefraction)
ADD_TEST(testRefraction)

ADD_CUSTOM_TARGET(tests COMMENT "Run the Stellarium unit tests")
FOREACH(NAME ${STELLARIUM_TESTS})
	IF(MSVC)
		ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./${CMAKE_BUILD_TYPE}/${NAME}.exe WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src)
	ELSE()
		ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./${NAME} WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src)
	ENDIF()
ENDFOREACH()
ADD_DEPENDENCIES(tests buildTests)

