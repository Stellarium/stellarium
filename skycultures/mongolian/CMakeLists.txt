
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab DESTINATION share/${PACKAGE}/skycultures/mongolian )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/mongolian
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "*.jpg" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
