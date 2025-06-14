get_filename_component(/skycultures/ruanui_sky_tahiti_and_society_islands "${CMAKE_CURRENT_SOURCE_DIR}" REALPATH)
get_filename_component(ruanui_sky_tahiti_and_society_islands ${/skycultures/ruanui_sky_tahiti_and_society_islands} NAME)

install(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/${ruanui_sky_tahiti_and_society_islands}
        FILES_MATCHING PATTERN "*"
        PATTERN "CMakeLists.txt" EXCLUDE)

