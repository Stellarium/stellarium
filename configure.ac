dnl Process this file with autoconf to produce a configure script.

AC_INIT(stellarium,0.9.0, stellarium@free.fr)
AC_CONFIG_SRCDIR([src/main.cpp])

AC_PREREQ(2.52)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AM_GNU_GETTEXT

dnl Check for compilers.
AC_PROG_CC
AC_PROG_CXX

CFLAGS="$CFLAGS -Wall"
CXXFLAGS="$CXXFLAGS -Wall"

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
AC_PROG_CXX
AC_PROG_RANLIB

PKG_CHECK_MODULES(GLIB, glib-2.0, [AC_DEFINE_UNQUOTED(HAVE_GLIB, 1, [Define if glib-2.0 is present.])], [AC_DEFINE_UNQUOTED(HAVE_GLIB, 0, [Define if glib-2.0 is present.])])
CXXFLAGS="$CXXFLAGS $GLIB_CFLAGS"
LIBS="$LIBS $GLIB_LIBS"
PKG_CHECK_MODULES(GMODULE, gmodule-2.0, [AC_DEFINE_UNQUOTED(HAVE_GMODULE, 1, [Define if gmodule-2.0 is present.])], [AC_DEFINE_UNQUOTED(HAVE_GLIB, 0, [Define if gmodule-2.0 is present.])])
CXXFLAGS="$CXXFLAGS $GMODULE_CFLAGS"
LIBS="$LIBS $GMODULE_LIBS"

# Check for freetype2
# This macro won't work on my system?!... use the old way until the problem is fixed..
#AC_CHECK_FT2(2.0.4,,)
AC_CHECK_LIB(freetype, FT_Init_FreeType,,AC_MSG_ERROR(freetype library not found, please install libfreetype.))
FT2_CFLAGS="`freetype-config --cflags`"
FT2_LIBS="`freetype-config --libs`"
CFLAGS="$CFLAGS $FT2_CFLAGS"
CXXFLAGS="$CXXFLAGS $FT2_CFLAGS"
LIBS="$LIBS $FT2_LIBS"

# Compilation with STLPort
#AM_PATH_STLPORT

AX_BOOST_BASE

# Check for libcurl
LIBCURL_CHECK_CONFIG([yes], , [haveCurl=yes], [haveCurl=no])
if test "${haveCurl}" != yes; then
	AC_MSG_WARN([libcurl library not found, please install it to enable remote file download.])
else
	CFLAGS="$CFLAGS $LIBCURL_CPPFLAGS"
	CXXFLAGS="$CXXFLAGS $LIBCURL_CPPFLAGS"
	LIBS="$LIBS $LIBCURL"
fi

dnl Can't find later libGLU if -lm not set here!?!??!
AC_CHECK_LIB(m,fabs,LIBS="$LIBS -lm",AC_MSG_RESULT(math library not found))

AC_C_BIGENDIAN
AC_HEADER_DIRENT

dnl Check for X11.
AC_PATH_XTRA
LIBS="$LIBS $X_LIBS"
CFLAGS="$CFLAGS $X_CFLAGS"

dnl Check for SDL
case "$host" in
*-*-mingw32)
;;
*-*-cygwin)
;;
*)
SDL_VERSION=1.2.0
	AM_PATH_SDL($SDL_VERSION,:,AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!]))
	CFLAGS="$CFLAGS $SDL_CFLAGS"
	CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
	LIBS="$LIBS $SDL_LIBS"
;;
esac

# Check for libpng
AC_CHECK_LIB(png, png_read_info,[AC_CHECK_HEADER(png.h,LIBS="$LIBS -lpng -lz",)], AC_MSG_ERROR(*** PNG library not found ***), -lz -lm)

# Check for libjpeg
AC_CHECK_LIB(jpeg, jpeg_read_header,[AC_CHECK_HEADER(jpeglib.h,LIBS="$LIBS -ljpeg ",)], AC_MSG_ERROR(*** JPEG library not found ***), -lm)


AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,[AC_CHECK_HEADER(SDL_mixer.h,LIBS="$LIBS -lSDL_mixer",,[#include "SDL.h"])], AC_MSG_WARN(*** SDL_mixer library not found - audio features will be deactivated in Stellarium ***))

AC_CHECK_HEADERS(byteswap.h SDL_mixer.h)

# Fix a bug in recent autoconf versions..
if test $prefix = "NONE"; then
	prefix=$ac_default_prefix
fi
datadir=`eval echo "${datadir}"`
datadir=`eval echo "${datadir}"`

dnl assign the package directory variable
case "$host" in
*-*-darwin*)
PKGDATADIR="Stellarium.app/Contents/Resources"
;;
*)
PKGDATADIR=`eval echo "${datadir}/$PACKAGE"`
;;
esac

AC_DEFINE_UNQUOTED(CONFIG_DATA_DIR, "$PKGDATADIR", [Set the directory where the data (as textures or star catalogs) are stored.])

localedir=`eval echo "${datadir}/locale"`
AC_DEFINE_UNQUOTED(LOCALEDIR, "$localedir", [Set the directory where the locales (translations) are stored.])

dnl For each host type, set the appropriate options
dnl MACOSX
case "$host" in
*-*-darwin*)
	AC_DEFINE_UNQUOTED(MACOSX,1,[If we are on macosx plateform.])
	LDFLAGS="$LDFLAGS -flat_namespace -undefined suppress -lz "
	LIBS="$LIBS -framework OpenGL -lobjc "
;;

dnl CYGWIN
*-*-cygwin*)
	AC_DEFINE_UNQUOTED(CYGWIN,1, [If we are on cygwin plateform.])
	LIBS="$LIBS -lopengl32 -lglu32 -lSDLmain -lSDL -mwindows -mthreads "
	CFLAGS="$CFLAGS -I/usr/local/include/SDL -Dmain=SDL_main -DWIN32 -Uunix -mthreads"
	CXXFLAGS="$CXXFLAGS -I/usr/local/include/SDL -Dmain=SDL_main -DWIN32 -Uunix -mthreads"
;;

dnl MINGW32
*-*-mingw32)
	AC_DEFINE_UNQUOTED(MINGW32,1, [If we are on mingw32 plateform.])
	LIBS="$LIBS -lopengl32 -lglu32"
	dnl Check for SDL but can't use AM_PATH_SDL on Mingw : it crashes..
	dnl So directly call the utility and add an include directory by hand..
	LIBS="$LIBS `sdl-config --libs` -lws2_32"
	CFLAGS="$CFLAGS `sdl-config --cflags`"
	CXXFLAGS="$CXXFLAGS `sdl-config --cflags` -I/mingw/include/SDL/"
;;

dnl OTHER SYSTEMS
*)
	AC_CHECK_LIB(GL,glEnable)
   	if test "$ac_cv_lib_GL_glEnable" = no ; then
    	AC_CHECK_LIB(MesaGL,glEnable,,AC_MSG_ERROR(GL not found - please install GL or MesaGL))
   	fi

   	AC_CHECK_LIB(GLU,gluLookAt)
   	if test "$ac_cv_lib_GLU_gluLookAt" = no ; then
    	AC_CHECK_LIB(MesaGLU,gluLookAt,,AC_MSG_ERROR(GLU not found - please install GLU or MesaGLU))
   	fi

;;
esac


dnl Checks for library functions.
AC_CHECK_FUNCS(toupper strcasecmp strdup putenv setlocale tzset strchr pow10 timegm)

AC_CONFIG_FILES([Makefile stellarium.spec src/Makefile src/stelutils/iniparser/Makefile src/planetsephems/Makefile src/stelutils/Makefile data/Makefile doc/Makefile textures/Makefile textures/landscapes/Makefile textures/constellation-art/Makefile textures/constellation-art/inuit/Makefile data/scripts/Makefile data/sky_cultures/Makefile  data/sky_cultures/polynesian/Makefile data/sky_cultures/western/Makefile data/sky_cultures/chinese/Makefile data/sky_cultures/egyptian/Makefile data/sky_cultures/korean/Makefile data/sky_cultures/navajo/Makefile data/sky_cultures/lakota/Makefile data/sky_cultures/inuit/Makefile intl/Makefile po/Makefile.in m4/Makefile])

AC_OUTPUT
