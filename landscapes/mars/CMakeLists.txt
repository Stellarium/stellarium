
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/mars )

# install texture
INSTALL (FILES mars.png DESTINATION share/${PACKAGE}/landscapes/mars )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/mars FILES_MATCHING PATTERN "description.*.utf8" )
