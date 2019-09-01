
########### install files ###############

# install models and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/scenery3d/Sterngarten
		  FILES_MATCHING
		  PATTERN "*.ini"
		  PATTERN "*.mtl"
		  PATTERN "*.obj"
		  PATTERN "*.obj.gz"
		  PATTERN "description.*.utf8"
		  PATTERN "*.png"
		  PATTERN "CMakeFiles" EXCLUDE )


ADD_SUBDIRECTORY( Sterngarten_Wien_innerArea )
