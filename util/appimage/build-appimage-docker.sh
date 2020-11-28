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
arch=$(uname -m)

if [ $arch = "armv7l" ]; then
    arch="armhf"
fi

if [ "$result" = 'appimage' ]
then
    baseURL="https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-${arch}.AppImage"
    AppImage_Tool="/usr/local/bin/appimagetool"
    wget ${baseURL} -O ${AppImage_Tool}
    chmod a+x ${AppImage_Tool}

    ROOT=../..
    rm -rf ${ROOT}/builds/appimage
    mkdir -p ${ROOT}/builds/appimage
    cd ${ROOT}/builds/appimage

    version="edge"
    export APP_VERSION=${version}

    echo "\nLet's try build an AppImage for version '${version}'\n"

    appimage-builder --recipe ${ROOT}/util/appimage/stellarium-appimage-${arch}.yml --skip-test

    chmod +x *.AppImage
else
    echo "Wrong directory! Please go to util/appimage directory and run again..."
fi

