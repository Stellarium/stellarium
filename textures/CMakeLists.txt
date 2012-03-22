########### install files ###############

INSTALL(FILES amalthea.png proteus.png triton.png mercury.png venus.png earth-clouds.png earthmap.png mars.png jupiter.png saturn.png uranus.png neptune.png earth-shadow.png saturn_rings_radial.png uranus_rings.png neptune_rings.png lune.png phobos.png deimos.png io.png europa.png ganymede.png callisto.png mimas.png enceladus.png tethys.png dione.png rhea.png titan.png hyperion.png iapetus.png miranda.png ariel.png umbriel.png titania.png oberon.png fog.png milkyway.png fogridge.png ocl.png gcl.png pnb.png neb.png halo.png pointeur2.png pointeur4.png pointeur5.png star16x16.png sun.png nomap.png haloLune.png planet-indicator.png DESTINATION share/${PACKAGE}/textures)

IF(${STELLARIUM_SPLASH} MATCHES "Development")
  INSTALL(FILES logo24bits_dev.png DESTINATION share/${PACKAGE}/textures RENAME logo24bits.png)
ELSEIF(${STELLARIUM_SPLASH} MATCHES "ReleaseCandidate")
  INSTALL(FILES logo24bits_rc.png DESTINATION share/${PACKAGE}/textures RENAME logo24bits.png)
ELSE()
  INSTALL(FILES logo24bits_rel.png DESTINATION share/${PACKAGE}/textures RENAME logo24bits.png)
ENDIF()
