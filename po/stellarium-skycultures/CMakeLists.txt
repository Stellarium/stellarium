SET(skycultures_DOMAIN stellarium-skycultures)
SET(skycultures_POT ${skycultures_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${skycultures_DOMAIN} DEFAULT_TARGET
    af ar az be bg bn bs ca cs cy da de dv el en en_AU en_CA en_GB eo es et eu fa fil fi fr ga gl gu haw he hi hr hrx hu hy ia id is it ja ka kn ko lt lv mk ml mr ms mt mi nan nb nl nn pl pt_BR pt ro ru se si sk sl sq sr sv te th tl tr uk vi zh_CN zh_HK zh_TW
)

ADD_CUSTOM_TARGET(
    generate-pot-${skycultures_DOMAIN}
    ${GETTEXT_XGETTEXT_EXECUTABLE}
        -o ${skycultures_POT}
        --keyword=_
        -C
        --directory=${PROJECT_SOURCE_DIR}
        --files-from=POTFILES.in
        --copyright-holder=Stellarium's\ team
	--from-code=utf-8
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ${skycultures_POT}"
    VERBATIM
)

# Generate this POT file when building the "generate-pot" target.
ADD_DEPENDENCIES(generate-pot generate-pot-${skycultures_DOMAIN})
