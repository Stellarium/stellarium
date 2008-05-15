SET(stellarium_DOMAIN stellarium)
SET(stellarium_POT ${stellarium_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${stellarium_DOMAIN} DEFAULT_TARGET ar  bg  cs  de  en_GB en_AU es  fa  fr  hr  hw  it  kn  lv  nb  nn  pt_BR  ro  sk  sr  tr  vi zh_HK az  ca  da  el  en     et  fi  ga  hu  id  ja  ko  mt  nl  pl  pt     ru  sl  sv  uk  zh_CN  zh_TW th ml he fil tl eo lt mk)

ADD_CUSTOM_TARGET(
    generate-pot-${stellarium_DOMAIN}
    ${GETTEXT_XGETTEXT_EXECUTABLE}
        -o ${stellarium_POT}
        --keyword=_
        --keyword=N_
        --keyword=q_
        --keyword=translate:2
        -C
        --qt
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
