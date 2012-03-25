# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# A collection of shell function definitions used by various build scripts
# in the Android NDK (Native Development Kit)
#

# Get current script name into PROGNAME
PROGNAME=`basename $0`

# Find the Android NDK root, assuming we are invoked from a script
# within its directory structure.
#
# $1: Variable name that will receive the path
# $2: Path of invoking script
find_ndk_root ()
{
    # Try to auto-detect the NDK root by walking up the directory
    # path to the current script.
    local PROGDIR="`dirname \"$2\"`"
    while [ -n "1" ] ; do
        if [ -d "$PROGDIR/build/core" ] ; then
            break
        fi
        if [ -z "$PROGDIR" -o "$PROGDIR" = '/' ] ; then
            return 1
        fi
        PROGDIR="`cd \"$PROGDIR/..\" && pwd`"
    done
    eval $1="$PROGDIR"
}

# Put location of Android NDK into ANDROID_NDK_ROOT and
# perform a tiny amount of sanity check
#
if [ -z "$ANDROID_NDK_ROOT" ] ; then
    find_ndk_root ANDROID_NDK_ROOT "$0"
    if [ $? != 0 ]; then
        echo "Please define ANDROID_NDK_ROOT to point to the root of your"
        echo "Android NDK installation."
        exit 1
    fi
fi

echo "$ANDROID_NDK_ROOT" | grep -q -e " "
if [ $? = 0 ] ; then
    echo "ERROR: The Android NDK installation path contains a space !"
    echo "Please install to a different location."
    exit 1
fi

if [ ! -d $ANDROID_NDK_ROOT ] ; then
    echo "ERROR: Your ANDROID_NDK_ROOT variable does not point to a directory."
    exit 1
fi

if [ ! -f $ANDROID_NDK_ROOT/build/core/ndk-common.sh ] ; then
    echo "ERROR: Your ANDROID_NDK_ROOT variable does not point to a valid directory."
    exit 1
fi

## Logging support
##
VERBOSE=${VERBOSE-yes}
VERBOSE2=${VERBOSE2-no}


# If NDK_LOGFILE is defined in the environment, use this as the log file
TMPLOG=
if [ -n "$NDK_LOGFILE" ] ; then
    mkdir -p `dirname "$NDK_LOGFILE"` && touch "$NDK_LOGFILE"
    TMPLOG="$NDK_LOGFILE"
fi

# Setup a log file where all log() and log2() output will be sent
#
# $1: log file path  (optional)
#
setup_default_log_file ()
{
    if [ -n "$NDK_LOGFILE" ] ; then
        return
    fi
    if [ -n "$1" ] ; then
        NDK_LOGFILE="$1"
    else
        NDK_LOGFILE=/tmp/ndk-log-$$.txt
    fi
    export NDK_LOGFILE
    TMPLOG="$NDK_LOGFILE"
    rm -rf "$TMPLOG" && mkdir -p `dirname "$TMPLOG"` && touch "$TMPLOG"
    echo "To follow build in another terminal, please use: tail -F $TMPLOG"
}

dump ()
{
    if [ -n "$TMPLOG" ] ; then
        echo "$@" >> $TMPLOG
    fi
    echo "$@"
}

log ()
{
    if [ "$VERBOSE" = "yes" ] ; then
        echo "$@"
    else
        if [ -n "$TMPLOG" ] ; then
            echo "$@" >> $TMPLOG
        fi
    fi
}

log2 ()
{
    if [ "$VERBOSE2" = "yes" ] ; then
        echo "$@"
    else
        if [ -n "$TMPLOG" ] ; then
            echo "$@" >> $TMPLOG
        fi
    fi
}

run ()
{
    if [ "$VERBOSE" = "yes" ] ; then
        echo "## COMMAND: $@"
        $@ 2>&1
    else
        if [ -n "$TMPLOG" ] ; then
            echo "## COMMAND: $@" >> $TMPLOG
            $@ >>$TMPLOG 2>&1
        else
            $@ > /dev/null 2>&1
        fi
    fi
}

panic ()
{
    dump "ERROR: $@"
    exit 1
}

fail_panic ()
{
    if [ $? != 0 ] ; then
        dump "ERROR: $@"
        exit 1
    fi
}


## Utilities
##

