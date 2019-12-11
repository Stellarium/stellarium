#!/bin/bash

if [ $# -eq 0 ]; then
    echo "Usage: create-package.sh <vts-app-dir>"
    exit -1
fi


OUT=$1/Apps/Stellarium
SCRIPT_DIR=$(dirname `realpath $0`)
QT_DIR=$(qtpaths --install-prefix)

BUILD_DIR=/tmp/build/
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=Release \
    -DUSE_PLUGIN_VTS=1 -DUSE_PLUGIN_TELESCOPECONTROL=0 \
    -DENABLE_GPS=0 \
    $SCRIPT_DIR/../../
make -j8
cd -

echo Copy all files to \"$OUT\"

for file in landscapes nebulae skycultures stars textures data
do
    cp -rf $SCRIPT_DIR/../../$file $OUT/bin/
done

mkdir -p $OUT/bin $OUT/doc
cp $SCRIPT_DIR/data/launcherStellarium.sh $OUT/bin/
cp $SCRIPT_DIR/data/stellariumVtsConf.ini $OUT/doc/
cp $SCRIPT_DIR/data/vtsclient.json $OUT/doc/
cp $SCRIPT_DIR/data/config.ini $OUT/bin/
cp $BUILD_DIR/src/stellarium $OUT/bin/

for lib in Core DBus Gui NetworkAuth Network OpenGL Script Widgets XcbQpa \
           Concurrent MultimediaWidgets Multimedia \
           Positioning SerialPort PrintSupport
do
    cp $QT_DIR/lib/libQt5${lib}.so.5 $OUT/bin/
done

cp $QT_DIR/lib/libicui18n.so.56 $OUT/bin/
cp $QT_DIR/lib/libicuuc.so.56 $OUT/bin/
cp $QT_DIR/lib/libicudata.so.56 $OUT/bin/

cp -rf $QT_DIR/plugins/xcbglintegrations $OUT/bin/
cp -rf $QT_DIR/plugins/platforms $OUT/bin/

cp $SCRIPT_DIR/../../data/icons/128x128/stellarium.png $OUT/doc/icon.png
