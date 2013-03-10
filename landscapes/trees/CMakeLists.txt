
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/trees )

# install texture
INSTALL (FILES trees_512.png DESTINATION share/${PACKAGE}/landscapes/trees )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/trees FILES_MATCHING PATTERN "description.*.utf8" )
