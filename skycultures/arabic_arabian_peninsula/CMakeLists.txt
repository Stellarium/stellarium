
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/arabic_arabian_peninsula )

# install constellations data, native names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/arabic_arabian_peninsula
	FILES_MATCHING PATTERN "*.fab" 
	PATTERN "description.*.utf8"
	PATTERN "*.jpg"
	PATTERN "CMakeFiles" EXCLUDE )
