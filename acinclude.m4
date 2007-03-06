# LIBCURL_CHECK_CONFIG ([DEFAULT-ACTION], [MINIMUM-VERSION],
#                       [ACTION-IF-YES], [ACTION-IF-NO])
# ----------------------------------------------------------
#      David Shaw <dshaw@jabberwocky.com>   May-09-2006
#
# Checks for libcurl.  DEFAULT-ACTION is the string yes or no to
# specify whether to default to --with-libcurl or --without-libcurl.
# If not supplied, DEFAULT-ACTION is yes.  MINIMUM-VERSION is the
# minimum version of libcurl to accept.  Pass the version as a regular
# version number like 7.10.1. If not supplied, any version is
# accepted.  ACTION-IF-YES is a list of shell commands to run if
# libcurl was successfully found and passed the various tests.
# ACTION-IF-NO is a list of shell commands that are run otherwise.
# Note that using --without-libcurl does run ACTION-IF-NO.
#
# This macro #defines HAVE_LIBCURL if a working libcurl setup is
# found, and sets @LIBCURL@ and @LIBCURL_CPPFLAGS@ to the necessary
# values.  Other useful defines are LIBCURL_FEATURE_xxx where xxx are
# the various features supported by libcurl, and LIBCURL_PROTOCOL_yyy
# where yyy are the various protocols supported by libcurl.  Both xxx
# and yyy are capitalized.  See the list of AH_TEMPLATEs at the top of
# the macro for the complete list of possible defines.  Shell
# variables $libcurl_feature_xxx and $libcurl_protocol_yyy are also
# defined to 'yes' for those features and protocols that were found.
# Note that xxx and yyy keep the same capitalization as in the
# curl-config list (e.g. it's "HTTP" and not "http").
#
# Users may override the detected values by doing something like:
# LIBCURL="-lcurl" LIBCURL_CPPFLAGS="-I/usr/myinclude" ./configure
#
# For the sake of sanity, this macro assumes that any libcurl that is
# found is after version 7.7.2, the first version that included the
# curl-config script.  Note that it is very important for people
# packaging binary versions of libcurl to include this script!
# Without curl-config, we can only guess what protocols are available,
# or use curl_version_info to figure it out at runtime.

