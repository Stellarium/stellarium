
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION ${SDATALOC}/landscapes/jupiter )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION ${SDATALOC}/landscapes/jupiter
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
