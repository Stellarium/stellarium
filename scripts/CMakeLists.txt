########### install files ###############

INSTALL(FILES startup.ssc landscapes.ssc lunar_partial.ssc lunar_total.ssc screensaver.ssc common_objects.inc solar_eclipse.ssc zodiac.ssc triple_sunrise_and_sunsets.ssc phobos_phun_1.ssc phobos_phun_2.ssc phobos_phun_3.ssc phobos_phun_4.ssc phobos_phun_5.ssc solar_system_screensaver.ssc constellations_tour.ssc sun.ssc earth_1.ssc earth_2.ssc earth_3.ssc earth_4.ssc earth_5.ssc earth_6.ssc transit_of_venus.ssc analemma.ssc supernova.ssc sky_cultures.ssc DESTINATION share/${PACKAGE}/scripts)

# FIXME: It's temporaty solution for public testing of media support
INSTALL(FILES GZ_videotest_WMV.ssc GZ_videotest_MP4.ssc GZ_videotest_MP4_extended.ssc  GZ_videotest_popout_1.ssc GZ_videotest_popout_2.ssc DESTINATION share/${PACKAGE}/scripts)
INSTALL(FILES media/stellarium.mp4 media/karnak.wmv  DESTINATION share/${PACKAGE}/scripts/media)


