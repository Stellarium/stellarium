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
	core/StelToneReproducer.cpp
	core/StelToneReproducer.hpp
	core/StelSkyLayerMgr.cpp
	core/StelSkyLayerMgr.hpp
	core/StelSkyLayer.hpp
	core/StelSkyLayer.cpp
	core/StelFader.hpp
	core/StelSphereGeometry.cpp
	core/StelSphereGeometry.hpp
	core/TriangleIterator.hpp
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
	core/StelViewportEffect.hpp
	core/StelViewportEffect.cpp
	core/TrailGroup.hpp
	core/TrailGroup.cpp
	core/RefractionExtinction.hpp
	core/RefractionExtinction.cpp
	core/StelLoadingBar.cpp
	core/StelLoadingBar.hpp 
	core/StelShortcutMgr.hpp
	core/StelShortcutMgr.cpp
	core/StelShortcutGroup.cpp
	core/StelShortcutGroup.hpp

	core/Triplet.hpp

	core/renderer/StelRendererStatistics.hpp
	core/renderer/StelRenderer.cpp
	core/renderer/StelRenderer.hpp
	core/renderer/StelQGLViewport.cpp
	core/renderer/StelQGLViewport.hpp
	core/renderer/StelQGLRenderer.cpp
        core/renderer/StelQGLRenderer.hpp
	core/renderer/StelQGL2Renderer.hpp
	
	core/renderer/StelCircleArcRenderer.hpp

	core/renderer/StelGLCompatibility.hpp
	core/renderer/StelGLUtilityFunctions.cpp
	core/renderer/StelGLUtilityFunctions.hpp

	core/renderer/StelIndexBuffer.hpp
	core/renderer/StelQGLIndexBuffer.hpp
	core/renderer/StelVertexBuffer.hpp
	core/renderer/StelVertexBufferBackend.hpp
	core/renderer/StelQGLArrayVertexBufferBackend.cpp
	core/renderer/StelQGLArrayVertexBufferBackend.hpp
	core/renderer/StelQGLInterleavedArrayVertexBufferBackend.cpp
        core/renderer/StelQGLInterleavedArrayVertexBufferBackend.hpp
	core/renderer/StelQGL2InterleavedArrayVertexBufferBackend.cpp
        core/renderer/StelQGL2InterleavedArrayVertexBufferBackend.hpp
	core/renderer/StelQGL2ArrayVertexBufferBackend.cpp
	core/renderer/StelQGL2ArrayVertexBufferBackend.hpp
	core/renderer/StelVertexAttribute.hpp
	core/renderer/StelGeometryBuilder.hpp
	core/renderer/StelGeometryBuilder.cpp
	core/renderer/GenericVertexTypes.hpp

	core/renderer/StelTextureParams.hpp
	core/renderer/StelTextureNew.cpp
	core/renderer/StelTextureNew.hpp
	core/renderer/StelTextureBackend.hpp
	core/renderer/StelTextureCache.hpp
	core/renderer/StelQGLTextureBackend.cpp
	core/renderer/StelQGLTextureBackend.hpp
	core/renderer/StelTextureLoader.hpp

	core/renderer/StelLight.hpp

	core/renderer/StelGLSLShader.hpp
	core/renderer/StelQGLGLSLShader.hpp
	core/renderer/StelQGLGLSLShader.cpp

	${gl_lib_SRCS}
	core/external/fixx11h.h
	core/external/gettext.h

	core/external/kfilter/kfilterdev.h
	core/external/kfilter/kfilterdev.cpp
	core/external/kfilter/kfilterbase.h
	core/external/kfilter/kfilterbase.cpp
	core/external/kfilter/kgzipfilter.h
	core/external/kfilter/kgzipfilter.cpp
	core/external/kfilter/kzip.h
	core/external/kfilter/kzip.cpp
	core/external/kfilter/klimitediodevice.h
	core/external/kfilter/klimitediodevice.cpp
	core/external/kfilter/karchive.h
	core/external/kfilter/karchive.cpp

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
	core/planetsephems/sideral_time.c
	core/planetsephems/sideral_time.h
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
	core/modules/ZoneArray.hpp
	core/modules/ZoneData.hpp

	StelMainGraphicsView.hpp
	StelMainGraphicsView.cpp
	StelMainWindow.hpp
	StelMainWindow.cpp
	StelLogger.hpp
	StelLogger.cpp
	CLIProcessor.hpp
	CLIProcessor.cpp
	translations.h
)

