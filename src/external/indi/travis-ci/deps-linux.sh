#!/bin/bash

set -x -e

sudo apt-get -qq update

sudo apt-get -q -y install \
 cdbs \
 curl \
 dcraw \
 fakeroot \
 libboost-dev \
 libboost-regex-dev \
 libcfitsio3-dev \
 libftdi-dev \
 libgphoto2-dev \
 libgps-dev \
 libgsl0-dev \
 libjpeg-dev \
 libnova-dev \
 libopenal-dev \
 libraw-dev \
 libusb-1.0-0-dev \
 librtlsdr-dev \
 libfftw3-dev \
 wget

# Install libftdi for MGen driver
CURDIR="$(pwd)"
sudo apt-get -q -y install libusb-1.0-0-dev libconfuse-dev python3-all-dev doxygen libboost-test-dev python-all-dev swig g++
cd ~
mkdir ftdi_src
cd ftdi_src
wget http://archive.ubuntu.com/ubuntu/pool/universe/libf/libftdi1/libftdi1_1.2-5build1.dsc
wget http://archive.ubuntu.com/ubuntu/pool/universe/libf/libftdi1/libftdi1_1.2.orig.tar.bz2
wget http://archive.ubuntu.com/ubuntu/pool/universe/libf/libftdi1/libftdi1_1.2-5build1.debian.tar.xz
dpkg-source -x libftdi1_1.2-5build1.dsc
cd libftdi1-1.2
# Don't build the unnecessary packages
truncate -s 1668 debian/control
truncate -s 1819 debian/rules
dpkg-buildpackage -rfakeroot -nc -b -j6 -d -uc -us
cd ..
sudo dpkg -i libftdi1-2_1.2-5build1_amd64.deb
sudo dpkg -i libftdi1-dev_1.2-5build1_amd64.deb
cd $CURDIR

if [ ! -z $BUILD_INSTALL_GTEST ]; then
  DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
  /bin/bash ${DIR}/install-gtest.sh
else
  echo "==> BUILD_INSTALL_GTEST not specified"
fi

exit 0

