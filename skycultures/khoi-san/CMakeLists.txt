
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/khoi-san )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/khoi-san
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
