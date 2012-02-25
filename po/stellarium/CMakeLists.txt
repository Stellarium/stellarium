SET(stellarium_DOMAIN stellarium)
SET(stellarium_POT ${stellarium_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${stellarium_DOMAIN} DEFAULT_TARGET
    af ar az be bg bn bo br bs ca cs cy da de dv el en_AU en_CA en_GB en eo es et eu fa fil fi fr fy ga gl gu gv haw he hi hr hrx hu hy ia id is it ja ka kn ko lt lv mk ml mr ms mt mi nan nb nl nn oc pl pt_BR pt ro ru sco se si sk sl sq sr sv ta te th tl tr uk vi zh_CN zh_HK zh_TW
)

ADD_CUSTOM_TARGET(
    generate-pot-${stellarium_DOMAIN}
    ${GETTEXT_XGETTEXT_EXECUTABLE}
        -o ${CMAKE_CURRENT_SOURCE_DIR}/${stellarium_POT}
        -C
        --qt
        --keyword=_
        --keyword=N_
        --keyword=q_
        --keyword=translate:2
        --add-comments=TRANSLATORS:
        --directory=${CMAKE_BINARY_DIR}
        --directory=${CMAKE_SOURCE_DIR}
        --output-dir=${CMAKE_BINARY_DIR}
        --files-from=${CMAKE_CURRENT_SOURCE_DIR}/POTFILES.in
        --copyright-holder=Stellarium's\ team
        --from-code=utf-8
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMENT "Generating ${stellarium_POT}"
    VERBATIM
)
# TODO: It would be nice to just depend on the exact files in POTFILES.in
#file(READ ${CMAKE_CURRENT_SOURCE_DIR}/${stellarium_POT} UiHeaders)
#ADD_DEPENDENCIES(generate-pot-${stellarium_DOMAIN} UiHeaders)
#ADD_DEPENDENCIES(generate-pot-${stellarium_DOMAIN} stellarium_UIS_H)
# Make sure the UI headers are created first.
ADD_DEPENDENCIES(generate-pot-${stellarium_DOMAIN} StelGuiLib)

# Generate this POT file when building the "generate-pot" target.
ADD_DEPENDENCIES(generate-pot generate-pot-${stellarium_DOMAIN})
