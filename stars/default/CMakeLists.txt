
########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/stars/default
        FILES_MATCHING PATTERN "*.json"
        PATTERN "*.dat"
        PATTERN "*.fab"
        PATTERN "*.cat"
        PATTERN "CMakeFiles" EXCLUDE)

