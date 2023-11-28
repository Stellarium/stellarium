
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/tibetan )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/tibetan
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png" 
	PATTERN "*.jpg" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
