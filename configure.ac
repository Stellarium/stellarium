dnl Process this file with autoconf to produce a configure script.

AC_INIT(stellarium,0.7.0, stellarium@free.fr)
AC_CONFIG_SRCDIR([src/main.cpp])

AC_PREREQ(2.52)

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AM_GNU_GETTEXT_VERSION(0.10.40)
AM_GNU_GETTEXT

dnl Check for compilers.
AC_PROG_CC
AC_PROG_CXX

dnl CFLAGS=" -Wall -s -O3"
dnl CXXFLAGS=" -Wall -s -O3"
CFLAGS=" -g -Wall"
CXXFLAGS=" -g -Wall"
dnl CFLAGS=" -Wall -pg"
dnl CXXFLAGS=" -Wall -pg"

dnl Checks for programs.
AC_PROG_CC
AC_ISC_POSIX
AC_PROG_CXX
AC_PROG_RANLIB

dnl Can't find later libGLU if -lm not set here!?!??!
AC_CHECK_LIB(m,fabs,LIBS="$LIBS -lm",AC_MSG_RESULT(math library not found))

AC_C_BIGENDIAN

dnl Check for X11.
AC_PATH_XTRA
LIBS="$LIBS $X_LIBS"
CFLAGS="$CFLAGS $X_CFLAGS"

dnl Check for SDL
case "$host" in
*-*-darwin*)
SDL_VERSION=1.2.0
dnl AM_PATH_SDL($SDL_VERSION,
dnl            	:,
dnl            	AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
dnl )
;;

dnl Other systems
*)
SDL_VERSION=1.2.0
AM_PATH_SDL($SDL_VERSION,
           	:,
           	AC_MSG_ERROR([*** SDL version $SDL_VERSION not found!])
)
CFLAGS="$CFLAGS $SDL_CFLAGS"
CXXFLAGS="$CXXFLAGS $SDL_CFLAGS"
LIBS="$LIBS $SDL_LIBS"
;;
esac

AC_CHECK_HEADERS(byteswap.h)

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
	LIBS="$LIBS -lopengl32 -lglu32"
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

dnl Cross-compiling MINGW32
i386-pc-mingw32msvc)
	AC_DEFINE_UNQUOTED(MINGW32,1, [If we are on mingw32 plateform.])
	LIBS="$LIBS -lopengl32 -lglu32"
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

dnl AC_CACHE_CHECK(
dnl 	[whether libglgooey is available],
dnl 	[my_cv_glgooey_avail],
dnl 	[my_ac_save_libs=$LIBS
dnl 	LIBS="-lglgooey $LIBS"
dnl 	AC_LANG_PUSH(C++)
dnl 	AC_TRY_LINK(
dnl 	[#include <glgooey/WindowManager.h>],
dnl 	[Gooey::WindowManager();],
dnl 	[my_cv_glgooey_avail=true],
dnl 	[AC_MSG_ERROR(GLGOOEY not found - please install glgooey)])
dnl 	AC_LANG_POP
dnl 	LIBS=$my_ac_save_libs
dnl ])

LIBS="$LIBS -lglgooey"

dnl Checks for library functions.
AC_CHECK_FUNCS(round toupper strcasecmp strdup sinf cosf tanf asinf acosf atanf expf logf log10f atan2f sqrtf powf modf putenv setlocale tzset strchr)


AC_CONFIG_FILES([Makefile stellarium.spec src/Makefile src/iniparser/Makefile src/glpng/Makefile src/stellplanet/Makefile src/stellastro/Makefile config/Makefile data/Makefile doc/Makefile textures/Makefile textures/landscapes/Makefile textures/constellation-art/Makefile data/sky_cultures/Makefile  data/sky_cultures/polynesian/Makefile data/sky_cultures/western/Makefile po/Makefile.in intl/Makefile ])

AC_OUTPUT