# Return the value of a given named variable
# $1: variable name
#
# example:
#    FOO=BAR
#    BAR=ZOO
#    echo `var_value $FOO`
#    will print 'ZOO'
#
var_value ()
{
    # find a better way to do that ?
    eval echo "$`echo $1`"
}

# convert to uppercase
# assumes tr is installed on the platform ?
#
to_uppercase ()
{
    echo $1 | tr "[:lower:]" "[:upper:]"
}

## Normalize OS and CPU
##
HOST_ARCH=`uname -m`
case "$HOST_ARCH" in
    i?86) HOST_ARCH=x86
    ;;
    amd64) HOST_ARCH=x86_64
    ;;
    powerpc) HOST_ARCH=ppc
    ;;
esac

log2 "HOST_ARCH=$HOST_ARCH"

# at this point, the supported values for CPU are:
#   x86
#   x86_64
#   ppc
#
# other values may be possible but haven't been tested
#
HOST_EXE=""
HOST_OS=`uname -s`
case "$HOST_OS" in
    Darwin)
        HOST_OS=darwin
        ;;
    Linux)
        # note that building  32-bit binaries on x86_64 is handled later
        HOST_OS=linux
        ;;
    MINGW*)
        HOST_OS=mingw
        ;;
    FreeBsd)  # note: this is not tested
        HOST_OS=freebsd
        ;;
    CYGWIN*|*_NT-*)
        HOST_OS=windows
        HOST_EXE=.exe
        if [ "x$OSTYPE" = xcygwin ] ; then
            HOST_OS=cygwin
        fi
        ;;
esac

log2 "HOST_OS=$HOST_OS"
log2 "HOST_EXE=$HOST_EXE"

# at this point, the value of HOST_OS should be one of the following:
#   linux
#   darwin
#    windows (MSys)
#    cygwin
#
# Note that cygwin is treated as a special case because it behaves very differently
# for a few things. Other values may be possible but have not been tested
#

# define HOST_TAG as a unique tag used to identify both the host OS and CPU
# supported values are:
#
#   linux-x86
#   linux-x86_64
#   darwin-x86
#   darwin-ppc
#   windows
#
# other values are possible but were not tested.
#
compute_host_tag ()
{
    case "$HOST_OS" in
        windows|cygwin)
            HOST_TAG="windows"
            ;;
        *)  HOST_TAG="${HOST_OS}-${HOST_ARCH}"
    esac
    log2 "HOST_TAG=$HOST_TAG"
}

compute_host_tag

# Compute the number of host CPU cores an HOST_NUM_CPUS
#
case "$HOST_OS" in
    linux)
        HOST_NUM_CPUS=`cat /proc/cpuinfo | grep processor | wc -l`
        ;;
    darwin|freebsd)
        HOST_NUM_CPUS=`sysctl -n hw.ncpu`
        ;;
    windows|cygwin)
        HOST_NUM_CPUS=$NUMBER_OF_PROCESSORS
        ;;
    *)  # let's play safe here
        HOST_NUM_CPUS=1
esac

log2 "HOST_NUM_CPUS=$HOST_NUM_CPUS"

# If BUILD_NUM_CPUS is not already defined in your environment,
# define it as the double of HOST_NUM_CPUS. This is used to
# run Make commands in parralles, as in 'make -j$BUILD_NUM_CPUS'
#
if [ -z "$BUILD_NUM_CPUS" ] ; then
    BUILD_NUM_CPUS=`expr $HOST_NUM_CPUS \* 2`
fi

log2 "BUILD_NUM_CPUS=$BUILD_NUM_CPUS"


##  HOST TOOLCHAIN SUPPORT
##

# force the generation of 32-bit binaries on 64-bit systems
#
FORCE_32BIT=no
force_32bit_binaries ()
{
    if [ "$HOST_ARCH" = x86_64 ] ; then
        log2 "Forcing generation of 32-bit host binaries on $HOST_ARCH"
        FORCE_32BIT=yes
        HOST_ARCH=x86
        log2 "HOST_ARCH=$HOST_ARCH"
        compute_host_tag
    fi
}

