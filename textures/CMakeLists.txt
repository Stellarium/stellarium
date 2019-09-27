########### install files ###############

INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/textures
        FILES_MATCHING PATTERN "*.png"
        PATTERN "CMakeFiles" EXCLUDE )


