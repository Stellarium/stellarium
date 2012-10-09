########### install files ###############

INSTALL(FILES amalthea.png proteus.png triton.png pluto.png charon.png moon.png mercury.png venus.png earth_cmap.png earth-clouds.png earthmap.png mars.png jupiter.png saturn.png uranus.png neptune.png earth-shadow.png saturn_rings_radial.png uranus_rings.png neptune_rings.png lune.png phobos.png deimos.png io.png europa.png ganymede.png callisto.png mimas.png enceladus.png tethys.png dione.png rhea.png titan.png hyperion.png iapetus.png miranda.png ariel.png umbriel.png titania.png oberon.png fog.png milkyway.png fogridge.png ocl.png gcl.png pnb.png neb.png halo.png pointeur2.png pointeur4.png pointeur5.png star16x16.png sun.png nomap.png haloLune.png planet-indicator.png corona.png DESTINATION share/${PACKAGE}/textures)

ADD_CUSTOM_TARGET(splash ALL)
SET(_splashFile logo24bits.png)
SET(_splashDstFile ${CMAKE_CURRENT_BINARY_DIR}/${_splashFile})

IF(${STELLARIUM_SPLASH} STREQUAL "Development")
  GET_FILENAME_COMPONENT(_splashSrcFile logo24bits_dev.png ABSOLUTE)
ELSEIF(${STELLARIUM_SPLASH} STREQUAL "ReleaseCandidate")
  GET_FILENAME_COMPONENT(_splashSrcFile logo24bits_rc.png ABSOLUTE)
ELSE()
  GET_FILENAME_COMPONENT(_splashSrcFile logo24bits.png ABSOLUTE)
ENDIF()

ADD_CUSTOM_COMMAND(
  TARGET splash
  COMMAND ${CMAKE_COMMAND} -E copy ${_splashSrcFile} ${_splashDstFile}
)

INSTALL(FILES ${_splashDstFile} DESTINATION share/${PACKAGE}/textures RENAME ${_splashFile})

