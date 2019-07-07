########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/scripts
        FILES_MATCHING PATTERN "*.ssc"
        PATTERN "*.inc"
        PATTERN "CMakeFiles" EXCLUDE
        PATTERN "tests" EXCLUDE)


