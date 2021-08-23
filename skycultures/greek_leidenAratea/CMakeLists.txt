########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/greek_leidenAratea )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/greek_leidenAratea
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*_names.fab"
	PATTERN "description.*.utf8"
	PATTERN "*.jpg"
 	PATTERN "CMakeFiles" EXCLUDE )
