
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab DESTINATION ${SDATALOC}/skycultures/navajo )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/navajo
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
