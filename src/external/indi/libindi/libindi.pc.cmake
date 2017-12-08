prefix=@CMAKE_INSTALL_PREFIX@
exec_prefix=@CMAKE_INSTALL_PREFIX@
libdir=@PKG_CONFIG_LIBDIR@
includedir=@INCLUDE_INSTALL_DIR@

Name: libindi
Description: Instrument Neutral Distributed Interface
URL: http://www.indilib.org/
Version: @CMAKE_INDI_VERSION_STRING@
Libs: -L${libdir} @PKG_CONFIG_LIBS@
Libs.private: -lz -lcfitsio -lnova
Cflags: -I${includedir} -I${includedir}/libindi