#Compile in OpenGL1 support iff we're not compiling for OpenGL ES2
IF (OPENGL_MODE STREQUAL "Desktop")
        SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
                core/renderer/StelQGL1InterleavedArrayVertexBufferBackend.cpp
                core/renderer/StelQGL1InterleavedArrayVertexBufferBackend.hpp
                core/renderer/StelQGL1ArrayVertexBufferBackend.cpp
                core/renderer/StelQGL1ArrayVertexBufferBackend.hpp
                core/renderer/StelQGL1Renderer.hpp
        )
ELSE()
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
		core/renderer/StelQGLES2Renderer.hpp
		core/renderer/StelQGLES2GLSLShader.cpp
		core/renderer/StelQGLES2GLSLShader.hpp
	)
ENDIF()

IF(ENABLE_SCRIPTING)
	SET(stellarium_lib_SRCS ${stellarium_lib_SRCS}
		scripting/StelScriptMgr.cpp
		scripting/StratoscriptPreprocessor.cpp
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
		core/external/kdewin32/basetyps.h
		core/external/kdewin32/byteswap.h
		core/external/kdewin32/comcat.h
		core/external/kdewin32/dirent.h
		core/external/kdewin32/docobj.h
		core/external/kdewin32/errno.h
		core/external/kdewin32/fcntl.h
		core/external/kdewin32/grp.h
		core/external/kdewin32/ifaddrs.h
		core/external/kdewin32/langinfo.h
		core/external/kdewin32/mshtml.h
		core/external/kdewin32/netdb.h
		core/external/kdewin32/nl_types.h
		core/external/kdewin32/oaidl.h
		core/external/kdewin32/objfwd.h
		core/external/kdewin32/objidl.h
		core/external/kdewin32/ocidl.h
		core/external/kdewin32/olectl.h
		core/external/kdewin32/oleidl.h
		core/external/kdewin32/pwd.h
		core/external/kdewin32/signal.h
		core/external/kdewin32/stdio.h
		core/external/kdewin32/stdlib.h
		core/external/kdewin32/string.h
		core/external/kdewin32/strings.h
		core/external/kdewin32/syslog.h
		core/external/kdewin32/time.h
		core/external/kdewin32/unistd.h
		core/external/kdewin32/unknwn.h
		core/external/kdewin32/wchar.h
		core/external/kdewin32/arpa/inet.h
		core/external/kdewin32/asm/byteorder.h
		core/external/kdewin32/net/if.h
		core/external/kdewin32/netinet/in.h
		core/external/kdewin32/netinet/tcp.h
		core/external/kdewin32/sys/ioctl.h
		core/external/kdewin32/sys/mman.h
		core/external/kdewin32/sys/resource.h
		core/external/kdewin32/sys/select.h
		core/external/kdewin32/sys/signal.h
		core/external/kdewin32/sys/socket.h
		core/external/kdewin32/sys/stat.h
		core/external/kdewin32/sys/time.h
		core/external/kdewin32/sys/times.h
		core/external/kdewin32/sys/types.h
		core/external/kdewin32/sys/uio.h
		core/external/kdewin32/sys/un.h
		core/external/kdewin32/sys/unistd.h
		core/external/kdewin32/sys/utsname.h
		core/external/kdewin32/sys/wait.h

		core/external/kdewin32/bind/inet_ntop.c
		core/external/kdewin32/bind/inet_pton.c
		core/external/kdewin32/dirent.c
		core/external/kdewin32/errno.c
		core/external/kdewin32/fsync.c
		core/external/kdewin32/getenv.c
		core/external/kdewin32/grp.c
		core/external/kdewin32/inet.c
		core/external/kdewin32/mmap.c
		core/external/kdewin32/nl_langinfo.c
		core/external/kdewin32/net.c
		core/external/kdewin32/pwd.c
		core/external/kdewin32/realpath.c
		core/external/kdewin32/resource.c
		core/external/kdewin32/signal.c
		core/external/kdewin32/stdlib.c
		core/external/kdewin32/string.c
		core/external/kdewin32/strptime.c
		core/external/kdewin32/syslog.c
		core/external/kdewin32/time.c
		core/external/kdewin32/uname.c
		core/external/kdewin32/unistd.c
	)
