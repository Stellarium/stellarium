
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab dso_names.fab DESTINATION share/${PACKAGE}/skycultures/chinese )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/chinese
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
