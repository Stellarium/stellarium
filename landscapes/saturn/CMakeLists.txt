
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/saturn )

# install texture
INSTALL (FILES saturn-256.png DESTINATION share/${PACKAGE}/landscapes/saturn )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/saturn FILES_MATCHING PATTERN "description.*.utf8" )
