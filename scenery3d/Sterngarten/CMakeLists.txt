
########### install files ###############

# install models and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/scenery3d/Sterngarten 
				  FILES_MATCHING PATTERN "*.obj"
				  PATTERN "*.mtl"
				  PATTERN "*.ini"
				  PATTERN "description.*.utf8"
				  PATTERN "*.png"
				  PATTERN "CMakeFiles" EXCLUDE )

ADD_SUBDIRECTORY( Sterngarten_Wien_innerArea )
