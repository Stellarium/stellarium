
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/geneve )

# install textures and descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/geneve 
	FILES_MATCHING PATTERN "*.txt" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
