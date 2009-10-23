

INCLUDE_DIRECTORIES(
	${CMAKE_BINARY_DIR}
	${CMAKE_SOURCE_DIR}/src
	${CMAKE_SOURCE_DIR}/src/core
	${CMAKE_SOURCE_DIR}/src/gui
	${CMAKE_SOURCE_DIR}/src/modules
	${CMAKE_SOURCE_DIR}/src/planetsephems
	${CMAKE_SOURCE_DIR}/src/external
	${CMAKE_SOURCE_DIR}/src/scripting
	${CMAKE_SOURCE_DIR}/src/external/kfilter
	${CMAKE_SOURCE_DIR}/src/external/glues_stel/source/
	${CMAKE_SOURCE_DIR}/src/external/glues_stel/source/libtess
	${OPENGL_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${ICONV_INCLUDE_DIR}
	${INTL_INCLUDE_DIR}
	${QT_INCLUDE_DIR}/QtScript
	${QT_QTSCRIPT_INCLUDE_DIR}
	${CMAKE_BINARY_DIR}/src
	${QT_QTTEST_INCLUDE_DIR}
)

########### next target ###############

SET(glues_lib_SRCS
	external/glues_stel/source/glues_error.c
	external/glues_stel/source/glues_error.h
	external/glues_stel/source/glues.h
	external/glues_stel/source/glues_mipmap.c
	external/glues_stel/source/glues_mipmap.h
	external/glues_stel/source/libtess/dict.h
	external/glues_stel/source/libtess/dict.c
	external/glues_stel/source/libtess/dict-list.h
	external/glues_stel/source/libtess/geom.c
	external/glues_stel/source/libtess/geom.h
	external/glues_stel/source/libtess/memalloc.c
	external/glues_stel/source/libtess/memalloc.h
	external/glues_stel/source/libtess/mesh.c
	external/glues_stel/source/libtess/mesh.h
	external/glues_stel/source/libtess/normal.c
	external/glues_stel/source/libtess/normal.h
	external/glues_stel/source/libtess/priorityq.c
	external/glues_stel/source/libtess/priorityq.h
	external/glues_stel/source/libtess/priorityq-heap.h
	external/glues_stel/source/libtess/priorityq-heap.i
	external/glues_stel/source/libtess/priorityq-sort.h
	external/glues_stel/source/libtess/render.c
	external/glues_stel/source/libtess/render.h
	external/glues_stel/source/libtess/sweep.c
	external/glues_stel/source/libtess/sweep.h
	external/glues_stel/source/libtess/tess.c
	external/glues_stel/source/libtess/tess.h
	external/glues_stel/source/libtess/tessmono.c
	external/glues_stel/source/libtess/tessmono.h
	)

IF (OPENGL_MODE STREQUAL "Desktop")
  SET(gl_lib_SRCS external/GLee.c	external/GLee.h)
ENDIF()
IF(OPENGL_MODE STREQUAL "ES2")
  SET(gl_lib_SRCS external/GLee.h)
ENDIF()

SET(stellarium_lib_SRCS
	core/StelAudioMgr.hpp
	core/StelAudioMgr.cpp
	core/StelDownloadMgr.hpp
	core/StelDownloadMgr.cpp
	core/StelGeodesicGrid.cpp
	core/StelGeodesicGrid.hpp
	core/StelLoadingBar.cpp
	core/StelLoadingBar.hpp
	core/StelMovementMgr.cpp
	core/StelMovementMgr.hpp
	core/StelNavigator.cpp
	core/StelNavigator.hpp
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
	core/StelAppGraphicsWidget.cpp
	core/StelAppGraphicsWidget.hpp
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
	core/StelTexture.cpp
	core/StelTexture.hpp
	core/StelTextureTypes.hpp
	core/StelToneReproducer.cpp
	core/StelToneReproducer.hpp
	core/StelViewportDistorter.cpp
	core/StelViewportDistorter.hpp
	core/StelStyle.hpp
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
	core/StelGuiBase.hpp
	core/StelGuiBase.cpp

	external/bytes.h
	external/fixx11h.h
	external/gettext.h
		${gl_lib_SRCS}

	external/kfilter/kfilterdev.h
	external/kfilter/kfilterdev.cpp
	external/kfilter/kfilterbase.h
	external/kfilter/kfilterbase.cpp
	external/kfilter/kgzipfilter.h
	external/kfilter/kgzipfilter.cpp
	external/kfilter/kzip.h
	external/kfilter/kzip.cpp
	external/kfilter/klimitediodevice.h
	external/kfilter/klimitediodevice.cpp
	external/kfilter/karchive.h
	external/kfilter/karchive.cpp

	${glues_lib_SRCS}

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
	modules/LabelMgr.hpp
	modules/LabelMgr.cpp
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
	modules/ScreenImageMgr.hpp
	modules/ScreenImageMgr.cpp
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
	modules/Telescope.cpp
	modules/Telescope.hpp
	modules/TelescopeMgr.cpp
	modules/TelescopeMgr.hpp
#	modules/TextUI.cpp
#	modules/TextUI.hpp
	modules/ZoneArray.cpp
	modules/ZoneArray.hpp
	modules/ZoneData.hpp

	scripting/StelScriptMgr.cpp
	scripting/StratoscriptPreprocessor.cpp
	scripting/StelScriptMgr.hpp
	scripting/ScriptSleeper.cpp
	scripting/ScriptSleeper.hpp
	scripting/StelMainScriptAPI.cpp
	scripting/StelMainScriptAPI.hpp
	scripting/StelMainScriptAPIProxy.cpp
	scripting/StelMainScriptAPIProxy.hpp

	StelMainGraphicsView.hpp
	StelMainGraphicsView.cpp
	StelMainWindow.hpp
	StelMainWindow.cpp
	translations.h
)

IF(WIN32)
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
		external/kdewin32/basetyps.h
		external/kdewin32/byteswap.h
		external/kdewin32/comcat.h
		external/kdewin32/dirent.h
		external/kdewin32/docobj.h
		external/kdewin32/errno.h
		external/kdewin32/fcntl.h
		external/kdewin32/grp.h
		external/kdewin32/ifaddrs.h
		external/kdewin32/langinfo.h
		external/kdewin32/mshtml.h
		external/kdewin32/netdb.h
		external/kdewin32/nl_types.h
		external/kdewin32/oaidl.h
		external/kdewin32/objfwd.h
		external/kdewin32/objidl.h
		external/kdewin32/ocidl.h
		external/kdewin32/olectl.h
		external/kdewin32/oleidl.h
		external/kdewin32/pwd.h
		external/kdewin32/signal.h
		external/kdewin32/stdio.h
		external/kdewin32/stdlib.h
		external/kdewin32/string.h
		external/kdewin32/strings.h
		external/kdewin32/syslog.h
		external/kdewin32/time.h
		external/kdewin32/unistd.h
		external/kdewin32/unknwn.h
		external/kdewin32/wchar.h
		external/kdewin32/arpa/inet.h
		external/kdewin32/asm/byteorder.h
		external/kdewin32/net/if.h
		external/kdewin32/netinet/in.h
		external/kdewin32/netinet/tcp.h
		external/kdewin32/sys/ioctl.h
		external/kdewin32/sys/mman.h
		external/kdewin32/sys/resource.h
		external/kdewin32/sys/select.h
		external/kdewin32/sys/signal.h
		external/kdewin32/sys/socket.h
		external/kdewin32/sys/stat.h
		external/kdewin32/sys/time.h
		external/kdewin32/sys/times.h
		external/kdewin32/sys/types.h
		external/kdewin32/sys/uio.h
		external/kdewin32/sys/un.h
		external/kdewin32/sys/unistd.h
		external/kdewin32/sys/utsname.h
		external/kdewin32/sys/wait.h

		external/kdewin32/bind/inet_ntop.c
		external/kdewin32/bind/inet_pton.c
		external/kdewin32/dirent.c
		external/kdewin32/errno.c
		external/kdewin32/fsync.c
		external/kdewin32/getenv.c
		external/kdewin32/grp.c
		external/kdewin32/inet.c
		external/kdewin32/mmap.c
		external/kdewin32/nl_langinfo.c
		external/kdewin32/net.c
		external/kdewin32/pwd.c
		external/kdewin32/realpath.c
		external/kdewin32/resource.c
		external/kdewin32/signal.c
		external/kdewin32/stdlib.c
		external/kdewin32/string.c
		external/kdewin32/strptime.c
		external/kdewin32/syslog.c
		external/kdewin32/time.c
		external/kdewin32/uname.c
		external/kdewin32/unistd.c
	)
