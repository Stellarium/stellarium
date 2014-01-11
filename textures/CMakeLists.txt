########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/textures
        FILES_MATCHING PATTERN "*.png"
        PATTERN "logo24bit*.png" EXCLUDE
        PATTERN "CMakeFiles" EXCLUDE )


