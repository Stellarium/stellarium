
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION ${SDATALOC}/skycultures/mongolian )

# install constellations data, proper names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION ${SDATALOC}/skycultures/mongolian
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*_names.fab" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
