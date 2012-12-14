SET(stellarium_DOMAIN stellarium)
SET(stellarium_POT ${stellarium_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${stellarium_DOMAIN} DEFAULT_TARGET
    aa ab ae af ak am an ar as ast av az ba be bg bh bi bn bo br bs ca ce ckb cs cv cy da de dv el en en_AU en_CA en_GB en_US eo es et eu fa fi fil fj fr fy ga gd gl gn gu gv haw he hi hr hrx ht hu hy ia id is it ja ka kg kk kn ko ky la lb lo lt lv mi mk ml mn mo mr ms mt nan nb nl nn oc os pa pl pt pt_BR ro ru sah sco se si sk sl sm sq sr su sv sw ta te tg th tl tr tt uk uz vi zh_CN zh_HK zh_TW zu
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
