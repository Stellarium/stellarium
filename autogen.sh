#!/bin/sh

DIE=0
srcdir=`pwd`

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed to compile $PKG_NAME."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.in >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed to compile $PKG_NAME."
    echo "Get ftp://ftp.gnu.org/pub/gnu/libtool-1.2d.tar.gz"
    echo "(or a newer version if it is available)"
    DIE=1
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed to compile $PKG_NAME."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
  NO_AUTOMAKE=yes
}

# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "Get ftp://ftp.gnu.org/pub/gnu/automake-1.3.tar.gz"
  echo "(or a newer version if it is available)"
  DIE=1
}

if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

for coin in `find $srcdir -name configure.in -print`
  do 
  dr=`dirname $coin`
  if test -f $dr/NO-AUTO-GEN; then
      echo skipping $dr -- flagged as no auto-gen
  else
      echo processing $dr
      ( cd $dr
	  if grep "^AM_PROG_LIBTOOL" configure.in >/dev/null; then
	      if test -z "$NO_LIBTOOLIZE" ; then 
		  echo "Running libtoolize..."
		  libtoolize --force --copy
	      fi
	  fi
	  
	  echo "Running aclocal ..."
	  aclocal || {
	      echo
	      echo "**Error**: aclocal failed. This may mean that you have not"
	      echo "installed all of the packages you need, or you may need to"
	      echo "set ACLOCAL_FLAGS to include \"-I \$prefix/share/aclocal\""
	      echo "for the prefix where you installed the packages whose"
	      echo "macros were not found"
	      exit 1
	  }
	  
	  if grep "^AM_CONFIG_HEADER" configure.in >/dev/null; then
	      echo "Running autoheader..."
	      autoheader || { echo "**Error**: autoheader failed."; exit 1; }
	  fi

	  echo "Running automake --gnu $am_opt ..."
	  automake --add-missing --gnu $am_opt ||
	  { echo "**Error**: automake failed."; exit 1; }
	  echo "Running autoconf ..."
	  autoconf || { echo "**Error**: autoconf failed."; exit 1; }
      ) || exit 1
  fi
done

if test x$NOCONFIGURE = x; then
    echo Running $srcdir/configure "$@" ...
    $srcdir/configure "$@" \
	&& echo Now type \`make\' to compile $PKG_NAME || exit 1
else
    echo Skipping configure process.
fi