ENDIF(WIN32)

SET(stellarium_exe_SRCS main.cpp)
IF (BUILD_FOR_ANDROID)
    SET(stellarium_exe_SRCS
        ${stellarium_exe_SRCS}
		android/qtmain_android.cpp
		android/JavaWrapper.hpp
		android/JavaWrapper.cpp
        )
ENDIF (BUILD_FOR_ANDROID)

################# compiles resources files ############
SET(stellarium_RES ${CMAKE_SOURCE_DIR}/data/mainRes.qrc)
QT4_ADD_RESOURCES(stellarium_RES_CXX ${stellarium_RES})


#############################################################################################
############################# Standard GUI plugin compilation ###############################
#############################################################################################
IF (GUI_MODE STREQUAL "Standard")
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
	gui/addRemoveLandscapesDialog.ui)

	IF(ENABLE_SCRIPT_CONSOLE)
		SET(stellarium_gui_SRCS ${stellarium_gui_SRCS}
			gui/ScriptConsole.hpp
			gui/ScriptConsole.cpp
			gui/StelScriptSyntaxHighlighter.hpp
			gui/StelScriptSyntaxHighlighter.cpp)
		SET(stellarium_UIS ${stellarium_UIS}
			gui/scriptConsole.ui)
	ENDIF(ENABLE_SCRIPT_CONSOLE)

	QT4_WRAP_UI(stellarium_UIS_H ${stellarium_UIS})

	# compiles resources files
	SET(stellarium_gui_RES ${CMAKE_SOURCE_DIR}/data/gui/guiRes.qrc)
	QT4_ADD_RESOURCES(stellarium_gui_RES_CXX ${stellarium_gui_RES})

	SET(extLinkerOption ${QT_LIBRARIES} ${QT_QTSCRIPT_LIBRARY} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} )

	# Compile the static library
	ADD_LIBRARY(StelGuiLib STATIC ${stellarium_gui_SRCS} ${stellarium_UIS_H} ${stellarium_gui_RES_CXX})
	TARGET_LINK_LIBRARIES(StelGuiLib ${StelMain} ${extLinkerOption})
	SET_TARGET_PROPERTIES(StelGuiLib PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN -DQT_PLUGIN")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/${CMAKE_CFG_INTDIR}/libStelGuiLib.a)
	ADD_DEPENDENCIES(AllStaticPlugins StelGuiLib)
ENDIF()

#############################################################################################
################################ No GUI plugin compilation ##################################
#############################################################################################
IF (GUI_MODE STREQUAL "None")
	SET(stellarium_nogui_SRCS
		noGui/StelNoGui.hpp
		noGui/StelNoGui.cpp)
	ADD_LIBRARY(StelNoGuiLib STATIC ${stellarium_nogui_SRCS})
	TARGET_LINK_LIBRARIES(StelNoGuiLib ${extLinkerOption})
	SET_TARGET_PROPERTIES(StelNoGuiLib PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/libStelNoGuiLib.a)
	ADD_DEPENDENCIES(AllStaticPlugins StelNoGuiLib)
ENDIF()

#############################################################################################
############################# Mobile GUI plugin compilation #################################
#############################################################################################
IF (GUI_MODE STREQUAL "Mobile")
        ADD_DEFINITIONS(-DMOBILE_GUI)

	SET(mobilegui_SRCS
		mobileGui/MobileGui.hpp
		mobileGui/MobileGui.cpp
		mobileGui/SystemDisplayInfo.hpp
		mobileGui/SystemDisplayInfo.cpp
		mobileGui/StelWrapper.hpp
		mobileGui/StelWrapper.cpp
		mobileGui/UpdateSignallingItem.hpp
		mobileGui/UpdateSignallingItem.cpp
		mobileGui/MobileImageProvider.hpp
                mobileGui/MobileImageProvider.cpp
                mobileGui/MenuListModel.hpp
                mobileGui/MenuListModel.cpp)
	SET(mobilegui_MOC_HDRS
		mobileGui/MobileGui.hpp
		mobileGui/SystemDisplayInfo.hpp
                mobileGui/StelWrapper.hpp
                mobileGui/MenuListModel.hpp)
	SET(mobilegui_RES
		mobileGui/qml/qmlResources.qrc)
	QT4_ADD_RESOURCES(mobilegui_RES_CXX ${mobilegui_RES})
	QT4_WRAP_CPP(mobilegui_MOC_SRCS ${mobilegui_MOC_HDRS})
	ADD_LIBRARY(MobileGuiLib STATIC ${mobilegui_SRCS} ${mobilegui_MOC_SRCS} ${mobilegui_RES_CXX})
	TARGET_LINK_LIBRARIES(MobileGuiLib ${extLinkerOption})
	SET_TARGET_PROPERTIES(MobileGuiLib PROPERTIES COMPILE_FLAGS "-DQT_STATICPLUGIN")
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/libMobileGuiLib.a)
	ADD_DEPENDENCIES(AllStaticPlugins MobileGuiLib)
