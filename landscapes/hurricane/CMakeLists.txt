
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/hurricane )

# install textures
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/hurricane FILES_MATCHING PATTERN "*.png" )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/hurricane FILES_MATCHING PATTERN "description.*.utf8" )
