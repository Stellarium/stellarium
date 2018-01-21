
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/western_hlad )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/western_hlad
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
