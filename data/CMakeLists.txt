
########### install files ###############
INSTALL(FILES ssystem_major.ini ssystem_minor.ini ssystem_1000comets.ini 
	base_locations.bin.gz default_cfg.ini languages.tab iso3166.tab 
	regions-geoscheme.tab constellation_boundaries.dat constellations_spans.dat 
        nomenclature.dat DESTINATION ${SDATALOC}/data)
# install fonts
INSTALL(FILES NotoSans-Regular.ttf NotoSansMono-Regular.ttf NotoSansSC-Regular.otf 
	DejaVuSans.ttf DejaVuSansMono.ttf DESTINATION ${SDATALOC}/data)
# install shaders
INSTALL(DIRECTORY shaders/ DESTINATION ${SDATALOC}/data/shaders FILES_MATCHING
	PATTERN "planet*")

# install splash
IF(STELLARIUM_RELEASE_BUILD)
	INSTALL(FILES splash.png DESTINATION ${SDATALOC}/data)
ELSE()
	INSTALL(FILES splash-gray.png DESTINATION ${SDATALOC}/data RENAME splash.png)
ENDIF()

# install icon
IF(APPLE)
	IF(STELLARIUM_RELEASE_BUILD)
		INSTALL(FILES stellarium.icns DESTINATION ${SDATALOC})
	ELSE()
		INSTALL(FILES stellarium-gray.icns DESTINATION ${SDATALOC} RENAME stellarium.icns)
	ENDIF()
	INSTALL(FILES Info.plist PkgInfo DESTINATION .)
ELSE()
	IF(STELLARIUM_RELEASE_BUILD)
		INSTALL(FILES stellarium.ico DESTINATION ${SDATALOC}/data)
	ELSE()
		INSTALL(FILES stellarium-gray.ico DESTINATION ${SDATALOC}/data RENAME stellarium.ico)
	ENDIF()
ENDIF()

# install Scenery3d shaders
IF(USE_PLUGIN_SCENERY3D)
INSTALL(DIRECTORY shaders/ DESTINATION ${SDATALOC}/data/shaders FILES_MATCHING
	PATTERN "s3d_*.vert" 
	PATTERN "s3d_*.geom" 
	PATTERN "s3d_*.frag")
ENDIF()

IF(UNIX AND NOT APPLE)
	INSTALL(FILES org.stellarium.Stellarium.desktop DESTINATION share/applications)
	INSTALL(FILES org.stellarium.Stellarium.appdata.xml DESTINATION share/metainfo)
	INSTALL(FILES stellarium.xml DESTINATION share/mime/packages)
	INSTALL(FILES icons/16x16/stellarium.png DESTINATION share/icons/hicolor/16x16/apps)
	INSTALL(FILES icons/32x32/stellarium.png DESTINATION share/icons/hicolor/32x32/apps)
	INSTALL(FILES icons/48x48/stellarium.png DESTINATION share/icons/hicolor/48x48/apps)
	INSTALL(FILES icons/64x64/stellarium.png DESTINATION share/icons/hicolor/64x64/apps)
	INSTALL(FILES icons/128x128/stellarium.png DESTINATION share/icons/hicolor/128x128/apps)
	INSTALL(FILES icons/256x256/stellarium.png DESTINATION share/icons/hicolor/256x256/apps)
	INSTALL(FILES icons/512x512/stellarium.png DESTINATION share/icons/hicolor/512x512/apps)
ENDIF()

