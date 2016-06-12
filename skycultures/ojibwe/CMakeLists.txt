
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/ojibwe )

# install constellations data, proper names data, artwork and descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/ojibwe
	FILES_MATCHING PATTERN "constellation*" 
	PATTERN "*_names.fab" 
	PATTERN "*.png" 
	PATTERN "description.*.utf8" 
	PATTERN "CMakeFiles" EXCLUDE )
