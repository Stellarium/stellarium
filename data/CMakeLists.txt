########### install files ###############

INSTALL(FILES stellarium.ico ssystem.ini zone.tab cities_Earth.fab cities_Moon.fab cities_Mars.fab cities_Venus.fab DejaVuSans.ttf double_txt.dat variable_txt.dat default_config.ini DejaVuSansMono.ttf constellations_boundaries.dat fontmap.dat iso639-1.utf8 DESTINATION share/${PACKAGE}/data)

IF(MACOSX)
INSTALL(FILES Icon.icns DESTINATION share )
INSTALL(FILES Info.plist PkgInfo DESTINATION . )
ENDIF(MACOSX)
