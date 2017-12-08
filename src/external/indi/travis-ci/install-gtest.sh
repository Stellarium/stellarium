#!/bin/bash
set -e

mkdir -p build
pushd build
git clone https://github.com/google/googletest.git
if [ ! -d googletest ]; then
echo "Failed to get googletest.git repo"
exit 1
fi
pushd googletest
mkdir build
cd build
cmake .. -DCMAKE_CXX_FLAGS="-fPIC"
sudo make install
popd
rm -rf googletest
popd
