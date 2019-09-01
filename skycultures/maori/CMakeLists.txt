
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/maori )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/maori
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*_names.fab"
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