ENDIF(WIN32)

IF(APPLE)
  SET(stellarium_lib_SRCS
	${stellarium_lib_SRCS}
	core/StelMacosxDirs.cpp
	core/StelMacosxDirs.hpp)
ENDIF(APPLE)

SET(stellarium_exe_SRCS
	main.cpp
)

################# compiles resources files ############
SET(stellarium_RES
  ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
QT4_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})

################# compiles moc files ############
# Add here all the files which require moc compilation
# that is, all the headers with SIGNAL/SLOTS/PLUGIN_INTERFACE etc..
SET(stellarium_MOC_HDRS
	core/StelAudioMgr.hpp
	core/StelDownloadMgr.hpp
	core/StelModuleMgr.hpp
	core/StelObserver.hpp
	core/StelApp.hpp
	core/StelNavigator.hpp
	core/StelLocationMgr.hpp
	core/StelCore.hpp
	core/StelAppGraphicsWidget.hpp
	core/StelTexture.hpp
	core/StelMovementMgr.hpp
	core/MultiLevelJsonBase.hpp
	core/StelSkyImageTile.hpp
	core/StelSkyPolygon.hpp
	core/StelSkyDrawer.hpp
	core/StelSkyLayerMgr.hpp
	core/StelSkyLayer.hpp
	core/StelSkyCultureMgr.hpp
	core/SimbadSearcher.hpp
	modules/ConstellationMgr.hpp
	modules/GridLinesMgr.hpp
	modules/LabelMgr.hpp
	modules/LandscapeMgr.hpp
	modules/MeteorMgr.hpp
	modules/MilkyWay.hpp
	modules/NebulaMgr.hpp
	modules/ScreenImageMgr.hpp
	modules/SolarSystem.hpp
	modules/StarMgr.hpp
	modules/Telescope.hpp
	modules/TelescopeMgr.hpp
	scripting/StelScriptMgr.hpp
	scripting/StelMainScriptAPI.hpp
	scripting/StelMainScriptAPIProxy.hpp
	StelMainGraphicsView.hpp
	StelMainWindow.hpp
)
# After this call, stellarium_MOC_SRCS = moc_stellarium.cxx
QT4_WRAP_CPP(stellarium_MOC_SRCS ${stellarium_MOC_HDRS})


