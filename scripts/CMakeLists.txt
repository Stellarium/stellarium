########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/scripts
        FILES_MATCHING PATTERN "*.ssc"
        PATTERN "*.inc"
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "tests" EXCLUDE)


# FIXME: A temporary solution for public testing of media support. Remove before building 0.15
INSTALL(FILES videotest.ssc videotest_extended.ssc videotest_popout.ssc  DESTINATION share/${PACKAGE}/scripts/tests)
INSTALL(FILES tests/media/stellarium.mp4 DESTINATION share/${PACKAGE}/scripts/tests/media)