AC_DEFUN([LIBCURL_CHECK_CONFIG],
[
  AH_TEMPLATE([LIBCURL_FEATURE_SSL],[Defined if libcurl supports SSL])
  AH_TEMPLATE([LIBCURL_FEATURE_KRB4],[Defined if libcurl supports KRB4])
  AH_TEMPLATE([LIBCURL_FEATURE_IPV6],[Defined if libcurl supports IPv6])
  AH_TEMPLATE([LIBCURL_FEATURE_LIBZ],[Defined if libcurl supports libz])
  AH_TEMPLATE([LIBCURL_FEATURE_ASYNCHDNS],[Defined if libcurl supports AsynchDNS])
  AH_TEMPLATE([LIBCURL_FEATURE_IDN],[Defined if libcurl supports IDN])
  AH_TEMPLATE([LIBCURL_FEATURE_SSPI],[Defined if libcurl supports SSPI])
  AH_TEMPLATE([LIBCURL_FEATURE_NTLM],[Defined if libcurl supports NTLM])

  AH_TEMPLATE([LIBCURL_PROTOCOL_HTTP],[Defined if libcurl supports HTTP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_HTTPS],[Defined if libcurl supports HTTPS])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FTP],[Defined if libcurl supports FTP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FTPS],[Defined if libcurl supports FTPS])
  AH_TEMPLATE([LIBCURL_PROTOCOL_FILE],[Defined if libcurl supports FILE])
  AH_TEMPLATE([LIBCURL_PROTOCOL_TELNET],[Defined if libcurl supports TELNET])
  AH_TEMPLATE([LIBCURL_PROTOCOL_LDAP],[Defined if libcurl supports LDAP])
  AH_TEMPLATE([LIBCURL_PROTOCOL_DICT],[Defined if libcurl supports DICT])
  AH_TEMPLATE([LIBCURL_PROTOCOL_TFTP],[Defined if libcurl supports TFTP])

  AC_ARG_WITH(libcurl,
     AC_HELP_STRING([--with-libcurl=DIR],[look for the curl library in DIR]),
     [_libcurl_with=$withval],[_libcurl_with=ifelse([$1],,[yes],[$1])])

  if test "$_libcurl_with" != "no" ; then

     AC_PROG_AWK

     _libcurl_version_parse="eval $AWK '{split(\$NF,A,\".\"); X=256*256*A[[1]]+256*A[[2]]+A[[3]]; print X;}'"

     _libcurl_try_link=yes

     if test -d "$_libcurl_with" ; then
        LIBCURL_CPPFLAGS="-I$withval/include"
        _libcurl_ldflags="-L$withval/lib"
        AC_PATH_PROG([_libcurl_config],["$withval/bin/curl-config"])
     else
	AC_PATH_PROG([_libcurl_config],[curl-config])
     fi

     if test x$_libcurl_config != "x" ; then
        AC_CACHE_CHECK([for the version of libcurl],
	   [libcurl_cv_lib_curl_version],
           [libcurl_cv_lib_curl_version=`$_libcurl_config --version | $AWK '{print $[]2}'`])

	_libcurl_version=`echo $libcurl_cv_lib_curl_version | $_libcurl_version_parse`
	_libcurl_wanted=`echo ifelse([$2],,[0],[$2]) | $_libcurl_version_parse`

        if test $_libcurl_wanted -gt 0 ; then
	   AC_CACHE_CHECK([for libcurl >= version $2],
	      [libcurl_cv_lib_version_ok],
              [
   	      if test $_libcurl_version -ge $_libcurl_wanted ; then
	         libcurl_cv_lib_version_ok=yes
      	      else
	         libcurl_cv_lib_version_ok=no
  	      fi
              ])
        fi

	if test $_libcurl_wanted -eq 0 || test x$libcurl_cv_lib_version_ok = xyes ; then
           if test x"$LIBCURL_CPPFLAGS" = "x" ; then
              LIBCURL_CPPFLAGS=`$_libcurl_config --cflags`
           fi
           if test x"$LIBCURL" = "x" ; then
              LIBCURL=`$_libcurl_config --libs`

              # This is so silly, but Apple actually has a bug in their
	      # curl-config script.  Fixed in Tiger, but there are still
	      # lots of Panther installs around.
              case "${host}" in
                 powerpc-apple-darwin7*)
                    LIBCURL=`echo $LIBCURL | sed -e 's|-arch i386||g'`
                 ;;
              esac
           fi

	   # All curl-config scripts support --feature
	   _libcurl_features=`$_libcurl_config --feature`

           # Is it modern enough to have --protocols? (7.12.4)
	   if test $_libcurl_version -ge 461828 ; then
              _libcurl_protocols=`$_libcurl_config --protocols`
           fi
	else
           _libcurl_try_link=no
	fi

	unset _libcurl_wanted
     fi

     if test $_libcurl_try_link = yes ; then

        # we didn't find curl-config, so let's see if the user-supplied
        # link line (or failing that, "-lcurl") is enough.
        LIBCURL=${LIBCURL-"$_libcurl_ldflags -lcurl"}

        AC_CACHE_CHECK([whether libcurl is usable],
           [libcurl_cv_lib_curl_usable],
           [
           _libcurl_save_cppflags=$CPPFLAGS
           CPPFLAGS="$LIBCURL_CPPFLAGS $CPPFLAGS"
           _libcurl_save_libs=$LIBS
           LIBS="$LIBCURL $LIBS"

           AC_LINK_IFELSE(AC_LANG_PROGRAM([#include <curl/curl.h>],[
/* Try and use a few common options to force a failure if we are
   missing symbols or can't link. */
int x;
curl_easy_setopt(NULL,CURLOPT_URL,NULL);
x=CURL_ERROR_SIZE;
x=CURLOPT_WRITEFUNCTION;
x=CURLOPT_FILE;
x=CURLOPT_ERRORBUFFER;
x=CURLOPT_STDERR;
x=CURLOPT_VERBOSE;
]),libcurl_cv_lib_curl_usable=yes,libcurl_cv_lib_curl_usable=no)

           CPPFLAGS=$_libcurl_save_cppflags
           LIBS=$_libcurl_save_libs
           unset _libcurl_save_cppflags
           unset _libcurl_save_libs
           ])

        if test $libcurl_cv_lib_curl_usable = yes ; then

	   # Does curl_free() exist in this version of libcurl?
	   # If not, fake it with free()

           _libcurl_save_cppflags=$CPPFLAGS
           CPPFLAGS="$CPPFLAGS $LIBCURL_CPPFLAGS"
           _libcurl_save_libs=$LIBS
           LIBS="$LIBS $LIBCURL"

           AC_CHECK_FUNC(curl_free,,
  	      AC_DEFINE(curl_free,free,
		[Define curl_free() as free() if our version of curl lacks curl_free.]))

           CPPFLAGS=$_libcurl_save_cppflags
           LIBS=$_libcurl_save_libs
           unset _libcurl_save_cppflags
           unset _libcurl_save_libs

           AC_DEFINE(HAVE_LIBCURL,1,
             [Define to 1 if you have a functional curl library.])
           AC_SUBST(LIBCURL_CPPFLAGS)
           AC_SUBST(LIBCURL)

           for _libcurl_feature in $_libcurl_features ; do
	      AC_DEFINE_UNQUOTED(AS_TR_CPP(libcurl_feature_$_libcurl_feature),[1])
	      eval AS_TR_SH(libcurl_feature_$_libcurl_feature)=yes
           done

	   if test "x$_libcurl_protocols" = "x" ; then

	      # We don't have --protocols, so just assume that all
	      # protocols are available
	      _libcurl_protocols="HTTP FTP FILE TELNET LDAP DICT"

	      if test x$libcurl_feature_SSL = xyes ; then
	         _libcurl_protocols="$_libcurl_protocols HTTPS"

		 # FTPS wasn't standards-compliant until version
		 # 7.11.0
		 if test $_libcurl_version -ge 461568; then
		    _libcurl_protocols="$_libcurl_protocols FTPS"
		 fi
	      fi
	   fi

	   for _libcurl_protocol in $_libcurl_protocols ; do
	      AC_DEFINE_UNQUOTED(AS_TR_CPP(libcurl_protocol_$_libcurl_protocol),[1])
	      eval AS_TR_SH(libcurl_protocol_$_libcurl_protocol)=yes
           done
	else
	   unset LIBCURL
	   unset LIBCURL_CPPFLAGS
        fi
     fi

     unset _libcurl_try_link
     unset _libcurl_version_parse
     unset _libcurl_config
     unset _libcurl_feature
     unset _libcurl_features
     unset _libcurl_protocol
     unset _libcurl_protocols
     unset _libcurl_version
     unset _libcurl_ldflags
  fi

  if test x$_libcurl_with = xno || test x$libcurl_cv_lib_curl_usable != xyes ; then
     # This is the IF-NO path
     ifelse([$4],,:,[$4])
  else
     # This is the IF-YES path
     ifelse([$3],,:,[$3])
  fi

  unset _libcurl_with
])dnl

