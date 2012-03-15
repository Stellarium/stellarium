########### install files ###############

IF(BUILD_FOR_ANDROID)
  INSTALL(FILES ssystem.ini base_locations.bin.gz Roboto-Regular.ttf default_config.ini DejaVuSansMono.ttf constellations_boundaries.dat iso639-1.utf8 iso3166-1-alpha-2.utf8 countryCodes.dat DESTINATION share/${PACKAGE}/data)
ELSE()
  INSTALL(FILES stellarium.ico ssystem.ini base_locations.bin.gz DejaVuSans.ttf default_config.ini DejaVuSansMono.ttf constellations_boundaries.dat iso639-1.utf8 iso3166-1-alpha-2.utf8 countryCodes.dat DESTINATION share/${PACKAGE}/data)
ENDIF()

IF (BUILD_FOR_MAEMO)
  INSTALL(FILES stellarium.desktop DESTINATION /usr/share/applications/hildon)
  INSTALL(FILES stellarium.png DESTINATION /usr/share/icons/hicolor/48x48/apps)
ENDIF()

IF(MACOSX)
INSTALL(FILES Icon.icns DESTINATION share )
INSTALL(FILES Info.plist PkgInfo DESTINATION . )
ENDIF(MACOSX)

