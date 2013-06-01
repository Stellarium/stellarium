
########### install files ###############

# install textures info (JSON and FAB)
INSTALL(FILES textures.json nebula_textures.fab DESTINATION share/${PACKAGE}/nebulae/default )

# install DSO catalog
INSTALL(FILES ngc2000.dat ngc2000names.dat DESTINATION share/${PACKAGE}/nebulae/default )

# install textures of DSO
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/nebulae/default 
	FILES_MATCHING PATTERN "*.png" 
	PATTERN "CMakeFiles" EXCLUDE )