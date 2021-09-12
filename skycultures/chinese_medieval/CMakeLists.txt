
########### install files ###############

# install info.ini and star_names.fab
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/chinese_medieval )

# install constellations data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/chinese_medieval
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*.png"
	PATTERN "*.fab" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