ENDIF()


#############################################################################################
############################# External GUI plugin compilation ###############################
#############################################################################################
IF (GUI_MODE STREQUAL "External")
	SET(EXTERNAL_GUI_SRC_PATH "../../MyGui/" CACHE PATH "The location of the plugin main directory i.e. the one containing the top level CMakeLists.txt")
	ADD_SUBDIRECTORY( ${EXTERNAL_GUI_SRC_PATH} externalGui )
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/src/externalGui/src/libExternalGui.a)
	SET(EXTERNAL_GUI_PLUGIN_EXTRALIBS CACHE STRING "Additionnal link options needed for the GUI plugin")
	SET(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} ${EXTERNAL_GUI_PLUGIN_EXTRALIBS})
ENDIF()


###############################################################################################
################## Static plugins need to be known by the file StelMainGraphicsView.cpp at compilation time
################## so add the definitions to the compilation.
################## Also add the full path to the .a libraries at compilation time.
IF (BUILD_STATIC_PLUGINS)
  IF (USE_PLUGIN_HELLOSTELMODULE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/HelloStelModule/src/${CMAKE_CFG_INTDIR}/libHelloStelModule.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_HELLOSTELMODULE)
  ENDIF()
  
  IF (USE_PLUGIN_SIMPLEDRAWLINE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/SimpleDrawLine/src/${CMAKE_CFG_INTDIR}/libSimpleDrawLine.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SIMPLEDRAWLINE)
  ENDIF()

IF (USE_PLUGIN_OBSERVABILITY)
       SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Observability/src/${CMAKE_CFG_INTDIR}/libObservability.a")
       ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_OBSERVABILITY)
