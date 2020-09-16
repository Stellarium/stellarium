
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini star_names.fab DESTINATION ${SDATALOC}/skycultures/seleucid )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/seleucid
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.fab" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
