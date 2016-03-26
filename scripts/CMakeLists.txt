########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/scripts
        FILES_MATCHING PATTERN "*.ssc"
        PATTERN "*.inc"
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "tests" EXCLUDE)


