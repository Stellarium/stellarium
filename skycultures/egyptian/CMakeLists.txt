
########### install files ###############

# install info.ini
INSTALL(FILES info.ini DESTINATION share/${PACKAGE}/skycultures/egyptian)

# install star_names.fab
INSTALL(FILES star_names.fab DESTINATION share/${PACKAGE}/skycultures/egyptian)

# install constellations data
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/egyptian
	FILES_MATCHING PATTERN "constellation*" )

# install artwork
#INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/egyptian
#	FILES_MATCHING PATTERN "*.png" )

# install descriptions
INSTALL(DIRECTORY ./ DESTINATION share/${PACKAGE}/skycultures/egyptian
	FILES_MATCHING PATTERN "description.*.utf8" )
