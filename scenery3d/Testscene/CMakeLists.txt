
########### install files ###############
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/scenery3d/Testscene
		  FILES_MATCHING
		  PATTERN "*.ini"
		  PATTERN "*.mtl"
		  PATTERN "*.obj"
		  PATTERN "*.obj.gz"
		  PATTERN "description.*.utf8"
		  PATTERN "*.png"
		  PATTERN "CMakeFiles" EXCLUDE )


ADD_SUBDIRECTORY( TX_Stellarium-Testscene )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_ground )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_withGrid )
