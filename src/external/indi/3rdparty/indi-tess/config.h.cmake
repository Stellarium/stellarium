#ifndef CONFIG_H
#define CONFIG_H

/* Define INDI Data Dir */
#cmakedefine INDI_DATA_DIR "@INDI_DATA_DIR@"
/* Define Driver version */
#define TESS_VERSION_MAJOR @TESS_VERSION_MAJOR@
#define TESS_VERSION_MINOR @TESS_VERSION_MINOR@

#define DEFAULT_SKELETON_FILE "@INDI_DATA_DIR@/tess_sk.xml"

#endif // CONFIG_H
