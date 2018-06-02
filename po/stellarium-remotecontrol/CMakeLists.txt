FILE(GLOB porcfiles RELATIVE "${CMAKE_SOURCE_DIR}/po/stellarium-remotecontrol/" "*.po")
LIST(SORT porcfiles)
STRING(REGEX REPLACE ".po" "" outporcfiles "${porcfiles}")

GETTEXT_CREATE_TRANSLATIONS(stellarium-remotecontrol DEFAULT_TARGET ${outporcfiles})

# Discard this target if xgettext is not installed
IF(${XGETTEXT_FOUND})
	# Extract string from Javascript
	# Note: This .pot should NOT be provided to translators, it is only required for correctly creating the translationdata.js file through the update_translationdata.py script
	ADD_CUSTOM_TARGET(
		generate-pot-stellarium-remotecontrol-js
		${GETTEXT_XGETTEXT_EXECUTABLE}
			-o stellarium-remotecontrol.jst
			--keyword=tr
			--language=JavaScript
			--add-comments=TRANSLATORS:
			--directory=${PROJECT_SOURCE_DIR}
			--files-from=${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in
			--copyright-holder=Stellarium's\ team
			--msgid-bugs-address=stellarium@googlegroups.com
			--from-code=utf-8
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating stellarium-remotecontrol.jst"
		VERBATIM
	)
	
	# We use PHP mode to extract text from HTML
	# This file also contains the JS strings, so it is the only one that must be translated!
	ADD_CUSTOM_TARGET(
		generate-pot-stellarium-remotecontrol
		COMMAND ${CMAKE_COMMAND} -E copy stellarium-remotecontrol.jst stellarium-remotecontrol.pot
		COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
			-o stellarium-remotecontrol.pot
			--join-existing
			--keyword=tr
			--language=PHP
			--add-comments=TRANSLATORS:
			--directory=${PROJECT_SOURCE_DIR}/plugins/RemoteControl/webroot
			--copyright-holder=Stellarium's\ team
			--msgid-bugs-address=stellarium@googlegroups.com
			--package-name=stellarium-remotecontrol
			--from-code=utf-8
			index.html tablet7in.html
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		COMMENT "Generating stellarium-remotecontrol.pot"
		VERBATIM
	)
	
	ADD_DEPENDENCIES(generate-pot-stellarium-remotecontrol generate-pot-stellarium generate-pot-stellarium-remotecontrol-js)
	
	# custom command to remove messages that already exist in the base .pot
	# find the msgattrib tool
	find_program(GETTEXT_MSGATTRIB_EXECUTABLE msgattrib)
	
	IF(GETTEXT_MSGATTRIB_EXECUTABLE)		
		ADD_CUSTOM_COMMAND(
			TARGET generate-pot-stellarium-remotecontrol
			POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E echo "** Removing entries from stellarium-remotecontrol.pot that exist in stellarium.pot"
			# set the obsolete flag for all entries that are ALSO in stellarium.pot
			COMMAND ${GETTEXT_MSGATTRIB_EXECUTABLE}
				--set-obsolete --only-file=../stellarium/stellarium.pot
				-o stellarium-remotecontrol.pot stellarium-remotecontrol.pot
			# remove the obsolete entries
			COMMAND ${GETTEXT_MSGATTRIB_EXECUTABLE}
				--no-obsolete -o stellarium-remotecontrol.pot stellarium-remotecontrol.pot
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			VERBATIM
		)
	ELSE()
		MESSAGE(WARNING "msgattrib not found. Can not simplify stellarium-remotecontrol.pot")
	ENDIF()
	
	# Generate this POT file when building the "generate-pot" target.
	ADD_DEPENDENCIES(generate-pot generate-pot-stellarium-remotecontrol)
ENDIF()
