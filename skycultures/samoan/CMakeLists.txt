get_filename_component(skyculturePath "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(skyculture ${skyculturePath} NAME)

install(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/${skyculture}
        FILES_MATCHING PATTERN "*"
        PATTERN "CMakeLists.txt" EXCLUDE)
