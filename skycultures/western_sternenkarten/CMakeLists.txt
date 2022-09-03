
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/western_sternenkarten )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/western_sternenkarten
	FILES_MATCHING PATTERN "*.fab"
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
