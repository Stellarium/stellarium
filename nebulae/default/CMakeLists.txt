
########### install files ###############

# install DSO catalog and textures
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/nebulae/default 
	FILES_MATCHING 
	PATTERN "*.dat"
	PATTERN "*.png"
	PATTERN "*.json"
	PATTERN "CMakeFiles" EXCLUDE )
