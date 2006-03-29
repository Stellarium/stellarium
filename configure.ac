dnl Process this file with autoconf to produce a configure script.

AC_INIT(stellarium,0.8.0, stellarium@free.fr)
AC_CONFIG_SRCDIR([src/main.cpp])

AC_PREREQ(2.52)

AM_INIT_AUTOMAKE(1.6)
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


# Check for freetype2
# This macro won't work on my system?!... use the old way until the problem is fixed..
# AC_CHECK_FT2(2.0.4,,)
AC_CHECK_LIB(freetype, FT_Init_FreeType,,AC_MSG_ERROR(freetype library not found, please install libfreetype.))
FT2_CFLAGS="`freetype-config --cflags`"
FT2_LIBS="`freetype-config --libs`"
CFLAGS="$CFLAGS $FT2_CFLAGS"
CXXFLAGS="$CXXFLAGS $FT2_CFLAGS"
LIBS="$LIBS $FT2_LIBS"

# Compilation with STLPort
#AM_PATH_STLPORT

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

AC_CHECK_LIB(SDL_mixer, Mix_OpenAudio,[AC_CHECK_HEADER(SDL_mixer.h,LIBS="$LIBS -lSDL_mixer",,[#include "SDL.h"])], AC_MSG_WARN(*** SDL_mixer library not found - audio features will be deactivated in Stellarium ***))

AC_CHECK_HEADERS(byteswap.h SDL_mixer.h)

dnl assign the package directory variable
case "$host" in
*-*-darwin*)
PKGDATADIR="Stellarium.app/Contents/Resources"
;;
*)
if test "$prefix" = "NONE" ; then
   prefix=$ac_default_prefix
fi
PKGDATADIR=`eval echo "${datadir}/$PACKAGE"`
;;
esac

AC_DEFINE_UNQUOTED(CONFIG_DATA_DIR, "$PKGDATADIR", [Set the directory where the datas (as textures or star catalogs) are stored.])

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
	dnl Check for SDL but can't use AM_PATH_DSDL on Mingw : it crashes..
	dnl So directly call the utility and add an include directory by hand..
	LIBS="$LIBS `sdl-config --libs`"
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
AC_CHECK_FUNCS(toupper strcasecmp strdup sinf cosf tanf asinf acosf atanf expf logf log10f atan2f sqrtf powf modf putenv setlocale tzset strchr pow10 timegm)

AC_CONFIG_FILES([Makefile stellarium.spec src/Makefile src/iniparser/Makefile src/glpng/Makefile src/stellplanet/Makefile src/stellastro/Makefile config/Makefile data/Makefile doc/Makefile textures/Makefile textures/landscapes/Makefile textures/constellation-art/Makefile data/scripts/Makefile data/sky_cultures/Makefile  data/sky_cultures/polynesian/Makefile data/sky_cultures/western/Makefile data/sky_cultures/chinese/Makefile data/sky_cultures/egyptian/Makefile intl/Makefile po/Makefile.in m4/Makefile])

AC_OUTPUT
