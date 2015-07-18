
########### install files ###############

# install textures info (JSON and FAB)
INSTALL(FILES textures.json nebula_textures.fab DESTINATION share/${PACKAGE}/nebulae/default )

# install DSO catalogs and textures
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/nebulae/default 
	FILES_MATCHING 
	PATTERN "*.dat"
	PATTREN "*.txt"
	PATTERN "*.png"
	PATTERN "CMakeFiles" EXCLUDE )
