SET(stellarium_POT stellarium.pot)

GETTEXT_CREATE_TRANSLATIONS(stellarium DEFAULT_TARGET
    aa ab ae af ak am an ar as ast av az ba be bg bh bi bn bo br bs ca ce ckb cs csb cv cy da de dv el en en_AU en_CA en_GB en_US eo es et eu fa fi fil fj fr fy ga gd gl gn gu gv haw he hi hne hr hrx ht hu hy ia id is it ja jv ka kg kk km kn ko ky la lb lo lt lv mai mi mk ml mn mo mr ms mt na nan nb ne nds nl nn oc os pa pl pt pt_BR ro ru sah sco se si sk sl sm sq sr su sv sw ta te tg th tl tr tt ug uk uz vi wa xh zh_CN zh_HK zh_TW zu
)

# Discard this target if xgettext is not installed
IF(${XGETTEXT_FOUND})
	ADD_CUSTOM_TARGET(
		generate-pot-stellarium
		${GETTEXT_XGETTEXT_EXECUTABLE}
			-o ${CMAKE_CURRENT_SOURCE_DIR}/stellarium.pot
			-C
			--qt
			--keyword=_
			--keyword=N_
			--keyword=q_
			--keyword=qc_:1,2c
			--keyword=translate:2
			--keyword=translate:2,3c
			--add-comments=TRANSLATORS:
			--directory=${CMAKE_BINARY_DIR}
			--directory=${CMAKE_SOURCE_DIR}
			--output-dir=${CMAKE_BINARY_DIR}
			--files-from=${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in
			--copyright-holder=Stellarium's\ team
			--from-code=utf-8
		WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
		COMMENT "Generating stellarium.pot"
		VERBATIM
	)
	# TODO: It would be nice to just depend on the exact files in POTFILES.in
	#file(READ ${CMAKE_CURRENT_SOURCE_DIR}/stellarium.pot UiHeaders)
	#ADD_DEPENDENCIES(generate-pot-stellarium UiHeaders)
	#ADD_DEPENDENCIES(generate-pot-stellarium stellarium_UIS_H)
	# Make sure the UI headers are created first.
	ADD_DEPENDENCIES(generate-pot-stellarium StelGuiLib)
	
	# Generate this POT file when building the "generate-pot" target.
	ADD_DEPENDENCIES(generate-pot generate-pot-stellarium)
ENDIF()
