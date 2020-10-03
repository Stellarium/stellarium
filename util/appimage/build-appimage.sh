#!/bin/sh

#
# A tool for build an AppImage package of Stellarium
#
# Copyright (c) 2020 Alexander Wolf
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#

dir=$(pwd)
result="${dir%"${dir##*[!/]}"}"
result="${result##*/}"

if [ $result = 'appimage' ]
then
    # Stage 1: Check required packages
    ait=$(whereis appimagetool | sed 's/appimagetool://i')
    if [ -z $ait ]
    then
	# Install appimagetool AppImage
	sudo wget https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage -O /usr/local/bin/appimagetool
	sudo chmod +x /usr/local/bin/appimagetool
    fi

    builder=$(whereis appimage-builder | sed 's/appimage-builder://i')
    if [ -z $builder ]
    then
	# Installing dependencies
	sudo apt install -y python3-pip python3-setuptools patchelf desktop-file-utils libgdk-pixbuf2.0-dev fakeroot
	# Installing latest tagged release
	sudo pip3 install appimage-builder
    fi

    # Stage 2: Build an AppImage package
    ROOT=../..
    mkdir -p ${ROOT}/builds/appimage
    cd ${ROOT}/builds/appimage
    rm -rf *

    dtag=$(git describe --abbrev=0 | sed 's/v//i')
    rtag=$(git describe --tags | sed 's/v//i')
    revision=$(git log -1 --pretty=format:%h)

    if [ $rtag = $dtag ]
    then
	version=${dtag}
    else
	version="${dtag}-${revision}"
    fi
    export APP_VERSION=${version}

    echo "\nLet's try build an AppImage for version ${version}\n"

    appimage-builder --recipe ${ROOT}/util/appimage/stellarium-appimage.yml --skip-test
else
    echo "Wrong directory! Please go to util/appimage directory and run again..."
fi

