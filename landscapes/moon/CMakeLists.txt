
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION ${SDATALOC}/landscapes/moon )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION ${SDATALOC}/landscapes/moon
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
