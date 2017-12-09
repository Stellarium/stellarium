#!/bin/bash

# This is a script building libraries for travis-ci
# It is *not* for general audience

SRC=../../3rdparty/

if [ ${TRAVIS_OS_NAME} == "linux" ] ; then
    LIBS="libapogee libfishcamp libfli libqhy libqsi libsbig libinovasdk libdspau"
else 
    LIBS="libqsi"
fi

if [ .${TRAVIS_BRANCH%_*} == '.drv' ] ; then 
    DRV=lib"${TRAVIS_BRANCH#drv_}"
    if [ -d 3rdparty/$DRV ] ; then
        LIBS="$DRV"
    else 
        LIBS=""
    fi
    echo "[$DRV] [$LIBS]"
    if [ ${TRAVIS_OS_NAME} == "osx" ] ; then
        echo "Cannot build one driver on OSX"
        LIBS=""
    fi
fi

for lib in $LIBS ; do
(
    echo "Building $lib ..."
    mkdir -p build/$lib
    pushd build/$lib
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local . $SRC/$lib -DFIX_WARNINGS=ON -DCMAKE_BUILD_TYPE=$1
    make
    make install
    popd
)
done
