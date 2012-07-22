#!/bin/bash

CMAKE_INSTALL_PREFIX=$1
shift;
PROJECT_SOURCE_DIR=$1
shift;

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
cp -pr /Developer/Applications/Qt/plugins/{imageformats,iconengines}/* ${CMAKE_INSTALL_PREFIX}/plugins
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
