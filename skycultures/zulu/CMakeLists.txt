########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/zulu )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/zulu
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