AC_DEFUN([PKG_CHECK_MODULES], [
  succeeded=no

  if test -z "$PKG_CONFIG"; then
    AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
  fi

  if test "$PKG_CONFIG" = "no" ; then
     echo "*** The pkg-config script could not be found. Make sure it is"
     echo "*** in your path, or set the PKG_CONFIG environment variable"
     echo "*** to the full path to pkg-config."
     echo "*** Or see http://www.freedesktop.org/software/pkgconfig to get 
pkg-config."
  else
     PKG_CONFIG_MIN_VERSION=0.9.0
     if $PKG_CONFIG --atleast-pkgconfig-version $PKG_CONFIG_MIN_VERSION; then
        AC_MSG_CHECKING(for $2)

        if $PKG_CONFIG --exists "$2" ; then
            AC_MSG_RESULT(yes)
            succeeded=yes

            AC_MSG_CHECKING($1_CFLAGS)
            $1_CFLAGS=`$PKG_CONFIG --cflags "$2"`
            AC_MSG_RESULT($$1_CFLAGS)

            AC_MSG_CHECKING($1_LIBS)
            $1_LIBS=`$PKG_CONFIG --libs "$2"`
            AC_MSG_RESULT($$1_LIBS)
        else
            $1_CFLAGS=""
            $1_LIBS=""
            AC_MSG_RESULT([no])
        fi

        AC_SUBST($1_CFLAGS)
        AC_SUBST($1_LIBS)
     else
        echo "*** Your version of pkg-config is too old. You need version 
$PKG_CONFIG_MIN_VERSION or newer."
        echo "*** See http://www.freedesktop.org/software/pkgconfig";
     fi
  fi

  if test $succeeded = yes; then
     ifelse([$3], , :, [$3])
  else
     ifelse([$4], , AC_MSG_ERROR([Library requirements ($2) not met; consider 
adjusting the PKG_CONFIG_PATH environment variable if your libraries are in a 
nonstandard prefix so pkg-config can find them.]), [$4])
  fi
])


/* ----------------------------------------------------------------
 Boost library macros
 
  License
Copyright © 2006 Thomas Porschberg <thomas@randspringer.de>
Copyright © 2006 Michael Tindal <mtindal@paradoxpoint.com>

Copying and distribution of this file, with or without modification, 
are permitted in any medium without royalty provided the copyright 
notice and this notice are preserved.
 
 Last Modified

2006-12-28 

See http://randspringer.de/boost/index.html for more infos
 ----------------------------------------------------------------*/

