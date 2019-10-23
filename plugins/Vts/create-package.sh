#!/bin/bash

OUT=$1/Stellarium
SCRIPT_DIR=$(dirname `which $0`)

# TODO: Compute those.
BUILD_DIR=$SCRIPT_DIR/../../build/
QT_DIR=~/Qt/5.12.4/gcc_64/

echo Copy all files to \"$OUT\"

mkdir -p $OUT/bin $OUT/doc
cp $SCRIPT_DIR/data/launcherStellarium.sh $OUT/bin/
cp $SCRIPT_DIR/data/stellariumVtsConf.ini $OUT/doc/
cp $SCRIPT_DIR/data/vtsclient.json $OUT/doc/
cp $SCRIPT_DIR/data/config.ini $OUT/bin/
cp $BUILD_DIR/src/stellarium $OUT/bin/

for lib in Core DBus Gui NetworkAuth Network OpenGL Script Widgets XcbQpa
do
    cp $QT_DIR/lib/libQt5${lib}.so.5 $OUT/bin/
done
