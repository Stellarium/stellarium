
########### install files ###############

# install models
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/models
	FILES_MATCHING 
	PATTERN "*.mtl"
	PATTERN "*.png"
	PATTERN "*.obj"
	PATTERN "*.gzip"
	PATTERN "CMakeFiles" EXCLUDE )

INSTALL(DIRECTORY blender/ DESTINATION ${SDATALOC}/models/blender
	FILES_MATCHING 
	PATTERN "*.blend"
	PATTERN "CMakeFiles" EXCLUDE )
