
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/guereins )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/guereins 
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
