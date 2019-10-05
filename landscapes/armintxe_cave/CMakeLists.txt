
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/armintxe_cave )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/armintxe_cave 
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
