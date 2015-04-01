
########### install files ###############

# install models and descriptions
INSTALL(FILES Sterngarten_Wien_innerArea_ground.obj Sterngarten_Wien_innerArea_ground.mtl Sterngarten_Wien_innerArea-optimized.obj.gz Sterngarten_Wien_innerArea-optimized.mtl DESTINATION share/${PACKAGE}/scenery3d/Sterngarten)
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/scenery3d/Sterngarten 
				  FILES_MATCHING
				  PATTERN "*.ini"
				  PATTERN "description.*.utf8"
				  PATTERN "*.png"
				  PATTERN "CMakeFiles" EXCLUDE )

ADD_SUBDIRECTORY( Sterngarten_Wien_innerArea )
