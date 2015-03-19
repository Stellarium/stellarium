
########### install files ###############

INSTALL(FILES scenery3d.ini Stellarium-Testscene_ground.mtl Stellarium-Testscene_ground.obj Stellarium-Testscene_withGrid_Bump-separateSpheres.mtl Stellarium-Testscene_withGrid_Bump-separateSpheres.obj.gz 
		DESTINATION share/${PACKAGE}/scenery3d/Testscene)

ADD_SUBDIRECTORY( TX_Stellarium-Testscene )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_ground )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_withGrid )
