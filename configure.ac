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

AX_BOOST_BASE
AX_BOOST_THREAD
if test "x$BOOST_THREAD_LIB" = "x"; then
	echo "Sorry, we need the thread lib from Boost."
	echo "Please install from http://www.boost.org"
	exit 1
fi
dnl AX_BOOST_DATE_TIME
dnl AX_BOOST_FILESYSTEM 

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


AC_ARG_WITH([qt42],
             AC_HELP_STRING([--with-qt42=yes],
                            [to specify that we will use QT-4.2.0 instead of SDL.]),
             [WITHQT="$withval"],
             [WITHQT=no])

if test "${WITHQT}" != no ; then
AC_DEFINE_UNQUOTED(USE_QT4, 1, [Define whether QT4 is to be used.])
dnl ---------------------------------------------------------
dnl ---------------------- QT stuff -------------------------
dnl ---------------------------------------------------------
AC_MSG_CHECKING(QT4.2 directory)
QTDIR=xxx
AC_ARG_WITH([qt-dir],
             AC_HELP_STRING([--with-qt-dir=/path/to/Qt-4.2.0],
                            [to specify the path to the Qt-4.2.0 directory.]),
             [QTPATHS="$withval"],
             [QTPATHS="/usr/local/Trolltech/Qt-4.2.0 /usr/share/qt4 /c/Qt/4.2.0 /opt/qt4"])

for x in $QTPATHS; do
    if test -d $x ; then
       QTDIR="$x"
    fi
done
if test $QTDIR = xxx ; then
   AC_MSG_ERROR(Could not locate QT 4.2)
fi
AC_MSG_RESULT($QTDIR)
host=`uname -a` # AC_CANONICAL_HOST is broken at the time of this writing.
case $host in
  MINGW32*)
    AC_MSG_NOTICE(MinGW detected.)
    QTLIBS="-L$QTDIR/lib -lopengl32 -lglu32 -lgdi32 -luser32 -lmingw32 -lqtmain -lQtOpenGL4 -lQtGui4 -lQtCore4 -mthreads -Wl,-enable-stdcall-fixup -Wl,-enable-auto-import -Wl,-enable-runtime-pseudo-reloc -Wl,-s -Wl,-s -Wl,-subsystem,windows"
    QTINC="-I$QTDIR/include -I$QTDIR/include/QtCore -I$QTDIR/include/QtGui -I$QTDIR/include/QtOpenGL -DUNICODE -DQT_LARGEFILE_SUPPORT -DQT_DLL -DQT_NO_DEBUG -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_THREAD_SUPPORT -DQT_NEEDS_QMAIN -frtti -fexceptions"
    QTBIN="$QTDIR/bin"
    ;;
  *)
    AC_MSG_NOTICE(I'm assuming this is Linux)
    AC_PATH_XTRA
    QTLIBS="-Wl,-rpath,$QTDIR/lib -L$QTDIR/lib -lQtGui -lQtOpenGL -lQtCore $X_LIBS -lX11 -lXext -lXmu -lXt -lXi $X_EXTRA_LIBS -lGLU -lGL -lpthread"
    QTINC="-I$QTDIR/include -I$QTDIR/include/QtGui -I$QTDIR/include/QtCore -I$QTDIR/include/QtOpenGL $X_CFLAGS -DQT_OPENGL_LIB -DQT_GUI_LIB -DQT_CORE_LIB -DQT_SHARED"
    QTBIN="$QTDIR/bin"
    ;;
esac

LIBS="$LIBS $QTLIBS"
INCLUDES="$INCLUDES $QTINC"
PATH="$PATH:$QTBIN"
CXXFLAGS="$CXXFLAGS $QTINC"

# Now we check whether we can actually build a Qt app.
cat > myqt.h << EOF
#include <QObject>
class Test : public QObject
{
Q_OBJECT
public:
  Test() {}
  ~Test() {}
public slots:
  void receive() {}
signals:
  void send();
};
EOF

cat > myqt.cpp << EOF
#include "myqt.h"
#include <QApplication>
int main( int argc, char **argv )
{
  QApplication app( argc, argv );
  Test t;
  QObject::connect( &t, SIGNAL(send()), &t, SLOT(receive()) );
}
EOF

AC_MSG_CHECKING(does moc work?)
bnv_try_1="moc myqt.h -o moc_myqt.cpp"
AC_TRY_EVAL(bnv_try_1)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(moc doesn't work)
fi
AC_MSG_RESULT(yes)
AC_MSG_CHECKING(can I compile moc_myqt.cpp?)
bnv_try_2="$CXX -c $CXXFLAGS -o moc_myqt.o moc_myqt.cpp"
AC_TRY_EVAL(bnv_try_2)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(couldn't compile moc_myqt.cpp)
fi
AC_MSG_RESULT(yes)
AC_MSG_CHECKING(can I compile myqt.cpp?)
bnv_try_3="$CXX $QTINC -c $CXXFLAGS -o myqt.o myqt.cpp"
AC_TRY_EVAL(bnv_try_3)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(couldn't compile myqt.cpp)
fi
AC_MSG_RESULT(yes)
AC_MSG_CHECKING(can I link against QT?)
nv_try_4="$CXX $LIBS -o myqt myqt.o moc_myqt.o"
AC_TRY_EVAL(bnv_try_4)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(couldn't link)
fi
AC_MSG_RESULT(yes)

AC_MSG_CHECKING(for mkoctfile)
AC_TRY_EVAL(mkoctfile)
if test x"$ac_status" != x0; then
   AC_MSG_ERROR(mkoctfile is not in the path)
fi
AC_MSG_RESULT(yes)
rm -f moc_myqt.cpp myqt.h myqt.cpp myqt.o myqt moc_myqt.o
dnl ---------------------------------------------------------
else
	AC_DEFINE_UNQUOTED(USE_SDL, 0, [Define whether SDL is to be used.])
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
fi

AC_CONFIG_FILES([Makefile stellarium.spec src/Makefile src/stelutils/iniparser/Makefile src/planetsephems/Makefile src/stelutils/Makefile data/Makefile doc/Makefile textures/Makefile landscapes/Makefile landscapes/guereins/Makefile landscapes/hurricane/Makefile landscapes/moon/Makefile landscapes/ocean/Makefile landscapes/trees/Makefile textures/constellation-art/Makefile textures/constellation-art/inuit/Makefile data/scripts/Makefile data/sky_cultures/Makefile  data/sky_cultures/polynesian/Makefile data/sky_cultures/western/Makefile data/sky_cultures/chinese/Makefile data/sky_cultures/egyptian/Makefile data/sky_cultures/korean/Makefile data/sky_cultures/navajo/Makefile data/sky_cultures/lakota/Makefile data/sky_cultures/inuit/Makefile intl/Makefile po/Makefile.in m4/Makefile])

AC_OUTPUT
