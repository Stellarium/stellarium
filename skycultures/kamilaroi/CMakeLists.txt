
########### install files ###############

# install info.ini 
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/kamilaroi )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/kamilaroi
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "*.fab" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
