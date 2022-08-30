
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab DESTINATION ${SDATALOC}/skycultures/samoan )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/samoan
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
