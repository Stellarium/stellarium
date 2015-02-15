
########### install files ###############

INSTALL(FILES scenery3d.ini Stellarium-Testscene.mtl Stellarium-Testscene.obj Stellarium-Testscene.xml Stellarium-Testscene_ground.mtl Stellarium-Testscene_ground.obj Stellarium-Testscene_ground.xml Stellarium-Testscene_withGrid.mtl Stellarium-Testscene_withGrid.obj Stellarium-Testscene_withGrid.xml Stellarium-Testscene_withGrid_Bump.mtl Stellarium-Testscene_withGrid_Bump.obj Stellarium-Testscene_withGrid_Bump-separateSpheres.mtl Stellarium-Testscene_withGrid_Bump-separateSpheres.obj DESTINATION share/${PACKAGE}/scenery3d/Testscene)

ADD_SUBDIRECTORY( TX_Stellarium-Testscene )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_ground )
ADD_SUBDIRECTORY( TX_Stellarium-Testscene_withGrid )
