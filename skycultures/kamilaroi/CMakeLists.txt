
########### install files ###############

# install info.ini 
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/kamilaroi )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/kamilaroi
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "*.fab" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
