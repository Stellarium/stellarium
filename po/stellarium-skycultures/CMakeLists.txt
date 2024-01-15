FILE(GLOB poscfiles RELATIVE "${CMAKE_SOURCE_DIR}/po/stellarium-skycultures/" "*.po")
LIST(SORT poscfiles)
STRING(REGEX REPLACE ".po" "" outposcfiles "${poscfiles}")

# This is a copy of the implementation of GETTEXT_CREATE_TRANSLATIONS, but
# without updating the .po or .pot files, because they are made by an external
# script from a separate repository.
set(_domain stellarium-skycultures)

file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/translations/${_domain})

SET(_gmoFiles)

SET(_addToAll "ALL")

FOREACH (_lang ${outposcfiles})
     GET_FILENAME_COMPONENT(_absFile ${_lang}.po ABSOLUTE)
     FILE(RELATIVE_PATH _relFile ${PROJECT_SOURCE_DIR} ${_absFile})
     SET(_gmoFile ${CMAKE_BINARY_DIR}/translations/${_domain}/${_lang}.qm)

     # Convert a PO file into a qm file.
     ADD_CUSTOM_COMMAND(
          OUTPUT ${_gmoFile}
          COMMAND ${lconvert_executable} -i ${_absFile} -o ${_gmoFile}
          DEPENDS ${_absFile}
     )

     INSTALL(FILES ${_gmoFile} DESTINATION ${SDATALOC}/translations/${_domain}/)
     SET(_gmoFiles ${_gmoFiles} ${_gmoFile})
ENDFOREACH (_lang)

# Create the .qm files when building the "translations-<DOMAIN>" and
# "translations" targets.
ADD_CUSTOM_TARGET(translations-${_domain} ${_addToAll} DEPENDS ${_gmoFiles})
ADD_DEPENDENCIES(translations translations-${_domain})
SET_TARGET_PROPERTIES(translations-${_domain} PROPERTIES FOLDER "po")
