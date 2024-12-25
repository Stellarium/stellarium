
########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/stars/hip_gaia3
        FILES_MATCHING PATTERN "*.json"
        PATTERN "*.dat"
        PATTERN "*.fab"
        PATTERN "*.cat"
        PATTERN "CMakeFiles" EXCLUDE)

