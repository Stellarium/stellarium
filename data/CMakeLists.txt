########### install files ###############

INSTALL(FILES stellarium.ico ssystem.ini zone.tab base_locations.txt DejaVuSans.ttf default_config.ini DejaVuSansMono.ttf constellations_boundaries.dat iso639-1.utf8 countryCodes.dat DESTINATION share/${PACKAGE}/data)

IF(MACOSX)
INSTALL(FILES Icon.icns DESTINATION share )
INSTALL(FILES Info.plist PkgInfo DESTINATION . )
ENDIF(MACOSX)

ADD_SUBDIRECTORY( gui )
ADD_SUBDIRECTORY( shaders )