ENDIF()

  IF (USE_PLUGIN_ANGLEMEASURE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/AngleMeasure/src/${CMAKE_CFG_INTDIR}/libAngleMeasure.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_ANGLEMEASURE)
  ENDIF()

  IF (USE_PLUGIN_COMPASSMARKS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/CompassMarks/src/${CMAKE_CFG_INTDIR}/libCompassMarks.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_COMPASSMARKS)
  ENDIF()

  IF (USE_PLUGIN_SATELLITES)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Satellites/src/${CMAKE_CFG_INTDIR}/libSatellites.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SATELLITES)
  ENDIF()

  IF (USE_PLUGIN_TELESCOPECONTROL)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/TelescopeControl/src/${CMAKE_CFG_INTDIR}/libTelescopeControl.a")
	IF(WIN32)
		SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} libwsock32.a)
	ENDIF()
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_TELESCOPECONTROL)
  ENDIF()

  IF (USE_PLUGIN_LOGBOOK)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/LogBook/src/${CMAKE_CFG_INTDIR}/libLogBook.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_LOGBOOK)
	SET(QT_USE_QTSQL TRUE)
  ENDIF()

  IF (USE_PLUGIN_OCULARS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Oculars/src/${CMAKE_CFG_INTDIR}/libOculars.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_OCULARS)
	SET(QT_USE_QTSQL TRUE)
  ENDIF()

  IF (USE_PLUGIN_TEXTUSERINTERFACE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/TextUserInterface/src/${CMAKE_CFG_INTDIR}/libTextUserInterface.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_TEXTUSERINTERFACE)
  ENDIF()

  IF (USE_PLUGIN_SOLARSYSTEMEDITOR)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/SolarSystemEditor/src/${CMAKE_CFG_INTDIR}/libSolarSystemEditor.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SOLARSYSTEMEDITOR)
  ENDIF()

  IF (USE_PLUGIN_TIMEZONECONFIGURATION)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/TimeZoneConfiguration/src/${CMAKE_CFG_INTDIR}/libTimeZoneConfiguration.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_TIMEZONECONFIGURATION)
  ENDIF()

  IF (USE_PLUGIN_SUPERNOVAE)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Supernovae/src/${CMAKE_CFG_INTDIR}/libSupernovae.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SUPERNOVAE)
  ENDIF()

  IF (USE_PLUGIN_QUASARS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Quasars/src/${CMAKE_CFG_INTDIR}/libQuasars.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_QUASARS)
  ENDIF()

  IF (USE_PLUGIN_PULSARS)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Pulsars/src/${CMAKE_CFG_INTDIR}/libPulsars.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_PULSARS)
  ENDIF()

  IF (USE_PLUGIN_EXOPLANETS)
    SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/Exoplanets/src/${CMAKE_CFG_INTDIR}/libExoplanets.a")
    ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_EXOPLANETS)
  ENDIF()

  IF (USE_PLUGIN_SVMT)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/plugins/svmt/src/${CMAKE_CFG_INTDIR}/libSVMT.a)
	IF(APPLE)
	  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "-framework QtDeclarative")
	ELSE()
	  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${QT_LIBRARY_DIR}/libQtDeclarative.so)
	ENDIF()
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_SVMT)
  ENDIF()

  IF (USE_PLUGIN_VIRGO)
	SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} ${CMAKE_BINARY_DIR}/plugins/VirGO/src/${CMAKE_CFG_INTDIR}/libVirGO.a ${CFITSIO_LIBRARIES})
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_VIRGO)
  ENDIF()

  IF (USE_PLUGIN_RENDERERSTATISTICS)
	  SET(STELLARIUM_STATIC_PLUGINS_LIBRARIES ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} "${CMAKE_BINARY_DIR}/plugins/RendererStatistics/src/${CMAKE_CFG_INTDIR}/libRendererStatistics.a")
	ADD_DEFINITIONS(-DUSE_STATIC_PLUGIN_RENDERERSTATISTICS)
  ENDIF()
ENDIF()


#############################################################################################
################################ Build main program #########################################
#############################################################################################

SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} ${ZLIB_LIBRARIES})
IF(ENABLE_SCRIPTING)
  SET(extLinkerOption ${extLinkerOption} ${QT_QTSCRIPT_LIBRARY})
ENDIF()
IF(WIN32)
	SET(extLinkerOption ${extLinkerOption} libwinmm.a)
ENDIF()

IF(WIN32 AND ENABLE_WINDOWS_EXE_ICON)
  IF(RC_COMPILER_PATH)
    ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_BINARY_DIR}/stellarium-rc.o
                       COMMAND ${RC_COMPILER_PATH} ${CMAKE_SOURCE_DIR}/data/stellarium.rc ${CMAKE_BINARY_DIR}/stellarium-rc.o
                       WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/data)
    SET(stellarium_exe_SRCS ${stellarium_exe_SRCS} ${CMAKE_BINARY_DIR}/stellarium-rc.o)
  ENDIF(RC_COMPILER_PATH)
