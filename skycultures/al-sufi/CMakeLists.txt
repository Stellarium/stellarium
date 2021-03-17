
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/al-sufi )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/al-sufi
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "description.*.utf8"
	PATTERN "*.png"
	PATTERN "CMakeFiles" EXCLUDE )