# On Windows, cygwin binaries will be generated by default, but
# you can force mingw ones that do not link to cygwin.dll if you
# call this function.
#
disable_cygwin ()
{
    if [ $OS = cygwin ] ; then
        log2 "Disabling cygwin binaries generation"
        CFLAGS="$CFLAGS -mno-cygwin"
        LDFLAGS="$LDFLAGS -mno-cygwin"
        OS=windows
        HOST_OS=windows
        compute_host_tag
    fi
}

# Various probes are going to need to run a small C program
mkdir -p /tmp/ndk-$USER/tmp/tests

TMPC=/tmp/ndk-$USER/tmp/tests/test-$$.c
TMPO=/tmp/ndk-$USER/tmp/tests/test-$$.o
TMPE=/tmp/ndk-$USER/tmp/tests/test-$$$EXE
TMPL=/tmp/ndk-$USER/tmp/tests/test-$$.log

# cleanup temporary files
clean_temp ()
{
    rm -f $TMPC $TMPO $TMPL $TMPE
}

# cleanup temp files then exit with an error
clean_exit ()
{
    clean_temp
    exit 1
}

# this function will setup the compiler and linker and check that they work as advertised
# note that you should call 'force_32bit_binaries' before this one if you want it to
# generate 32-bit binaries on 64-bit systems (that support it).
#
setup_toolchain ()
{
    if [ -z "$CC" ] ; then
        CC=gcc
    fi
    if [ -z "$CXX" ] ; then
        CXX=g++
    fi
    if [ -z "$CXXFLAGS" ] ; then
        CXXFLAGS="$CFLAGS"
    fi
    if [ -z "$LD" ] ; then
        LD="$CC"
    fi

    log2 "Using '$CC' as the C compiler"

    # check that we can compile a trivial C program with this compiler
    mkdir -p $(dirname "$TMPC")
    cat > $TMPC <<EOF
int main(void) {}
EOF

    if [ "$FORCE_32BIT" = yes ] ; then
        CC="$CC -m32"
        CXX="$CXX -m32"
        LD="$LD -m32"
        compile
        if [ $? != 0 ] ; then
            # sometimes, we need to also tell the assembler to generate 32-bit binaries
            # this is highly dependent on your GCC installation (and no, we can't set
            # this flag all the time)
            CFLAGS="$CFLAGS -Wa,--32"
            compile
        fi
    fi

    compile
    if [ $? != 0 ] ; then
        echo "your C compiler doesn't seem to work:"
        cat $TMPL
        clean_exit
    fi
    log "CC         : compiler check ok ($CC)"

    # check that we can link the trivial program into an executable
    link
    if [ $? != 0 ] ; then
        OLD_LD="$LD"
        LD="$CC"
        compile
        link
        if [ $? != 0 ] ; then
            LD="$OLD_LD"
            echo "your linker doesn't seem to work:"
            cat $TMPL
            clean_exit
        fi
    fi
    log2 "Using '$LD' as the linker"
    log "LD         : linker check ok ($LD)"

    # check the C++ compiler
    log2 "Using '$CXX' as the C++ compiler"

    cat > $TMPC <<EOF
#include <iostream>
using namespace std;
int main()
{
  cout << "Hello World!" << endl;
  return 0;
}
EOF

    compile_cpp
    if [ $? != 0 ] ; then
        echo "your C++ compiler doesn't seem to work"
        cat $TMPL
        clean_exit
    fi

    log "CXX        : C++ compiler check ok ($CXX)"

    # XXX: TODO perform AR checks
    AR=ar
    ARFLAGS=
}

# try to compile the current source file in $TMPC into an object
# stores the error log into $TMPL
#
compile ()
{
    log2 "Object     : $CC -o $TMPO -c $CFLAGS $TMPC"
    $CC -o $TMPO -c $CFLAGS $TMPC 2> $TMPL
}

compile_cpp ()
{
    log2 "Object     : $CXX -o $TMPO -c $CXXFLAGS $TMPC"
    $CXX -o $TMPO -c $CXXFLAGS $TMPC 2> $TMPL
}

# try to link the recently built file into an executable. error log in $TMPL
#
link()
{
    log2 "Link      : $LD -o $TMPE $TMPO $LDFLAGS"
    $LD -o $TMPE $TMPO $LDFLAGS 2> $TMPL
}

# run a command
#
execute()
{
    log2 "Running: $*"
    $*
}

