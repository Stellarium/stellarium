
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab DESTINATION ${SDATALOC}/skycultures/modern_iau )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/modern_iau
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png" 
	PATTERN "*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
