#!/bin/bash

#need:
#export JAVA_HOME=/c/Program\ Files/Java/jdk1.6.0_30/
#  export ANDROID_NDK=/c/Users/Brady/necessitas/android-ndk-r6b
#export PATH=/c/Users/Brady/necessitas/apache-ant-1.8.2/bin:/c/Users/Brady/necessitas/android-sdk/tools:$PATH

MY_CMAKE_FLAGS=" \
-DCMAKE_TOOLCHAIN_FILE=../../android/android.toolchain.cmake
-DCMAKE_VERBOSE_MAKEFILE=ON"

cmake $MY_CMAKE_FLAGS -G"Unix Makefiles" ../..