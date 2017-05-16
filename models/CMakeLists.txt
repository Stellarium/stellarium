
########### install files ###############

# install models
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/models 
	FILES_MATCHING 
	PATTERN "*.mtl"
	PATTERN "*.png"
	PATTERN "*.obj"
	PATTERN "*.gz"
	PATTERN "CMakeFiles" EXCLUDE )

INSTALL(DIRECTORY blender/ DESTINATION share/${PACKAGE}/models/blender 
	FILES_MATCHING 
	PATTERN "*.blend"
	PATTERN "CMakeFiles" EXCLUDE )
