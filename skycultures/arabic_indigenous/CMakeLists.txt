
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/arabic_indigenous )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/arabic_indigenous
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "description.*.utf8"
	PATTERN "*.png"
	PATTERN "CMakeFiles" EXCLUDE )
