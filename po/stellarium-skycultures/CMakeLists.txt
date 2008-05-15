SET(skycultures_DOMAIN stellarium-skycultures)
SET(skycultures_POT ${skycultures_DOMAIN}.pot)

GETTEXT_CREATE_TRANSLATIONS(${skycultures_DOMAIN} DEFAULT_TARGET ar  bg  cs  de  en_GB en_AU es  fa  fr  hr  hw  it  kn  lv  nb  nn  pt_BR  ro  sk  sr  tr  vi zh_HK az  ca  da  el  en     et  fi  ga  hu  id  ja  ko  mt  nl  pl  pt     ru  sl  sv  uk  zh_CN  zh_TW th ml he fil tl eo lt mk)

ADD_CUSTOM_TARGET(
    generate-pot-${skycultures_DOMAIN}
    ${GETTEXT_XGETTEXT_EXECUTABLE}
        -o ${skycultures_POT}
        --keyword=_
        -C
        --directory=${PROJECT_SOURCE_DIR}
        --files-from=POTFILES.in
        --copyright-holder=Stellarium's\ team
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating ${skycultures_POT}"
    VERBATIM
)

# Generate this POT file when building the "generate-pot" target.
ADD_DEPENDENCIES(generate-pot generate-pot-${skycultures_DOMAIN})