AC_DEFUN([AX_BOOST_BASE],
[
AC_ARG_WITH([boost],
        AS_HELP_STRING([--with-boost@<:@=DIR@:>@], [use boost (default is No) - it is possible to specify the root directory for boost (optional)]),
        [
    if test "$withval" = "no"; then
                want_boost="no"
    elif test "$withval" = "yes"; then
        want_boost="yes"
        ac_boost_path=""
    else
            want_boost="yes"
        ac_boost_path="$withval"
        fi
    ],
    [want_boost="yes"])

if test "x$want_boost" = "xyes"; then
        boost_lib_version_req=ifelse([$1], ,1.20.0,$1)
        boost_lib_version_req_shorten=`expr $boost_lib_version_req : '\([[0-9]]*\.[[0-9]]*\)'`
        boost_lib_version_req_major=`expr $boost_lib_version_req : '\([[0-9]]*\)'`
        boost_lib_version_req_minor=`expr $boost_lib_version_req : '[[0-9]]*\.\([[0-9]]*\)'`
        boost_lib_version_req_sub_minor=`expr $boost_lib_version_req : '[[0-9]]*\.[[0-9]]*\.\([[0-9]]*\)'`
        if test "x$boost_lib_version_req_sub_minor" = "x" ; then
                boost_lib_version_req_sub_minor="0"
        fi
        WANT_BOOST_VERSION=`expr $boost_lib_version_req_major \* 100000 \+  $boost_lib_version_req_minor \* 100 \+ $boost_lib_version_req_sub_minor`
        AC_MSG_CHECKING(for boostlib >= $boost_lib_version_req)
        succeeded=no

        dnl first we check the system location for boost libraries
        dnl this location ist chosen if boost libraries are installed with the --layout=system option
        dnl or if you install boost with RPM
        if test "$ac_boost_path" != ""; then
                BOOST_LDFLAGS="-L$ac_boost_path/lib"
                BOOST_CPPFLAGS="-I$ac_boost_path/include"
        else
                for ac_boost_path_tmp in /usr /usr/local /opt ; do
                        if test -d "$ac_boost_path_tmp/include/boost" && test -r "$ac_boost_path_tmp/include/boost"; then
                                BOOST_LDFLAGS="-L$ac_boost_path_tmp/lib"
                                BOOST_CPPFLAGS="-I$ac_boost_path_tmp/include"
                                break;
                        fi
                done
        fi

        CPPFLAGS_SAVED="$CPPFLAGS"
        CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
        export CPPFLAGS

        LDFLAGS_SAVED="$LDFLAGS"
        LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
        export LDFLAGS

        AC_LANG_PUSH(C++)
        AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
        @%:@include <boost/version.hpp>
        ]], [[
        #if BOOST_VERSION >= $WANT_BOOST_VERSION
        // Everything is okay
        #else
        #  error Boost version is too old
        #endif
        ]])],[
        AC_MSG_RESULT(yes)
        succeeded=yes
        found_system=yes
        ],[
        ])
        AC_LANG_POP([C++])



        dnl if we found no boost with system layout we search for boost libraries
        dnl built and installed without the --layout=system option or for a staged(not installed) version
        if test "x$succeeded" != "xyes"; then
                _version=0
                if test "$ac_boost_path" != ""; then
                        BOOST_LDFLAGS="-L$ac_boost_path/lib"
                        if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                        _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                        V_CHECK=`expr $_version_tmp \> $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                _version=$_version_tmp
                                        fi
                                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                                        BOOST_CPPFLAGS="-I$ac_boost_path/include/boost-$VERSION_UNDERSCORE"
                                done
                        fi
                else
                        for ac_boost_path in /usr /usr/local /opt ; do
                                if test -d "$ac_boost_path" && test -r "$ac_boost_path"; then
                                        for i in `ls -d $ac_boost_path/include/boost-* 2>/dev/null`; do
                                                _version_tmp=`echo $i | sed "s#$ac_boost_path##" | sed 's/\/include\/boost-//' | sed 's/_/./'`
                                                V_CHECK=`expr $_version_tmp \> $_version`
                                                if test "$V_CHECK" = "1" ; then
                                                        _version=$_version_tmp
                                                        best_path=$ac_boost_path
                                                fi
                                        done
                                fi
                        done

                        VERSION_UNDERSCORE=`echo $_version | sed 's/\./_/'`
                        BOOST_CPPFLAGS="-I$best_path/include/boost-$VERSION_UNDERSCORE"
                        BOOST_LDFLAGS="-L$best_path/lib"

                        if test "x$BOOST_ROOT" != "x"; then
                                if test -d "$BOOST_ROOT" && test -r "$BOOST_ROOT" && test -d "$BOOST_ROOT/stage/lib" && test -r "$BOOST_ROOT/stage/lib"; then
                                        version_dir=`expr //$BOOST_ROOT : '.*/\(.*\)'`
                                        stage_version=`echo $version_dir | sed 's/boost_//' | sed 's/_/./g'`
                                        stage_version_shorten=`expr $stage_version : '\([[0-9]]*\.[[0-9]]*\)'`
                                        V_CHECK=`expr $stage_version_shorten \>\= $_version`
                                        if test "$V_CHECK" = "1" ; then
                                                AC_MSG_NOTICE(We will use a staged boost library from $BOOST_ROOT)
                                                BOOST_CPPFLAGS="-I$BOOST_ROOT"
                                                BOOST_LDFLAGS="-L$BOOST_ROOT/stage/lib"
                                        fi
                                fi
                        fi
                fi

                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

                AC_LANG_PUSH(C++)
                AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[
                @%:@include <boost/version.hpp>
                ]], [[
                #if BOOST_VERSION >= $WANT_BOOST_VERSION
                // Everything is okay
                #else
                #  error Boost version is too old
                #endif
                ]])],[
                AC_MSG_RESULT(yes)
                succeeded=yes
                found_system=yes
                ],[
                ])
                AC_LANG_POP([C++])
        fi

        if test "$succeeded" != "yes" ; then
                if test "$_version" = "0" ; then
                        AC_MSG_ERROR([[We could not detect the boost libraries (version $boost_lib_version_req_shorten or higher). If you have a staged boost library (still not installed) please specify \$BOOST_ROOT in your environment and do not give a PATH to --with-boost option.  If you are sure you have boost installed, then check your version number looking in <boost/version.hpp>. See http://randspringer.de/boost for more documentation.]])
                else
                        AC_MSG_NOTICE([Your boost libraries seems to old (version $_version).])
                fi
        else
                AC_SUBST(BOOST_CPPFLAGS)
                AC_SUBST(BOOST_LDFLAGS)
                AC_DEFINE(HAVE_BOOST,,[define if the Boost library is available])
        fi

        CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
fi

])




AC_DEFUN([AX_BOOST_THREAD],
[
        AC_ARG_WITH([boost-thread],
        AS_HELP_STRING([--with-boost-thread@<:@=special-lib@:>@],
                   [use the Thread library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-thread=boost_thread-gcc-mt ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_thread_lib=""
        else
                    want_boost="yes"
                ax_boost_user_thread_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
        AC_REQUIRE([AC_CANONICAL_BUILD])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Thread library is available,
                                           ax_cv_boost_thread,
        [AC_LANG_PUSH([C++])
                         CXXFLAGS_SAVE=$CXXFLAGS

                         if test "x$build_os" = "xsolaris" ; then
                                 CXXFLAGS="-pthreads $CXXFLAGS"
                         elif test "x$build_os" = "xming32" ; then
                                 CXXFLAGS="-mthreads $CXXFLAGS"
                         else
                                CXXFLAGS="-pthread $CXXFLAGS"
                         fi
                         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/thread/thread.hpp>]],
                                   [[boost::thread_group thrds;
                                   return 0;]]),
                   ax_cv_boost_thread=yes, ax_cv_boost_thread=no)
                         CXXFLAGS=$CXXFLAGS_SAVE
             AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_thread" = "xyes"; then
           if test "x$build_os" = "xsolaris" ; then
                          BOOST_CPPFLAGS="-pthreads $BOOST_CPPFLAGS"
                   elif test "x$build_os" = "xming32" ; then
                          BOOST_CPPFLAGS="-mthreads $BOOST_CPPFLAGS"
                   else
                          BOOST_CPPFLAGS="-pthread $BOOST_CPPFLAGS"
                   fi

                        AC_SUBST(BOOST_CPPFLAGS)

                        AC_DEFINE(HAVE_BOOST_THREAD,,[define if the Boost::Thread library is available])
                        BN=boost_thread

                        LDFLAGS_SAVE=$LDFLAGS
                        case "x$build_os" in
                          *bsd* )
                               LDFLAGS="-pthread $LDFLAGS"
                          break;
                          ;;
                        esac
            if test "x$ax_boost_user_thread_lib" = "x"; then
                                for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main, [BOOST_THREAD_LIB="-l$ax_lib" AC_SUBST(BOOST_THREAD_LIB) link_thread="yes" break],
                                 [link_thread="no"])
                                done
            else
               for ax_lib in $ax_boost_user_thread_lib $BN-$ax_boost_user_thread_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_THREAD_LIB="-l$ax_lib" AC_SUBST(BOOST_THREAD_LIB) link_thread="yes" break],
                                   [link_thread="no"])
                  done

            fi
                        if test "x$link_thread" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        else
                           case "x$build_os" in
                              *bsd* )
                                BOOST_LDFLAGS="-pthread $BOOST_LDFLAGS"
                              break;
                              ;;
                           esac

                        fi
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])