# perform a simple compile / link / run of the source file in $TMPC
compile_exec_run()
{
    log2 "RunExec    : $CC -o $TMPE $CFLAGS $TMPC"
    compile
    if [ $? != 0 ] ; then
        echo "Failure to compile test program"
        cat $TMPC
        cat $TMPL
        clean_exit
    fi
    link
    if [ $? != 0 ] ; then
        echo "Failure to link test program"
        cat $TMPC
        echo "------"
        cat $TMPL
        clean_exit
    fi
    $TMPE
}

pattern_match ()
{
    echo "$2" | grep -q -E -e "$1"
}

# Let's check that we have a working md5sum here
check_md5sum ()
{
    A_MD5=`echo "A" | md5sum | cut -d' ' -f1`
    if [ "$A_MD5" != "bf072e9119077b4e76437a93986787ef" ] ; then
        echo "Please install md5sum on this machine"
        exit 2
    fi
}

# Find if a given shell program is available.
# We need to take care of the fact that the 'which <foo>' command
# may return either an empty string (Linux) or something like
# "no <foo> in ..." (Darwin). Also, we need to redirect stderr
# to /dev/null for Cygwin
#
# $1: variable name
# $2: program name
#
# Result: set $1 to the full path of the corresponding command
#         or to the empty/undefined string if not available
#
find_program ()
{
    local PROG
    PROG=`which $2 2>/dev/null`
    if [ -n "$PROG" ] ; then
        if pattern_match '^no ' "$PROG"; then
            PROG=
        fi
    fi
    eval $1="$PROG"
}

prepare_download ()
{
    find_program CMD_WGET wget
    find_program CMD_CURL curl
    find_program CMD_SCRP scp
}

# Download a file with either 'curl', 'wget' or 'scp'
#
# $1: source URL (e.g. http://foo.com, ssh://blah, /some/path)
# $2: target file
download_file ()
{
    # Is this HTTP, HTTPS or FTP ?
    if pattern_match "^(http|https|ftp):.*" "$1"; then
        if [ -n "$CMD_WGET" ] ; then
            run $CMD_WGET -O $2 $1
        elif [ -n "$CMD_CURL" ] ; then
            run $CMD_CURL -o $2 $1
        else
            echo "Please install wget or curl on this machine"
            exit 1
        fi
        return
    fi

    # Is this SSH ?
    # Accept both ssh://<path> or <machine>:<path>
    #
    if pattern_match "^(ssh|[^:]+):.*" "$1"; then
        if [ -n "$CMD_SCP" ] ; then
            scp_src=`echo $1 | sed -e s%ssh://%%g`
            run $CMD_SCP $scp_src $2
        else
            echo "Please install scp on this machine"
            exit 1
        fi
        return
    fi

    # Is this a file copy ?
    # Accept both file://<path> or /<path>
    #
    if pattern_match "^(file://|/).*" "$1"; then
        cp_src=`echo $1 | sed -e s%^file://%%g`
        run cp -f $cp_src $2
        return
    fi
}


# Unpack a given archive
#
# $1: archive file path
# $2: optional target directory (current one if omitted)
#
unpack_archive ()
{
    local ARCHIVE="$1"
    local DIR=${2-.}
    local RESULT TARFLAGS ZIPFLAGS
    mkdir -p "$DIR"
    if [ "$VERBOSE2" = "yes" ] ; then
        TARFLAGS="vxpf"
        ZIPFLAGS=""
    else
        TARFLAGS="xpf"
        ZIPFLAGS="q"
    fi
    case "$ARCHIVE" in
        *.zip)
            (cd $DIR && run unzip $ZIPFLAGS "$ARCHIVE")
            ;;
        *.tar)
            run tar $TARFLAGS "$ARCHIVE" -C $DIR
            ;;
        *.tar.gz)
            run tar z$TARFLAGS "$ARCHIVE" -C $DIR
            ;;
        *.tar.bz2)
            run tar j$TARFLAGS "$ARCHIVE" -C $DIR
            ;;
        *)
            panic "Cannot unpack archive with unknown extension: $ARCHIVE"
            ;;
    esac
}

