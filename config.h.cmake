#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine HAVE_UNISTD_H
#cmakedefine HAVE_STRING_H
#cmakedefine HAVE_TIMEGM
#cmakedefine HAVE_MKTIME
#cmakedefine HAVE_POW10
#cmakedefine HAVE_SETLOCALE
#cmakedefine HAVE_LIBCURL
#cmakedefine HAVE_SNPRINTF
#cmakedefine HAVE_BYTESWAP_H
#cmakedefine WORDS_BIGENDIAN

/* Define whether QT4 is to be used. */
#cmakedefine USE_QT4

/* Define whether SDL is to be used. */
#cmakedefine USE_SDL

#define PACKAGE_NAME "stellarium"

#cmakedefine INSTALL_DATADIR "${INSTALL_DATADIR}"
#cmakedefine INSTALL_LOCALEDIR "${INSTALL_LOCALEDIR}"
#cmakedefine PACKAGE_VERSION "${PACKAGE_VERSION}"

#cmakedefine ENABLE_NLS ${ENABLE_NLS}

#endif
