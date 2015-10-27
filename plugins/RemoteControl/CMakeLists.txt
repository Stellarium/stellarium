# This is the cmake config file for the RemoteControl plugin
SET(REMOTECONTROL_VERSION "0.0.1")
ADD_DEFINITIONS(-DREMOTECONTROL_VERSION="${REMOTECONTROL_VERSION}")

ADD_SUBDIRECTORY( src )

# Custom target for updating the translationdata.js
find_package(PythonInterp)

if(PYTHON_EXECUTABLE)
	add_custom_target(RemoteControl-update-translationdata
		COMMAND ${PYTHON_EXECUTABLE} util/update_translationdata.py 
		${PROJECT_SOURCE_DIR}/po/stellarium-remotecontrol/stellarium-remotecontrol-js.pot ${PROJECT_SOURCE_DIR}/data/webroot/js/translationdata.js
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Updating RemoteControl translation data"
		VERBATIM
		)
		
	add_dependencies(RemoteControl-update-translationdata generate-pot-stellarium-remotecontrol-js)
endif()

IF(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/Library/Application\ Support/Stellarium)
ElSE(APPLE)
    SET(CMAKE_INSTALL_PREFIX $ENV{HOME}/.stellarium)
ENDIF(APPLE)
INSTALL(FILES DESTINATION "modules/RemoteControl")
