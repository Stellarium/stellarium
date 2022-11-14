
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/modern_chinese )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/modern_chinese
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png"
	PATTERN "*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
