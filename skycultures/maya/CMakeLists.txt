
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/maya )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/maya
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png"
	PATTERN "description.*.utf8" 
        PATTERN "CMakeFiles" EXCLUDE )

