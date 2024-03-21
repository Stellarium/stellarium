
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/anutan )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/anutan
	FILES_MATCHING PATTERN "*.fab"
	PATTERN "*.utf8"
	PATTERN "CMakeFiles" EXCLUDE )