AC_DEFUN([AX_BOOST_FILESYSTEM],
[
        AC_ARG_WITH([boost-filesystem],
        AS_HELP_STRING([--with-boost-filesystem@<:@=special-lib@:>@],
                   [use the Filesystem library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-filesystem=boost_filesystem-gcc-mt ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_filesystem_lib=""
        else
                    want_boost="yes"
                ax_boost_user_filesystem_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Filesystem library is available,
                                           ax_cv_boost_filesystem,
        [AC_LANG_PUSH([C++])
         AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/filesystem/path.hpp>]],
                                   [[using namespace boost::filesystem;
                                   path my_path( "foo/bar/data.txt" );
                                   return 0;]]),
                                               ax_cv_boost_filesystem=yes, ax_cv_boost_filesystem=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_filesystem" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_FILESYSTEM,,[define if the Boost::Filesystem library is available])
                        BN=boost_filesystem
            if test "x$ax_boost_user_filesystem_lib" = "x"; then
                        for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                              lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                              $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                                    AC_CHECK_LIB($ax_lib, main,
                                 [BOOST_FILESYSTEM_LIB="-l$ax_lib" AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes" break],
                                 [link_filesystem="no"])
                                done
            else
               for ax_lib in $ax_boost_user_filesystem_lib $BN-$ax_boost_user_filesystem_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_FILESYSTEM_LIB="-l$ax_lib" AC_SUBST(BOOST_FILESYSTEM_LIB) link_filesystem="yes" break],
                                   [link_filesystem="no"])
                  done

            fi
                        if test "x$link_filesystem" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])

