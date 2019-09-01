
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION ${SDATALOC}/landscapes/hurricane )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION ${SDATALOC}/landscapes/hurricane
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
