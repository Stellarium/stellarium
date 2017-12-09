#!/bin/bash

set -x -e

echo ${TRAVIS_OS_NAME}

# The build-libs.sh must be run first for this to work
if [ .${TRAVIS_BRANCH%_*} == '.drv' ] ; then 
    DRV="indi-${TRAVIS_BRANCH#drv_}"
    echo "Building $DRV"
    mkdir -p build/$DRV
    pushd build/$DRV
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ . ../../3rdparty/$DRV -DFIX_WARNINGS=ON -DCMAKE_BUILD_TYPE=$1
    make
    popd
else
    echo "Building all 3rd party drivers"
    mkdir -p build/3rdparty
    pushd build/3rdparty
    cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ . ../../3rdparty/ -DFIX_WARNINGS=ON -DCMAKE_BUILD_TYPE=$1
    make
    popd
fi

exit 0