#############################################################################################
############################# Standard GUI plugin compilation ###############################
#############################################################################################
IF (GUI_MODE STREQUAL "Standard")
	SET(stellarium_gui_SRCS
		gui/StelGui.hpp
		gui/StelGui.cpp
		gui/SkyGui.hpp
		gui/SkyGui.cpp
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
		gui/StelDialog.cpp)

	################# compiles GUI moc files ############
	SET(stellarium_gui_MOC_HDRS
		gui/StelGui.hpp
		gui/SkyGui.hpp
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
		gui/StelDialog.hpp)

	################# compiles .ui files ############
	SET(stellarium_UIS
	gui/locationDialogGui.ui
	gui/helpDialogGui.ui
	gui/dateTimeDialogGui.ui
	gui/viewDialog.ui
	gui/searchDialogGui.ui
	gui/configurationDialog.ui)

	IF(ENABLE_SCRIPT_CONSOLE)
		SET(stellarium_gui_SRCS ${stellarium_gui_SRCS}
			gui/ScriptConsole.hpp
			gui/ScriptConsole.cpp
			gui/StelScriptSyntaxHighlighter.hpp
			gui/StelScriptSyntaxHighlighter.cpp)
		SET(stellarium_gui_MOC_HDRS ${stellarium_gui_MOC_HDRS}
			gui/ScriptConsole.hpp
			gui/StelScriptSyntaxHighlighter.hpp)
		SET(stellarium_UIS ${stellarium_UIS}
			gui/scriptConsole.ui)
	ENDIF(ENABLE_SCRIPT_CONSOLE)

	QT4_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})
	# Dummy target depending on ${stellarium_UIS_H} to trigger ui compilations
	ADD_CUSTOM_TARGET(GenerateUiHeaders ALL DEPENDS ${stellarium_UIS_H})

	# After this call, stellarium_gui_MOC_SRCS = moc_stellarium_gui.cxx
	QT4_WRAP_CPP(stellarium_gui_MOC_SRCS ${stellarium_gui_MOC_HDRS})

	# Compile the static library
	ADD_LIBRARY(StelGuiLib STATIC ${stellarium_gui_SRCS} ${stellarium_gui_MOC_SRCS})
	TARGET_LINK_LIBRARIES(StelGuiLib ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(StelGuiLib PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/libStelGuiLib.a)
ENDIF()

#############################################################################################
############################ Standard NO GUI plugin compilation #############################
#############################################################################################
IF (GUI_MODE STREQUAL "None")
	SET(stellarium_nogui_SRCS
		noGui/StelNoGui.hpp
		noGui/StelNoGui.cpp)
	SET(stellarium_nogui_MOC_HDRS
		noGui/StelNoGui.hpp)
	QT4_WRAP_CPP(stellarium_nogui_MOC_SRCS ${stellarium_nogui_MOC_HDRS})
	ADD_LIBRARY(StelNoGuiLib STATIC ${stellarium_nogui_SRCS} ${stellarium_nogui_MOC_SRCS})
	TARGET_LINK_LIBRARIES(StelNoGuiLib ${extLinkerOption})
	SET_TARGET_PROPERTIES(StelNoGuiLib PROPERTIES COMPILE_FLAGS "-fPIC -DQT_STATICPLUGIN")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/libStelNoGuiLib.a)
