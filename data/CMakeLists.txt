
########### install files ###############
INSTALL(FILES ssystem_major.ini ssystem_minor.ini ssystem_1000comets.ini base_locations.bin.gz DejaVuSans.ttf default_cfg.ini DejaVuSansMono.ttf iso639-1.utf8 iso3166.tab countryCodes.dat constellation_boundaries.dat constellations_spans.dat nomenclature.dat DESTINATION ${SDATALOC}/data)
INSTALL(DIRECTORY shaders/ DESTINATION ${SDATALOC}/data/shaders FILES_MATCHING
	PATTERN "planet*" 
	)

########### splash target ##############
ADD_CUSTOM_TARGET(splash ALL)
SET(_splashFile splash.png)
SET(_splashDstFile ${CMAKE_CURRENT_BINARY_DIR}/${_splashFile})

IF(NOT STELLARIUM_RELEASE_BUILD)
	GET_FILENAME_COMPONENT(_splashSrcFile splash-gray.png ABSOLUTE)
ELSE()
	GET_FILENAME_COMPONENT(_splashSrcFile splash.png ABSOLUTE)
ENDIF()

ADD_CUSTOM_COMMAND(
	TARGET splash
	COMMAND ${CMAKE_COMMAND} -E copy ${_splashSrcFile} ${_splashDstFile}
)
SET_TARGET_PROPERTIES(splash PROPERTIES FOLDER "data")

########### icon target ###############
ADD_CUSTOM_TARGET(icon ALL)
IF(APPLE)
	SET(_iconFile Icon.icns)
	IF(NOT STELLARIUM_RELEASE_BUILD)
		GET_FILENAME_COMPONENT(_iconSrcFile IconGray.icns ABSOLUTE)
	ELSE()
		GET_FILENAME_COMPONENT(_iconSrcFile Icon.icns ABSOLUTE)
	ENDIF()
ELSE()
	SET(_iconFile stellarium.ico)
	IF(NOT STELLARIUM_RELEASE_BUILD)
		GET_FILENAME_COMPONENT(_iconSrcFile stellarium-gray.ico ABSOLUTE)
	ELSE()
		GET_FILENAME_COMPONENT(_iconSrcFile stellarium.ico ABSOLUTE)
	ENDIF()	
ENDIF()

SET(_iconDstFile ${CMAKE_CURRENT_BINARY_DIR}/${_iconFile})
ADD_CUSTOM_COMMAND(
	TARGET icon
	COMMAND ${CMAKE_COMMAND} -E copy ${_iconSrcFile} ${_iconDstFile}
)
SET_TARGET_PROPERTIES(icon PROPERTIES FOLDER "data")

# install splash and icon
IF(APPLE)
        INSTALL(FILES ${_iconDstFile} DESTINATION ${SDATALOC} RENAME ${_iconFile})
        INSTALL(FILES ${_splashDstFile} DESTINATION ${SDATALOC}/data RENAME ${_splashFile})
	INSTALL(FILES Info.plist PkgInfo DESTINATION .)
ELSE()
        INSTALL(FILES ${_iconDstFile} DESTINATION ${SDATALOC}/data RENAME ${_iconFile})
        INSTALL(FILES ${_splashDstFile} DESTINATION ${SDATALOC}/data RENAME ${_splashFile})
ENDIF()

# install Scenery3d shaders
IF(USE_PLUGIN_SCENERY3D)
INSTALL(DIRECTORY shaders/ DESTINATION ${SDATALOC}/data/shaders FILES_MATCHING
	PATTERN "s3d_*.vert" 
	PATTERN "s3d_*.geom" 
	PATTERN "s3d_*.frag")
ENDIF()

IF (BUILD_FOR_MAEMO)
	INSTALL(FILES maemo/stellarium.desktop DESTINATION /usr/share/applications/hildon)
	INSTALL(FILES maemo/stellarium.png DESTINATION /usr/share/icons/hicolor/48x48/apps)
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

