SET(stellarium_DOMAIN stellarium)
SET(stellarium_POT ${stellarium_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${stellarium_DOMAIN} DEFAULT_TARGET
    af ar az be bg bn br bs ca cs cy da de el en_AU en_CA en_GB en eo es et eu fa fil fi fr fy ga gl haw he hr hu hy ia id is it ja ka kn ko lt lv mk ml mr ms mt nb nl nn pl pt_BR pt ro ru se si sk sl sq sr sv te th tl tr uk vi zh_CN zh_HK zh_TW
)

ADD_CUSTOM_TARGET(
    generate-pot-${stellarium_DOMAIN}
    ${GETTEXT_XGETTEXT_EXECUTABLE}
        -o ${stellarium_POT}
        -C
        --qt
        --keyword=_
        --keyword=N_
        --keyword=q_
        --keyword=translate:2
        --add-comments=TRANSLATORS:
        --directory=${PROJECT_SOURCE_DIR}
        --directory=${CMAKE_BINARY_DIR}
        --files-from=POTFILES.in
        --copyright-holder=Stellarium's\ team
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ${stellarium_POT}"
    VERBATIM
)

# Make sure the UI headers are created first.
ADD_DEPENDENCIES(generate-pot-${stellarium_DOMAIN} GenerateUiHeaders)

# Generate this POT file when building the "generate-pot" target.
ADD_DEPENDENCIES(generate-pot generate-pot-${stellarium_DOMAIN})