ENDIF(WIN32 AND ENABLE_WINDOWS_EXE_ICON)

IF(BUILD_FOR_ANDROID)
  SET(extLinkerOption ${extLinkerOption} ${QTMOBILITY_LIBRARIES})

 #compile as a library
 SET(extLinkerOption -lGLESv1_CM -ldl -llog ${extLinkerOption})
 ADD_LIBRARY(stellarium SHARED ${stellarium_lib_SRCS} ${stellarium_MOC_SRCS} ${stellarium_RES_CXX} ${stellarium_exe_SRCS})
 TARGET_LINK_LIBRARIES(stellarium ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
ELSE()

 IF(GENERATE_STELMAINLIB)
  ADD_LIBRARY(stelMain SHARED ${stellarium_lib_SRCS} ${stellarium_RES_CXX})
  TARGET_LINK_LIBRARIES(stelMain ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
  INSTALL(TARGETS stelMain DESTINATION lib)
  ADD_EXECUTABLE(stellarium ${stellarium_exe_SRCS})
  #stelMain should be after the plug-ins, otherwise the build crashes
  TARGET_LINK_LIBRARIES(stellarium ${STELLARIUM_STATIC_PLUGINS_LIBRARIES} stelMain ${extLinkerOption})
 ELSE()
  ADD_EXECUTABLE(stellarium ${stellarium_lib_SRCS} ${stellarium_RES_CXX} ${stellarium_exe_SRCS})
  TARGET_LINK_LIBRARIES(stellarium ${extLinkerOption} ${STELLARIUM_STATIC_PLUGINS_LIBRARIES})
  IF(APPLE)
 # SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/works/OpenGL.framework/Versions/A/Libraries/libGL.dylib -framework Cocoa -framework QtDeclarative")
 SET_TARGET_PROPERTIES(stellarium PROPERTIES LINK_FLAGS " -framework Cocoa -framework OpenGL")

  ENDIF()
 ENDIF()
ENDIF()

ADD_DEPENDENCIES(stellarium AllStaticPlugins)

INSTALL(TARGETS stellarium DESTINATION bin)



#############################################################################################
################################## Build unit tests #########################################
#############################################################################################

SET(extLinkerOptionTest ${QT_LIBRARIES} ${QT_QTTEST_LIBRARY} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} ${ZLIB_LIBRARIES})


# Custom target used to build all tests at once
ADD_CUSTOM_TARGET(buildTests)

SET(tests_testDates_SRCS
	core/StelUtils.cpp
	core/StelUtils.hpp
	tests/testDates.cpp)
ADD_EXECUTABLE(testDates EXCLUDE_FROM_ALL ${tests_testDates_SRCS})
TARGET_LINK_LIBRARIES(testDates ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testDates)

SET(tests_testStelFileMgr_SRCS
	core/StelFileMgr.cpp
	core/StelFileMgr.hpp
	tests/testStelFileMgr.cpp)
ADD_EXECUTABLE(testStelFileMgr EXCLUDE_FROM_ALL ${tests_testStelFileMgr_SRCS})
TARGET_LINK_LIBRARIES(testStelFileMgr ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelFileMgr)

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
	core/StelProjector.cpp
	core/StelProjector.hpp
	core/StelTranslator.cpp
	core/StelTranslator.hpp
	${glues_lib_SRCS})
ADD_EXECUTABLE(testStelSphereGeometry EXCLUDE_FROM_ALL ${tests_testStelSphereGeometry_SRCS})
TARGET_LINK_LIBRARIES(testStelSphereGeometry ${extLinkerOptionTest} ${QT_QTOPENGL_LIBRARY})
ADD_DEPENDENCIES(buildTests testStelSphereGeometry)

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
	core/StelProjector.cpp
	core/StelProjector.hpp
	core/StelTranslator.cpp
	core/StelTranslator.hpp
	${glues_lib_SRCS})
