#!/bin/bash

#   ***************************************************************************
#     build-apk.sh - builds the and installs the needed libraries for android QGIS
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
ADB=$ANDROID_SDK_ROOT/platform-tools/adb
$ADB kill-server

if [ "$ADB_WIRELESS" != "false" ]; then
  $ADB connect $ADB_WIRELESS
fi

if [ "$WINDOWS" == "true" ]; then
  $ADB devices
else
 sudo $ADB devices
fi

echo "Uninstalling org.stellarium.stellarium"
$ADB uninstall org.stellarium.stellarium
echo "Installing $APK_DIR/bin/stellarium-debug.apk"
$ADB install $APK_DIR/bin/stellarium-debug.apk
