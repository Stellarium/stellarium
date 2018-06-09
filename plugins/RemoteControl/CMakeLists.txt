# This is the cmake config file for the RemoteControl plugin
SET(REMOTECONTROL_VERSION "0.0.3")
ADD_DEFINITIONS(-DREMOTECONTROL_PLUGIN_VERSION="${REMOTECONTROL_VERSION}")
ADD_DEFINITIONS(-DREMOTECONTROL_PLUGIN_LICENSE="GNU GPLv2+")
SET(REMOTECONTROL_WEBROOT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/webroot/")
ADD_DEFINITIONS(-DREMOTECONTROL_WEBROOT_PATH="${REMOTECONTROL_WEBROOT_PATH}")

ADD_SUBDIRECTORY( src )

# Custom target for updating the translationdata.js
find_package(PythonInterp)

if(PYTHON_EXECUTABLE)
	add_custom_target(RemoteControl-update-translationdata
		COMMAND ${PYTHON_EXECUTABLE} util/update_translationdata.py 
		${PROJECT_SOURCE_DIR}/po/stellarium-remotecontrol/stellarium-remotecontrol.jst ${REMOTECONTROL_WEBROOT_PATH}/js/translationdata.js
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Updating RemoteControl translation data"
		VERBATIM
		)
		
	add_dependencies(RemoteControl-update-translationdata generate-pot-stellarium-remotecontrol-js)

    SET_TARGET_PROPERTIES(RemoteControl-update-translationdata PROPERTIES FOLDER "plugins/RemoteControl/util")
endif()

IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/RemoteControl")


########### install files ###############
# install webroot
INSTALL(DIRECTORY ${REMOTECONTROL_WEBROOT_PATH} DESTINATION share/${PACKAGE}/webroot FILES_MATCHING
	PATTERN "*.png" 
	PATTERN "*.gif"
	PATTERN "*.html"
	PATTERN "*.js"
	PATTERN "*.css"
	PATTERN "*.eot"
	PATTERN "*.svg"
	PATTERN "*.ttf"
	PATTERN "*.woff"
	PATTERN "*.woff2"
	PATTERN "translate_files"
	PATTERN "*.ico")
