
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/blackfoot )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/blackfoot
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.fab"
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