ADD_EXECUTABLE(testStelSphericalIndex EXCLUDE_FROM_ALL ${tests_testStelSphericalIndex_SRCS})
TARGET_LINK_LIBRARIES(testStelSphericalIndex ${extLinkerOptionTest} ${QT_QTOPENGL_LIBRARY})
ADD_DEPENDENCIES(buildTests testStelSphericalIndex)

SET(tests_testStelJsonParser_SRCS
	tests/testStelJsonParser.hpp
	tests/testStelJsonParser.cpp
	core/StelJsonParser.hpp
	core/StelJsonParser.cpp)
ADD_EXECUTABLE(testStelJsonParser EXCLUDE_FROM_ALL ${tests_testStelJsonParser_SRCS})
TARGET_LINK_LIBRARIES(testStelJsonParser ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelJsonParser)

SET(tests_testStelVertexBuffer_SRCS
  ${glues_lib_SRCS}
  tests/testStelVertexBuffer.hpp
  tests/testStelVertexBuffer.cpp
  core/StelFileMgr.cpp
  core/StelSphereGeometry.cpp
  core/StelSphereGeometry.hpp
  core/StelProjector.cpp
  core/StelProjector.hpp
  core/OctahedronPolygon.cpp
  core/OctahedronPolygon.hpp
  core/StelJsonParser.cpp
  core/StelJsonParser.hpp
  core/StelUtils.cpp
  core/StelUtils.hpp
  core/StelTranslator.cpp
  core/StelTranslator.hpp
  core/renderer/StelVertexBuffer.hpp
  core/renderer/StelVertexBufferBackend.hpp
  core/renderer/StelGLUtilityFunctions.cpp
  core/renderer/StelGLSLShader.hpp
  core/renderer/StelQGLGLSLShader.cpp
  core/renderer/StelQGLGLSLShader.hpp
  core/renderer/StelQGLArrayVertexBufferBackend.cpp
  core/renderer/StelQGLArrayVertexBufferBackend.hpp
  core/renderer/StelQGL2ArrayVertexBufferBackend.cpp
  core/renderer/StelQGL2ArrayVertexBufferBackend.hpp)
ADD_EXECUTABLE(testStelVertexBuffer EXCLUDE_FROM_ALL ${tests_testStelVertexBuffer_SRCS})
TARGET_LINK_LIBRARIES(testStelVertexBuffer ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testStelVertexBuffer)

SET(tests_testDeltaT_SRCS
  tests/testDeltaT.hpp
  tests/testDeltaT.cpp
  core/StelUtils.cpp
  core/StelUtils.hpp)
ADD_EXECUTABLE(testDeltaT EXCLUDE_FROM_ALL ${tests_testDeltaT_SRCS})
TARGET_LINK_LIBRARIES(testDeltaT ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testDeltaT)

SET(tests_testConversions_SRCS
	core/StelUtils.cpp
	core/StelUtils.hpp
	tests/testConversions.hpp
	tests/testConversions.cpp)
ADD_EXECUTABLE(testConversions EXCLUDE_FROM_ALL ${tests_testConversions_SRCS})
TARGET_LINK_LIBRARIES(testConversions ${extLinkerOptionTest})
ADD_DEPENDENCIES(buildTests testConversions)

######SET(extLinkerOption ${QT_LIBRARIES} ${OPENGL_LIBRARIES} ${ICONV_LIBRARIES} ${INTL_LIBRARIES} ${ZLIB_LIBRARIES})

ADD_CUSTOM_TARGET(tests COMMENT "Run the Stellarium unit tests")
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testDates WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testStelFileMgr WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testStelSphereGeometry WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testStelSphericalIndex WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testStelVertexBuffer WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testStelJsonParser WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testDeltaT WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_CUSTOM_COMMAND(TARGET tests POST_BUILD COMMAND ./testConversions WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/src/)
ADD_DEPENDENCIES(tests buildTests)

