
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/moon )

# install texture
INSTALL (FILES apollo17.png DESTINATION share/${PACKAGE}/landscapes/moon )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/moon FILES_MATCHING PATTERN "description.*.utf8" )
