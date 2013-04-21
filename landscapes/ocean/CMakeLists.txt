
########### install files ###############

# install landscape.ini
INSTALL (FILES landscape.ini DESTINATION share/${PACKAGE}/landscapes/ocean )

# install textures
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/ocean FILES_MATCHING PATTERN "*.png" )

# install descriptions
INSTALL (DIRECTORY ./ DESTINATION share/${PACKAGE}/landscapes/ocean FILES_MATCHING PATTERN "description.*.utf8" )
