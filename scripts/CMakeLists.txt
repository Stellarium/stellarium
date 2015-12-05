########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/scripts
        FILES_MATCHING PATTERN "*.ssc"
        PATTERN "*.inc"
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "tests" EXCLUDE)


# FIXME: It's temporaty solution for public testing of media support
INSTALL(FILES GZ_videotest_WMV.ssc GZ_videotest_MP4.ssc GZ_videotest_MP4_extended.ssc  GZ_videotest_popout_1.ssc GZ_videotest_popout_2.ssc DESTINATION share/${PACKAGE}/scripts)
INSTALL(FILES media/stellarium.mp4 media/karnak.wmv  DESTINATION share/${PACKAGE}/scripts/media)


