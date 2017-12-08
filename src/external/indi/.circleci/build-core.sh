#!/bin/bash

set -x -e

if [ .${TRAVIS_BRANCH%_*} == '.drv' ] ; then 
    # Skip the build just use recent upstream version if it exists
    if [ ${TRAVIS_OS_NAME} == 'linux' ] ; then
        sudo apt-add-repository -y ppa:jochym/indi-devel
        sudo apt-get -qq update
        sudo apt-get -q -y install libindi-dev
    else
        brew install jochym/indi/libindi
    fi
else
    # Build everything on master
    echo "==> Building INDI Core"
    mkdir -p build/libindi
    pushd build/libindi
    cmake -DINDI_BUILD_UNITTESTS=ON -DCMAKE_INSTALL_PREFIX=/usr/local/ . ../../libindi/ -DFIX_WARNINGS=ON -DCMAKE_BUILD_TYPE=$1
    make
    make install
    popd
fi

exit 0

