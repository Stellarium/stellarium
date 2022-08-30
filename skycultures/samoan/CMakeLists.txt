
########### install files ###############

# install info.ini and
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/samoan )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/samoan
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