# Pack a given archive
#
# $1: archive file path (including extension)
# $2: source directory for archive content
# $3+: list of files (including patterns), all if empty
pack_archive ()
{
    local ARCHIVE="$1"
    local SRCDIR="$2"
    local SRCFILES
    local TARFLAGS ZIPFLAGS
    shift; shift;
    if [ -z "$1" ] ; then
        SRCFILES="*"
    else
        SRCFILES="$@"
    fi
    if [ "`basename $ARCHIVE`" = "$ARCHIVE" ] ; then
        ARCHIVE="`pwd`/$ARCHIVE"
    fi
    mkdir -p `dirname $ARCHIVE`
    if [ "$VERBOSE2" = "yes" ] ; then
        TARFLAGS="vcf"
        ZIPFLAGS="-9r"
    else
        TARFLAGS="cf"
        ZIPFLAGS="-9qr"
    fi
    case "$ARCHIVE" in
        *.zip)
            (cd $SRCDIR && run zip $ZIPFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar)
            (cd $SRCDIR && run tar $TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar.gz)
            (cd $SRCDIR && run tar z$TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *.tar.bz2)
            (cd $SRCDIR && run tar j$TARFLAGS "$ARCHIVE" $SRCFILES)
            ;;
        *)
            panic "Unsupported archive format: $ARCHIVE"
            ;;
    esac
}

# Copy a directory, create target location if needed
#
# $1: source directory
# $2: target directory location
#
copy_directory ()
{
    local SRCDIR="$1"
    local DSTDIR="$2"
    if [ ! -d "$SRCDIR" ] ; then
        panic "Can't copy from non-directory: $SRCDIR"
    fi
    log "Copying directory: "
    log "  from $SRCDIR"
    log "  to $DSTDIR"
    mkdir -p "$DSTDIR" && (cd "$SRCDIR" && tar cf - *) | (tar xf - -C "$DSTDIR")
    fail_panic "Cannot copy to directory: $DSTDIR"
}

# This is the same than copy_directory(), but symlinks will be replaced
# by the file they actually point to instead.
copy_directory_nolinks ()
{
    local SRCDIR="$1"
    local DSTDIR="$2"
    if [ ! -d "$SRCDIR" ] ; then
        panic "Can't copy from non-directory: $SRCDIR"
    fi
    log "Copying directory (without symlinks): "
    log "  from $SRCDIR"
    log "  to $DSTDIR"
    mkdir -p "$DSTDIR" && (cd "$SRCDIR" && tar chf - *) | (tar xf - -C "$DSTDIR")
    fail_panic "Cannot copy to directory: $DSTDIR"
}

# Copy certain files from one directory to another one
# $1: source directory
# $2: target directory
# $3+: file list (including patterns)
copy_file_list ()
{
    local SRCDIR="$1"
    local DSTDIR="$2"
    shift; shift;
    if [ ! -d "$SRCDIR" ] ; then
        panic "Cant' copy from non-directory: $SRCDIR"
    fi
    log "Copying file: $@"
    log "  from $SRCDIR"
    log "  to $DSTDIR"
    mkdir -p "$DSTDIR" && (cd "$SRCDIR" && tar cf - $@) | (tar xf - -C "$DSTDIR")
    fail_panic "Cannot copy files to directory: $DSTDIR"
}

# Rotate a log file
# If the given log file exist, add a -1 to the end of the file.
# If older log files exist, rename them to -<n+1>
# $1: log file
# $2: maximum version to retain [optional]
rotate_log ()
{
    # Default Maximum versions to retain
    local MAXVER="5"
    local LOGFILE="$1"
    shift;
    if [ ! -z "$1" ] ; then
        local tmpmax="$1"
        shift;
        tmpmax=`expr $tmpmax + 0`
        if [ $tmpmax -lt 1 ] ; then
            panic "Invalid maximum log file versions '$tmpmax' invalid; defaulting to $MAXVER"
        else
            MAXVER=$tmpmax;
        fi
    fi

    # Do Nothing if the log file does not exist
    if [ ! -f "${LOGFILE}" ] ; then
        return
    fi

    # Rename existing older versions
    ver=$MAXVER
    while [ $ver -ge 1 ]
    do
        local prev=$(( $ver - 1 ))
        local old="-$prev"

        # Instead of old version 0; use the original filename
        if [ $ver -eq 1 ] ; then
            old=""
        fi

        if [ -f "${LOGFILE}${old}" ] ; then
            mv -f "${LOGFILE}${old}" "${LOGFILE}-${ver}"
        fi

        ver=$prev
    done
}
