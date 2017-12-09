#!/bin/bash

set -x

if [ ${TRAVIS_OS_NAME} == "linux" ] ; then
    if [ ${TRAVIS_BRANCH} == 'package' ] ; then 
      echo "==> BUILD_DEB_PACKAGES activated"
      mkdir -p build/deb_libindi
      pushd build/deb_libindi
      cp -r ../../libindi .
      cp -r ../../cmake_modules libindi/
      cp -r ../../debian/libindi debian
      fakeroot debian/rules binary
      cd ..
      ../3rdparty/make_deb_pkgs 
    else
      echo "==> BUILD_DEB_PACKAGES not specified"
    fi
fi

exit 0

