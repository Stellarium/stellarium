#!/bin/bash

#   ***************************************************************************
#     build-qgis.sh - builds android QGIS
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

#pass -configure as first parameter to perform cmake (configure step)

set -e

source `dirname $0`/config.conf

mkdir -p $STEL_BUILD_DIR
cd $STEL_BUILD_DIR

#
#-DEXECUTABLE_OUTPUT_PATH=$INSTALL_DIR/bin \
#-DLIBRARY_OUTPUT_DIRECTORY=$INSTALL_DIR/lib \
#-DRUNTIME_OUTPUT_DIRECTORY=$INSTALL_DIR/bin \
#

MY_CMAKE_FLAGS=" \
-DCMAKE_BUILD_TYPE=Debug \
-DARM_TARGET=$ANDROID_TARGET_ARCH \
-DCFLAGS='$MY_STD_CFLAGS' \
-DCMAKE_BUILD_TYPE=$BUILD_TYPE \
-DCMAKE_VERBOSE_MAKEFILE=ON \
-DCMAKE_INSTALL_PREFIX=$INSTALL_DIR \
-DCMAKE_TOOLCHAIN_FILE=$SCRIPT_DIR/android.toolchain.cmake \
-DINCLUDE_DIRECTORIES=$INSTALL_DIR \
-DLDFLAGS=$MY_STD_LDFLAGS \
-DQT_MKSPECS_DIR=$QT_ROOT/mkspecs \
-DQT_QMAKE_EXECUTABLE=$QMAKE \
-DBUILD_FOR_ANDROID:boolean=true \
-DOPENGL_MODE=ES2 \
-DBUILD_STATIC_PLUGINS=false \
-DENABLE_NLS=false"

#CMake isn't finding necessitas' Qt libraries automatically. This *may* just be a Windows-thing
if [ "$WINDOWS" == "true" ]; then
  MY_CMAKE_FLAGS="$MY_CMAKE_FLAGS \
-DQT_QTCORE_LIBRARY_RELEASE=$QT_ROOT/lib/libQtCore.so \
-DQT_QTCORE_INCLUDE_DIR=$QT_ROOT/include/QtCore \
-DQT_QTGUI_INCLUDE_DIR=$QT_ROOT/include/QtGui \
-DQT_QTNETWORK_INCLUDE_DIR=$QT_ROOT/include/QtNetwork \
-DQT_QTOPENGL_INCLUDE_DIR=$QT_ROOT/include/QtOpengl \
-DQT_QTSCRIPT_INCLUDE_DIR=$QT_ROOT/include/QtScript \
-DQT_QTTEST_INCLUDE_DIR=$QT_ROOT/include/QtTest \
-DQT_INCLUDE_DIR=$QT_INCLUDE \
-DCMAKE_PREFIX_PATH=$QT_ROOT "
fi

#uncomment the next 2 lines to only get the needed cmake flags echoed

#echo $MY_CMAKE_FLAGS
#exit 0

cmake $MY_CMAKE_FLAGS -G"Unix Makefiles" ../.. && make install


#GIT_REV=$(git rev-parse HEAD)
#update version file in share
#mkdir -p $INSTALL_DIR/files
#echo $GIT_REV > $INSTALL_DIR/files/version.txt
#update apk manifest
#sed -i "s|<meta-data android:name=\"android.app.git_rev\" android:value=\".*\"/>|<meta-data android:name=\"android.app.git_rev\" android:value=\"$GIT_REV\"/>|" $APK_DIR/AndroidManifest.xml
