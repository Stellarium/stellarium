
########### install directories and files ###############

# install default Earth atmosphere model
IF(ENABLE_SHOWMYSKY)
     INSTALL(DIRECTORY default DESTINATION ${SDATALOC}/atmosphere)
ENDIF()
INSTALL(FILES lightweight.amsh DESTINATION ${SDATALOC}/atmosphere)