AC_DEFUN([AX_BOOST_DATE_TIME],
[
        AC_ARG_WITH([boost-date-time],
        AS_HELP_STRING([--with-boost-date-time@<:@=special-lib@:>@],
                   [use the Date_Time library from boost - it is possible to specify a certain library for the linker
                        e.g. --with-boost-date-time=boost_date_time-gcc-mt-d-1_33_1 ]),
        [
        if test "$withval" = "no"; then
                        want_boost="no"
        elif test "$withval" = "yes"; then
            want_boost="yes"
            ax_boost_user_date_time_lib=""
        else
                    want_boost="yes"
                ax_boost_user_date_time_lib="$withval"
                fi
        ],
        [want_boost="yes"]
        )

        if test "x$want_boost" = "xyes"; then
        AC_REQUIRE([AC_PROG_CC])
                CPPFLAGS_SAVED="$CPPFLAGS"
                CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"
                export CPPFLAGS

                LDFLAGS_SAVED="$LDFLAGS"
                LDFLAGS="$LDFLAGS $BOOST_LDFLAGS"
                export LDFLAGS

        AC_CACHE_CHECK(whether the Boost::Date_Time library is available,
                                           ax_cv_boost_date_time,
        [AC_LANG_PUSH([C++])
                 AC_COMPILE_IFELSE(AC_LANG_PROGRAM([[@%:@include <boost/date_time/gregorian/gregorian_types.hpp>]],
                                   [[using namespace boost::gregorian; date d(2002,Jan,10);
                                     return 0;
                                   ]]),
         ax_cv_boost_date_time=yes, ax_cv_boost_date_time=no)
         AC_LANG_POP([C++])
                ])
                if test "x$ax_cv_boost_date_time" = "xyes"; then
                        AC_DEFINE(HAVE_BOOST_DATE_TIME,,[define if the Boost::Date_Time library is available])
                        BN=boost_date_time
            if test "x$ax_boost_user_date_time_lib" = "x"; then
                           for ax_lib in $BN $BN-$CC $BN-$CC-mt $BN-$CC-mt-s $BN-$CC-s \
                               lib$BN lib$BN-$CC lib$BN-$CC-mt lib$BN-$CC-mt-s lib$BN-$CC-s \
                               $BN-mgw $BN-mgw $BN-mgw-mt $BN-mgw-mt-s $BN-mgw-s ; do
                              AC_CHECK_LIB($ax_lib, main, [BOOST_DATE_TIME_LIB="-l$ax_lib" AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes" break],
                               [link_date_time="no"])
                           done
            else
               for ax_lib in $ax_boost_user_date_time_lib $BN-$ax_boost_user_date_time_lib; do
                                      AC_CHECK_LIB($ax_lib, main,
                                   [BOOST_DATE_TIME_LIB="-l$ax_lib" AC_SUBST(BOOST_DATE_TIME_LIB) link_date_time="yes" break],
                                   [link_date_time="no"])
                  done

            fi
                        if test "x$link_date_time" = "xno"; then
                                AC_MSG_ERROR(Could not link against $ax_lib !)
                        fi
                fi

                CPPFLAGS="$CPPFLAGS_SAVED"
        LDFLAGS="$LDFLAGS_SAVED"
        fi
])


# Configure paths for SDL
# Sam Lantinga 9/21/99
# stolen from Manish Singh
# stolen back from Frank Belew
# stolen from Manish Singh
# Shamelessly stolen from Owen Taylor

dnl AM_PATH_SDL([MINIMUM-VERSION, [ACTION-IF-FOUND [, ACTION-IF-NOT-FOUND]]])
dnl Test for SDL, and define SDL_CFLAGS and SDL_LIBS
dnl
AC_DEFUN([AM_PATH_SDL],
[dnl 
dnl Get the cflags and libraries from the sdl-config script
dnl
AC_ARG_WITH(sdl-prefix,[  --with-sdl-prefix=PFX   Prefix where SDL is installed (optional)],
            sdl_prefix="$withval", sdl_prefix="")
AC_ARG_WITH(sdl-exec-prefix,[  --with-sdl-exec-prefix=PFX Exec prefix where SDL is installed (optional)],
            sdl_exec_prefix="$withval", sdl_exec_prefix="")
AC_ARG_ENABLE(sdltest, [  --disable-sdltest       Do not try to compile and run a test SDL program],
		    , enable_sdltest=yes)

  if test x$sdl_exec_prefix != x ; then
     sdl_args="$sdl_args --exec-prefix=$sdl_exec_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_exec_prefix/bin/sdl-config
     fi
  fi
  if test x$sdl_prefix != x ; then
     sdl_args="$sdl_args --prefix=$sdl_prefix"
     if test x${SDL_CONFIG+set} != xset ; then
        SDL_CONFIG=$sdl_prefix/bin/sdl-config
     fi
  fi

dnl  AC_REQUIRE([AC_CANONICAL_TARGET])
  PATH="$prefix/bin:$prefix/usr/bin:$PATH"
  AC_PATH_PROG(SDL_CONFIG, sdl-config, no, [$PATH])
  min_sdl_version=ifelse([$1], ,0.11.0,$1)
  AC_MSG_CHECKING(for SDL - version >= $min_sdl_version)
  no_sdl=""
  if test "$SDL_CONFIG" = "no" ; then
    no_sdl=yes
  else
    SDL_CFLAGS=`$SDL_CONFIG $sdlconf_args --cflags`
    SDL_LIBS=`$SDL_CONFIG $sdlconf_args --libs`

    sdl_major_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\1/'`
    sdl_minor_version=`$SDL_CONFIG $sdl_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\2/'`
    sdl_micro_version=`$SDL_CONFIG $sdl_config_args --version | \
           sed 's/\([[0-9]]*\).\([[0-9]]*\).\([[0-9]]*\)/\3/'`
    if test "x$enable_sdltest" = "xyes" ; then
      ac_save_CFLAGS="$CFLAGS"
      ac_save_LIBS="$LIBS"
      CFLAGS="$CFLAGS $SDL_CFLAGS"
      LIBS="$LIBS $SDL_LIBS"
dnl
dnl Now check if the installed SDL is sufficiently new. (Also sanity
dnl checks the results of sdl-config to some extent
dnl
      rm -f conf.sdltest
      AC_TRY_RUN([
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SDL.h"

char*
my_strdup (char *str)
{
  char *new_str;
  
  if (str)
    {
      new_str = (char *)malloc ((strlen (str) + 1) * sizeof(char));
      strcpy (new_str, str);
    }
  else
    new_str = NULL;
  
  return new_str;
}

int main (int argc, char *argv[])
{
  int major, minor, micro;
  char *tmp_version;

  /* This hangs on some systems (?)
  system ("touch conf.sdltest");
  */
  { FILE *fp = fopen("conf.sdltest", "a"); if ( fp ) fclose(fp); }

  /* HP/UX 9 (%@#!) writes to sscanf strings */
  tmp_version = my_strdup("$min_sdl_version");
  if (sscanf(tmp_version, "%d.%d.%d", &major, &minor, &micro) != 3) {
     printf("%s, bad version string\n", "$min_sdl_version");
     exit(1);
   }

   if (($sdl_major_version > major) ||
      (($sdl_major_version == major) && ($sdl_minor_version > minor)) ||
      (($sdl_major_version == major) && ($sdl_minor_version == minor) && ($sdl_micro_version >= micro)))
    {
      return 0;
    }
  else
    {
      printf("\n*** 'sdl-config --version' returned %d.%d.%d, but the minimum version\n", $sdl_major_version, $sdl_minor_version, $sdl_micro_version);
      printf("*** of SDL required is %d.%d.%d. If sdl-config is correct, then it is\n", major, minor, micro);
      printf("*** best to upgrade to the required version.\n");
      printf("*** If sdl-config was wrong, set the environment variable SDL_CONFIG\n");
      printf("*** to point to the correct copy of sdl-config, and remove the file\n");
      printf("*** config.cache before re-running configure\n");
      return 1;
    }
}

],, no_sdl=yes,[echo $ac_n "cross compiling; assumed OK... $ac_c"])
       CFLAGS="$ac_save_CFLAGS"
       LIBS="$ac_save_LIBS"
     fi
  fi
  if test "x$no_sdl" = x ; then
     AC_MSG_RESULT(yes)
     ifelse([$2], , :, [$2])     
  else
     AC_MSG_RESULT(no)
     if test "$SDL_CONFIG" = "no" ; then
       echo "*** The sdl-config script installed by SDL could not be found"
       echo "*** If SDL was installed in PREFIX, make sure PREFIX/bin is in"
       echo "*** your path, or set the SDL_CONFIG environment variable to the"
       echo "*** full path to sdl-config."
     else
       if test -f conf.sdltest ; then
        :
       else
          echo "*** Could not run SDL test program, checking why..."
          CFLAGS="$CFLAGS $SDL_CFLAGS"
          LIBS="$LIBS $SDL_LIBS"
          AC_TRY_LINK([
#include <stdio.h>
#include "SDL.h"

int main(int argc, char *argv[])
{ return 0; }
#undef  main
#define main K_and_R_C_main
],      [ return 0; ],
        [ echo "*** The test program compiled, but did not run. This usually means"
          echo "*** that the run-time linker is not finding SDL or finding the wrong"
          echo "*** version of SDL. If it is not finding SDL, you'll need to set your"
          echo "*** LD_LIBRARY_PATH environment variable, or edit /etc/ld.so.conf to point"
          echo "*** to the installed location  Also, make sure you have run ldconfig if that"
          echo "*** is required on your system"
	  echo "***"
          echo "*** If you have an old version installed, it is best to remove it, although"
          echo "*** you may also be able to get things to work by modifying LD_LIBRARY_PATH"],
        [ echo "*** The test program failed to compile or link. See the file config.log for the"
          echo "*** exact error that occured. This usually means SDL was incorrectly installed"
          echo "*** or that you have moved SDL since it was installed. In the latter case, you"
          echo "*** may want to edit the sdl-config script: $SDL_CONFIG" ])
          CFLAGS="$ac_save_CFLAGS"
          LIBS="$ac_save_LIBS"
       fi
     fi
     SDL_CFLAGS=""
     SDL_LIBS=""
     ifelse([$3], , :, [$3])
  fi
  AC_SUBST(SDL_CFLAGS)
  AC_SUBST(SDL_LIBS)
  rm -f conf.sdltest
])



# =========================================================================
# AM_PATH_STLPORT : STLPort checking macros

AC_DEFUN([AM_PATH_STLPORT],
[ AC_REQUIRE_CPP()

AC_ARG_WITH( stlport,
    [  --with-stlport=<path>   path to the STLPort install files directory.
                          e.g. /usr/local/stlport])

AC_ARG_WITH( stlport-include,
    [  --with-stlport-include=<path>
                          path to the STLPort header files directory.
                          e.g. /usr/local/stlport/stlport])

AC_ARG_WITH( stlport-lib,
    [  --with-stlport-lib=<path>
                          path to the STLPort library files directory.
                          e.g. /usr/local/stlport/lib])

if test "$with_debug" = "full"
then
 stlport_lib="stlport_gcc_debug"
else
 stlport_lib="stlport_gcc"
fi

if test "$with_debug" = "full"
then
 stlport_lib2="stlport_gcc_debug"
else
 stlport_lib2="stlport_gcc"
fi

if test "$with_stlport" = no
then
    # The user explicitly disabled the use of the STLPorts
    AC_MSG_ERROR([STLPort is mandatory: do not specify --without-stlport])
else
    stlport_includes="/usr/include/stlport"
    if test "$with_stlport" -a "$with_stlport" != yes
    then
        stlport_includes="$with_stlport/stlport"
        stlport_libraries="$with_stlport/lib"

        if test ! -d "$stlport_includes"
        then
            stlport_includes="$with_stlport/include/stlport"
        fi
    fi
fi

if test "$with_stlport_include"
then
    stlport_includes="$with_stlport_include"
fi

if test "$with_stlport_lib"
then
    stlport_libraries="$with_stlport_lib"
fi

# Check for the 'pthread' library. SLTPort needs it.
AC_CHECK_LIB(pthread, main, , [AC_MSG_ERROR([cannot find the pthread library.])])
AC_CHECK_LIB(dl, dlopen, , [AC_MSG_ERROR([cannot find the dl library.])])

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

# Put STLPorts includes in CXXFLAGS
if test "$stlport_includes"
then
    CXXFLAGS="$CXXFLAGS -I$stlport_includes"
fi

# Put STLPorts libraries directory in LIBS
if test "$stlport_libraries"
then
    LIBS="-L$stlport_libraries $LIBS"
else
    stlport_libraries='default'
fi

# Put STLPort GCC libraries directory in LIBS
if test "$stlport_libraries2"
then
    LIBS="-L$stlport_libraries2 $LIBS"
else
    stlport_libraries2='default'
fi

# Test the headers

AC_CHECK_HEADER(algorithm,
    have_stlport_headers="yes",
    have_stlport_headers="no" )

AC_MSG_CHECKING(for STLPort headers)

if test "$have_stlport_headers" = "yes"
then
    AC_MSG_RESULT([$stlport_includes])
else
    AC_MSG_RESULT(no)
fi

AC_CHECK_LIB($stlport_lib, main,, have_stlport_libraries="no")

AC_MSG_CHECKING(for STLPort library)

if test "$have_stlport_libraries" != "no"
then
    AC_MSG_RESULT([$stlport_libraries])
else
    AC_MSG_RESULT(no)
fi

AC_CHECK_LIB($stlport_lib2, main,, have_stlport_libraries="no")

AC_MSG_CHECKING(for STLPort GCC library)

if test "$have_stlport_libraries2" != "no"
then
    AC_MSG_RESULT([$stlport_libraries2])
else
    AC_MSG_RESULT(no)
fi

if test "$have_stlport_headers" = "yes" &&
    (test "$have_stlport_libraries" != "no" || test "$have_stlport_libraries2" != "no")
then
    have_stlport="yes"
else
    have_stlport="no"
fi

if test "$have_stlport" = "no"
then
    AC_MSG_ERROR([STLPort must be installed (http://www.stlport.org).])
fi

AC_LANG_RESTORE

])