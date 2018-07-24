SET(stellarium_POT stellarium.pot)

FILE(GLOB pofiles RELATIVE "${CMAKE_SOURCE_DIR}/po/stellarium/" "*.po")
LIST(SORT pofiles)
STRING(REGEX REPLACE ".po" "" outpofiles "${pofiles}")

GETTEXT_CREATE_TRANSLATIONS(stellarium DEFAULT_TARGET ${outpofiles})

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
               --msgid-bugs-address=stellarium@googlegroups.com
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
     # NOTE: StelGuiLib was removed in series 0.13
     #ADD_DEPENDENCIES(generate-pot-stellarium StelGuiLib)
     ADD_DEPENDENCIES(generate-pot-stellarium stelMain)

     # Generate this POT file when building the "generate-pot" target.
     ADD_DEPENDENCIES(generate-pot generate-pot-stellarium)
ENDIF()
