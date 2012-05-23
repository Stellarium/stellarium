########### install files ###############

INSTALL(FILES stellarium.ico ssystem.ini shortcuts.ini base_locations.bin.gz DejaVuSans.ttf default_config.ini DejaVuSansMono.ttf constellations_boundaries.dat iso639-1.utf8 iso3166-1-alpha-2.utf8 countryCodes.dat DESTINATION share/${PACKAGE}/data)

IF (BUILD_FOR_MAEMO)
  INSTALL(FILES maemo/stellarium.desktop DESTINATION /usr/share/applications/hildon)
  INSTALL(FILES maemo/stellarium.png DESTINATION /usr/share/icons/hicolor/48x48/apps)
ENDIF()

IF(UNIX AND NOT APPLE)
  INSTALL(FILES stellarium.desktop DESTINATION share/applications)
  INSTALL(FILES stellarium.xpm DESTINATION share/pixmaps)
ENDIF()

IF(MACOSX)
  INSTALL(FILES Icon.icns DESTINATION share )
  INSTALL(FILES Info.plist PkgInfo DESTINATION . )
ENDIF(MACOSX)