ENDIF()

#############################################################################################
################################ Build main program #########################################
#############################################################################################

SET(extLinkerOption ${QT_LIBRARIES} ${QT_QTSCRIPT_LIBRARY} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} )

IF(GENERATE_STELMAINLIB)
 ADD_LIBRARY(stelMain SHARED ${stellarium_lib_SRCS} ${stellarium_MOC_SRCS} ${stellarium_RES_CXX})
 TARGET_LINK_LIBRARIES(stelMain ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
 INSTALL(TARGETS stelMain DESTINATION lib)
 ADD_EXECUTABLE(stellarium ${stellarium_exe_SRCS})
 TARGET_LINK_LIBRARIES(stellarium stelMain ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
ELSE()
 ADD_EXECUTABLE(stellarium ${stellarium_lib_SRCS} ${stellarium_MOC_SRCS} ${stellarium_RES_CXX} ${stellarium_exe_SRCS})
 TARGET_LINK_LIBRARIES(stellarium ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
 IF(APPLE)
  SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/works/OpenGL.framework/Versions/A/Libraries/libGL.dylib -framework Cocoa")
 ENDIF()
ENDIF()

INSTALL(TARGETS stellarium DESTINATION bin)



#############################################################################################
################################## Build unit tests #########################################
#############################################################################################

SET(tests_testDates_SRCS
	core/StelUtils.cpp
	core/StelUtils.hpp
	tests/testDates.cpp)
QT4_WRAP_CPP(testDates_MOC_SRCS tests/testDates.hpp)
ADD_EXECUTABLE(testDates ${tests_testDates_SRCS} ${testDates_MOC_SRCS})
TARGET_LINK_LIBRARIES(testDates ${extLinkerOption} ${QT_QTTEST_LIBRARY})
ADD_TEST(test_testDates ${EXECUTABLE_OUTPUT_PATH}/testDates)

SET(tests_testStelFileMgr_SRCS
	core/StelFileMgr.cpp
	core/StelFileMgr.hpp
	tests/testStelFileMgr.cpp)
QT4_WRAP_CPP(testStelFileMgr_MOC_SRCS tests/testStelFileMgr.hpp)
ADD_EXECUTABLE(testStelFileMgr ${tests_testStelFileMgr_SRCS} ${testStelFileMgr_MOC_SRCS})
TARGET_LINK_LIBRARIES(testStelFileMgr ${extLinkerOption} ${QT_QTTEST_LIBRARY})
ADD_TEST(test_testStelFileMgr ${EXECUTABLE_OUTPUT_PATH}/testStelFileMgr)

SET(tests_testStelSphereGeometry_SRCS
	tests/testStelSphereGeometry.hpp
	tests/testStelSphereGeometry.cpp
	core/StelSphereGeometry.hpp
	core/StelSphereGeometry.cpp
	core/OctahedronPolygon.hpp
	core/OctahedronPolygon.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp
	core/StelUtils.cpp
	core/StelUtils.hpp
	${glues_lib_SRCS})
QT4_WRAP_CPP(testStelSphereGeometry_MOC_SRCS tests/testStelSphereGeometry.hpp)
ADD_EXECUTABLE(testStelSphereGeometry ${tests_testStelSphereGeometry_SRCS} ${testStelSphereGeometry_MOC_SRCS})
TARGET_LINK_LIBRARIES(testStelSphereGeometry ${extLinkerOption} ${QT_QTTEST_LIBRARY})
ADD_TEST(test_testStelSphereGeometry ${EXECUTABLE_OUTPUT_PATH}/testStelSphereGeometry)

SET(tests_testStelSphericalIndex_SRCS
	tests/testStelSphericalIndex.hpp
	tests/testStelSphericalIndex.cpp
	core/StelSphericalIndex.hpp
	core/StelSphericalIndex.cpp
	core/StelSphereGeometry.hpp
	core/StelSphereGeometry.cpp
	core/OctahedronPolygon.hpp
	core/OctahedronPolygon.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp
	core/StelUtils.cpp
	core/StelUtils.hpp
	${glues_lib_SRCS})
QT4_WRAP_CPP(tests_testStelSphericalIndex_MOC_SRCS tests/testStelSphericalIndex.hpp)
ADD_EXECUTABLE(testStelSphericalIndex ${tests_testStelSphericalIndex_SRCS} ${tests_testStelSphericalIndex_MOC_SRCS})
TARGET_LINK_LIBRARIES(testStelSphericalIndex ${extLinkerOption} ${QT_QTTEST_LIBRARY})
ADD_TEST(test_testStelSphericalIndex ${EXECUTABLE_OUTPUT_PATH}/testStelSphericalIndex)

SET(tests_testStelJsonParser_SRCS
	tests/testStelJsonParser.hpp
	tests/testStelJsonParser.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp)
QT4_WRAP_CPP(tests_testStelJsonParser_MOC_SRCS tests/testStelJsonParser.hpp)
ADD_EXECUTABLE(testStelJsonParser ${tests_testStelJsonParser_SRCS} ${tests_testStelJsonParser_MOC_SRCS})
TARGET_LINK_LIBRARIES(testStelJsonParser ${extLinkerOption} ${QT_QTTEST_LIBRARY})
ADD_TEST(test_testStelJsonParser ${EXECUTABLE_OUTPUT_PATH}/testStelJsonParser)
