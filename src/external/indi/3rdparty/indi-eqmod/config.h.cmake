#ifndef CONFIG_H
#define CONFIG_H

/* Define INDI Data Dir */
#cmakedefine INDI_DATA_DIR "@INDI_DATA_DIR@"
/* Define Driver version */
#define EQMOD_VERSION_MAJOR @EQMOD_VERSION_MAJOR@
#define EQMOD_VERSION_MINOR @EQMOD_VERSION_MINOR@

#cmakedefine WITH_ALIGN
#cmakedefine WITH_ALIGN_GEEHALEL
#cmakedefine STOP_WHEN_MOTION_CHANGED
#cmakedefine WITH_SCOPE_LIMITS

#define MAX_PATH_LENGTH 512

#endif // CONFIG_H
