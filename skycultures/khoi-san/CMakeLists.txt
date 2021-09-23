
########### install files ###############

# install info.ini
INSTALL(FILES info.ini  DESTINATION ${SDATALOC}/skycultures/khoi-san )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/khoi-san
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
