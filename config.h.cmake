#ifndef CONFIG_H
#define CONFIG_H

#cmakedefine HAVE_TIMEGM
#cmakedefine HAVE_MKTIME
#cmakedefine HAVE_POW10
#cmakedefine HAVE_BYTESWAP_H

#cmakedefine INSTALL_DATADIR "${INSTALL_DATADIR}"
#cmakedefine INSTALL_LOCALEDIR "${INSTALL_LOCALEDIR}"
#cmakedefine PACKAGE_VERSION "${PACKAGE_VERSION}"
#cmakedefine SVN_REVISION "${SVN_REVISION}"
#cmakedefine ENABLE_NLS

#endif
