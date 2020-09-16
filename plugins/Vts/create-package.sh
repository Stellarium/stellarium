#!/bin/bash

OUT=/tmp/Stellarium
SCRIPT_DIR=$(dirname `realpath $0`)
QT_DIR=/usr/lib64/qt5

BUILD_DIR=/app/build/
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=Release \
    -DUSE_PLUGIN_VTS=1 -DUSE_PLUGIN_TELESCOPECONTROL=0 -DENABLE_GPS=0 -DENABLE_MEDIA=0 -DUSE_PLUGIN_SCENERY3D=0 \
    $SCRIPT_DIR/../../
make -j8
cd -

echo Copy all files to \"$OUT\"

mkdir -p $OUT/bin $OUT/doc

for file in landscapes nebulae skycultures stars textures data
do
    cp -rf $SCRIPT_DIR/../../$file $OUT/bin/
done

cp $SCRIPT_DIR/data/launcherStellarium.sh $OUT/bin/
cp $SCRIPT_DIR/data/stellariumVtsConf.ini $OUT/doc/
cp $SCRIPT_DIR/data/vtsclient.json $OUT/doc/
cp $SCRIPT_DIR/data/config.ini $OUT/bin/
cp $BUILD_DIR/src/stellarium $OUT/bin/

for lib in libQt5*.so.5 \
libicui18n.so.50 libicuuc.so.50 libicudata.so.50 libpcre2-16.so.0 \
libpng15.so.15 libssl.so.10 libcrypto.so.10 \
libxcb-icccm.so.4 libxcb-render-util.so.0 libxcb-image.so.0 libxcb-keysyms.so.1
do
    cp /lib64/${lib} $OUT/bin/
done

cp -rf $QT_DIR/plugins/xcbglintegrations $OUT/bin/
cp -rf $QT_DIR/plugins/platforms $OUT/bin/

cp $SCRIPT_DIR/../../data/icons/128x128/stellarium.png $OUT/doc/icon.png

cd /tmp
tar -czf Stellarium.tgz Stellarium
cd -
cp /tmp/Stellarium.tgz .
