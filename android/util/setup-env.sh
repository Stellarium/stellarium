#!/bin/bash

#   ***************************************************************************
#     setup-env.sh - prepares the build environnement for android QGIS
#      --------------------------------------
#
#      based on Marco Bernasocchi's android QGIS scripts
#
#   ***************************************************************************
#   *                                                                         *
#   *   This program is free software; you can redistribute it and/or modify  *
#   *   it under the terms of the GNU General Public License as published by  *
#   *   the Free Software Foundation; either version 2 of the License, or     *
#   *   (at your option) any later version.                                   *
#   *                                                                         *
#   ***************************************************************************/

set -e

#######Load config#######
source `dirname $0`/config.conf

########START SCRIPT########
#do these switches work?
usage(){
 echo "Usage:"
 echo " setup-env.sh 
        --help (-h)
        --version (-v)
        --echo <text> (-e)      this option does noting"
}

echo "SETTING UP ANDROID STELLARIUM ENVIRONEMENT"
echo "STELLARIUM build dir (WILL BE EMPTIED): " $STEL_BUILD_DIR
echo "NDK dir:                          " $ANDROID_NDK_ROOT
echo "Standalone toolchain dir:         " $ANDROID_NDK_TOOLCHAIN_ROOT
echo "Downloading src to:               " $SRC_DIR
echo "Installing to:                    " $INSTALL_DIR
if [ "$ANDROID_TARGET_ARCH" = "armeabi-v7a" ]; then
  echo "WARNING: armeabi-v7a builds usually don't work on android emulators"
else
  echo "NOTICE: if you build for a newer device (hummingbird, tegra,... processors)\
 armeabi-v7a arch would increase the performance. Set the architecture accordingly\
 in the conf file. Look as well for MY_FPU in the conf file for further tuning."
fi  
echo "PATH:"
echo $PATH
echo "CFLAGS:                           " $MY_STD_CFLAGS
echo "CXXFLAGS:                         " $MY_STD_CXXFLAGS
echo "LDFLAGS:                          " $MY_STD_LDFLAGS
echo "You can configure all this and more in `dirname $0`/config.conf"

while test "$1" != "" ; do
        case $1 in
                --echo|-e)
                        echo "$2"
                        shift
                ;;
                --help|-h)
                        usage
                        exit 0
                ;;
                --version|-v)
                        echo "setup.sh version 0.1"
                        exit 0
                ;;
                -*)
                        echo "Error: no such option $1"
                        usage
                        exit 1
                ;;
        esac
        shift
done

#confirm settings
CONTINUE="n"
echo "OK? [y, n*]:"
read CONTINUE
CONTINUE=$(echo $CONTINUE | tr "[:upper:]" "[:lower:]")

if [ "$CONTINUE" != "y" ]; then
  echo "User Abort"
  exit 1
else
  
  #######QTUITOOLS#######
  #HACK temporary needed until necessitas will include qtuitools
  #check if qt-src are installed
#  if [ -d $QT_SRC/tools/designer/src/lib/uilib ]; then
#    ln -sfn $QT_SRC/tools/designer/src/lib/uilib $QT_INCLUDE/QtDesigner
#    ln -sfn $QT_SRC/tools/designer/src/uitools $QT_INCLUDE/QtUiTools
#    cp -rf $PATCH_DIR/qtuitools/QtDesigner/* $QT_INCLUDE/QtDesigner/
#    cp -rf $PATCH_DIR/qtuitools/QtUiTools/* $QT_INCLUDE/QtUiTools/
#  else
#    echo "Please download the QT source using the package manager in Necessitas \
#    Creator under help/start updater and rerun this script"
#    exit 1
#  fi

  ########CHECK IF ant EXISTS################
  hash ant 2>&- || { echo >&2 "ant required to create APK. Aborting."; exit 1; }
  
  ########CHECK IF cmake EXISTS################
  hash cmake 2>&- || { echo >&2 "cmake required. Aborting."; exit 1; }

  #preparing environnement
  
  if [ "$WINDOWS" == "true" ]; then
    start android.bat update project --target android-11 --name Stellarium --path $APK_DIR
  else
    start android update project --target android-11 --name Stellarium --path $APK_DIR
  fi
  mkdir -p $STEL_BUILD_DIR
  rm -rf $STEL_BUILD_DIR/*
  cd $ROOT_DIR
    
  
  ########CREATE STANDALONE TOOLCHAIN########
  #echo "CREATING STANDALONE TOOLCHAIN"
  #$ANDROID_NDK_ROOT/build/tools/make-standalone-toolchain.sh --platform=$ANDROID_NDK_PLATFORM --install-dir=$ANDROID_NDK_TOOLCHAIN_ROOT


  #need updated config.sub/config.guess if building libraries. We're not building libraries right now.
  
  #Get Updated config.sub
  #wget -c "http://git.savannah.gnu.org/cgit/config.git/plain/config.sub" -O $TMP_DIR/config.sub
  #Get Updated guess.sub
  #wget -c "http://git.savannah.gnu.org/cgit/config.git/plain/config.guess" -O $TMP_DIR/config.guess
  
  exit 0
fi
