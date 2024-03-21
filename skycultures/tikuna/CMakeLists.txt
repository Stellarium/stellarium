
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/tikuna )

# install constellations data, proper names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/tikuna
	FILES_MATCHING PATTERN "*.fab"
	PATTERN "*.png"
	PATTERN "*.utf8"
	PATTERN "CMakeFiles" EXCLUDE )
