#!/bin/bash

#
# (C) Stellarium Team
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 

CMAKE_INSTALL_PREFIX=$1
shift;
PROJECT_SOURCE_DIR=$1
shift;

QTPLUGIN_DIR=""
for d in /Developer/Applications/Qt/plugins /opt/local/share/qt4/plugins; do
    if [ -e "$d" ]; then
        QTPLUGIN_DIR="$d"
        break
    fi
done

mv ${CMAKE_INSTALL_PREFIX}/bin ${CMAKE_INSTALL_PREFIX}/MacOS

if [ -e "${CMAKE_INSTALL_PREFIX}/MacOS/stellarium.app/Contents/MacOS/stellarium" ] ; then
    mv ${CMAKE_INSTALL_PREFIX}/MacOS/stellarium.app/Contents/MacOS/stellarium ${CMAKE_INSTALL_PREFIX}/MacOS
    /bin/rm -rf ${CMAKE_INSTALL_PREFIX}/MacOS/stellarium.app
fi

mv ${CMAKE_INSTALL_PREFIX}/share ${CMAKE_INSTALL_PREFIX}/Resources
mv ${CMAKE_INSTALL_PREFIX}/Resources/stellarium/* ${CMAKE_INSTALL_PREFIX}/Resources
rmdir ${CMAKE_INSTALL_PREFIX}/Resources/stellarium


mkdir ${CMAKE_INSTALL_PREFIX}/Frameworks
/usr/bin/perl util/pkgApp.pl ${CMAKE_INSTALL_PREFIX} MacOS/stellarium Frameworks 
mkdir ${CMAKE_INSTALL_PREFIX}/plugins

cp -pr "$QTPLUGIN_DIR/"{imageformats,iconengines}/* ${CMAKE_INSTALL_PREFIX}/plugins
for f in ${CMAKE_INSTALL_PREFIX}/plugins/*.dylib; do
    fdir=`dirname $f`
    dir=`basename $fdir`
    base=`basename $f`
    #/usr/bin/install_name_tool -id "@executable_path/$dir/$base" $f
    for qt in `otool -L $f | egrep '	Qt' | cut -f 1 -d ' '`; do
	newt="@executable_path/../Frameworks/$qt"
	/usr/bin/install_name_tool -change $qt $newt $f
    done
done

cp -pr $PROJECT_SOURCE_DIR/data/Icon.icns $CMAKE_INSTALL_PREFIX/Resources
cp -pr $PROJECT_SOURCE_DIR/data/{PkgInfo,Info.plist} $CMAKE_INSTALL_PREFIX
cp -pr $PROJECT_SOURCE_DIR/util/qt.conf $CMAKE_INSTALL_PREFIX/Resources
