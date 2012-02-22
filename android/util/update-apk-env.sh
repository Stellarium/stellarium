#!/bin/bash

#   ***************************************************************************
#     update-apk-env.sh - copies files into the apk directory
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

source `dirname $0`/config.conf

APK_LIBS_DIR=$APK_DIR/libs/$ANDROID_TARGET_ARCH

#copy libs to apk
mkdir -p $APK_DIR/libs/
rm -vrf $APK_DIR/libs/*
cp -vrf $INSTALL_DIR/bin $APK_LIBS_DIR

#copy assets to apk 
rm -vrf $APK_DIR/assets
cp -vrf $INSTALL_DIR/share/stellarium $APK_DIR/assets
cp -vrf $INSTALL_DIR/share/locale $APK_DIR/assets/locale