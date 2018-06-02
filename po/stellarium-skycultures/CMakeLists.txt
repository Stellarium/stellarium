FILE(GLOB poscfiles RELATIVE "${CMAKE_SOURCE_DIR}/po/stellarium-skycultures/" "*.po")
LIST(SORT poscfiles)
STRING(REGEX REPLACE ".po" "" outposcfiles "${poscfiles}")

GETTEXT_CREATE_TRANSLATIONS(stellarium-skycultures DEFAULT_TARGET ${outposcfiles})

# Discard this target if xgettext is not installed
IF(${XGETTEXT_FOUND})
     ADD_CUSTOM_TARGET(
          generate-pot-stellarium-skycultures
          ${GETTEXT_XGETTEXT_EXECUTABLE}
               -o stellarium-skycultures.pot
               --keyword=_:1,2c
               --language=Python
               --add-comments=TRANSLATORS:
               --directory=${PROJECT_SOURCE_DIR}
               --files-from=POTFILES.in
               --copyright-holder=Stellarium's\ team
               --msgid-bugs-address=stellarium@googlegroups.com
               --from-code=utf-8
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMENT "Generating stellarium-skycultures.pot"
          VERBATIM
     )

     # Generate this POT file when building the "generate-pot" target.
     ADD_DEPENDENCIES(generate-pot generate-pot-stellarium-skycultures)
ENDIF()
