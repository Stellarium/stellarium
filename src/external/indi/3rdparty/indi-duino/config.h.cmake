#ifndef CONFIG_H
#define CONFIG_H

/* Define INDI Data Dir */
#cmakedefine INDI_DATA_DIR "@INDI_DATA_DIR@"
/* Define Driver version */
#define DUINO_VERSION_MAJOR @DUINO_VERSION_MAJOR@
#define DUINO_VERSION_MINOR @DUINO_VERSION_MINOR@

#define DEFAULT_SKELETON_FILE "@INDI_DATA_DIR@/simple_switcher_sk.xml"

#endif // CONFIG_H
