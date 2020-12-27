FILE(GLOB poscrfiles RELATIVE "${CMAKE_SOURCE_DIR}/po/stellarium-scripts/" "*.po")
LIST(SORT poscrfiles)
STRING(REGEX REPLACE ".po" "" outposcrfiles "${poscrfiles}")

GETTEXT_CREATE_TRANSLATIONS(stellarium-scripts DEFAULT_TARGET ${outposcrfiles})

# Discard this target if xgettext is not installed
IF(${XGETTEXT_FOUND})
     ADD_CUSTOM_TARGET(
          generate-pot-stellarium-scripts
          ${GETTEXT_XGETTEXT_EXECUTABLE}
               -o stellarium-scripts.pot
               --keyword=tr
               --language=JavaScript
               --add-comments=TRANSLATORS:
               --directory=${PROJECT_SOURCE_DIR}
               --files-from=POTFILES.in
               --copyright-holder=Stellarium's\ team
               --msgid-bugs-address=stellarium@googlegroups.com
               --from-code=utf-8
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          COMMENT "Generating stellarium-scripts.pot"
          VERBATIM
     )

     # Generate this POT file when building the "generate-pot" target.
     ADD_DEPENDENCIES(generate-pot generate-pot-stellarium-scripts)
ENDIF()
