
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION ${SDATALOC}/landscapes/grossmugl )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION ${SDATALOC}/landscapes/grossmugl
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "gazetteer.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
