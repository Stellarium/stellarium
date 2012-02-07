#!/bin/bash

#   ***************************************************************************
#     build-all.sh - builds android QGIS
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
sudo echo ""
$SCRIPT_DIR/build-apk.sh
$SCRIPT_DIR/install-apk.sh
$SCRIPT_DIR/run-apk.sh

