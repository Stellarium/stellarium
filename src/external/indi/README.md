# libindi
[![Build Status](https://travis-ci.org/indilib/indi.svg?branch=master)](https://travis-ci.org/indilib/indi)
[![CircleCI](https://circleci.com/gh/indilib/indi.svg?style=svg)](https://circleci.com/gh/indilib/indi)

INDI is the defacto standard for open astronomical device control. INDI Library is an Open Source POSIX implementation of the [Instrument-Neutral-Device-Interface protocol](http://www.clearskyinstitute.com/INDI/INDI.pdf). The library is composed of server, tools, and device drivers for astronomical instrumentation and auxiliary devices. Core device drivers are shipped with INDI library by default. 3rd party drivers are also available in the repository and maintained by their respective owners.

## [Features](http://indilib.org/about/features.html)
## [Discover INDI](http://indilib.org/about/discover-indi.html)
## [Supported Devices](http://indilib.org/devices/)
## [Clients](http://indilib.org/about/clients.html)

# Building

## Install Pre-requisites

On Debian/Ubuntu:

```
sudo apt-get install libnova-dev libcfitsio-dev libusb-1.0-0-dev zlib1g-dev libgsl-dev build-essential cmake git libjpeg-dev libcurl4-gnutls-dev qtbase5-dev
```
## Get the code
```
git clone https://github.com/indilib/indi.git
cd indi
```
## Build libindi

```
mkdir -p build/libindi
cd build/libindi
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../libindi
make
sudo make install
```

## Build 3rd party drivers

You can build the all the 3rd party drivers at once (not recommended) or build the required 3rd party driver as required (recommended). Each 3rd party driver may have its own pre-requisites and requirements. For example, to build INDI EQMod driver:

```
cd build
mkdir indi-eqmod
cd indi-eqmod
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty/indi-eqmod
make
sudo make install
```

The complete list of system dependancies for all drivers on Debian / Ubuntu

```
sudo apt-get install libftdi-dev libgps-dev libraw-dev libgphoto2-dev libboost-dev libboost-regex-dev librtlsdr-dev libftdi1-dev libfftw3-dev
```

To build **all** 3rd party drivers, you need to run cmake and make install **twice**. First time is to install any dependencies of the 3rd party drivers (for example indi-qsi depends on libqsi), and second time to install the actual drivers themselves.

```
cd build
mkdir 3rdparty
cd 3rdparty
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty
make
sudo make install
cmake -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Debug ../../3rdparty
make
sudo make install
```

# Architecture


Typical INDI Client / Server / Driver / Device connectivity:


    INDI Client 1 ----|                  |---- INDI Driver A  ---- Dev X
                      |                  |
    INDI Client 2 ----|                  |---- INDI Driver B  ---- Dev Y
                      |                  |                     |
     ...              |--- indiserver ---|                     |-- Dev Z
                      |                  |
                      |                  |
    INDI Client n ----|                  |---- INDI Driver C  ---- Dev T
    
    
     Client       INET       Server       UNIX     Driver          Hardware
     processes    sockets    process      pipes    processes       devices



INDI server is the public network access point where one or more INDI Clients may contact one or more INDI Drivers. indiserver launches each driver process and arranges for it to receive the INDI protocol from clients on its stdin and expects to find commands destined for clients on the driver's stdout. Anything arriving from a driver process' stderr is copied to indiserver's stderr. INDI server only provides convenient port, fork and data steering services. If desired, a client may run and connect to INDI Drivers directly.

# Support

## [FAQ](http://indilib.org/support/faq.html)
## [Forum](http://indilib.org/forum.html)
## [Tutorials](http://indilib.org/support/tutorials.html)

# Development

## [INDI API](http://www.indilib.org/api/index.html)
## [INDI Developer Manual](http://indilib.org/develop/developer-manual.html)
## [Tutorials](http://indilib.org/develop/tutorials.html)
## [![Join the chat at https://gitter.im/knro/indi](https://badges.gitter.im/knro/indi.svg)](https://gitter.im/knro/indi?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

### How to create Github pull request (PR)

How I submit a PR:
1. Login with a Github account and fork the official INDI repository.
2. Clone the official INDI repository and add the forked INDI repository as a remote (git remote add ...).
3. Create a local Git branch (git co -b my_branch).
4. Work on the patch and commit the changes.
5. If it is ready push this branch to your fork repo (git push -f my_fork my_branch:my_branch).
6. Go to the official repo's github website in a browser, it will popup a message to create a PR. Create it.
7. Pushing updates to the PR: just update your branch (git push -f my_fork my_branch:my_branch)..

# Unit tests

In order to run the unit test suite you must first install the [Google Test Framework](https://github.com/google/googletest). You will need to build and install this from source code as Google does not recommend package managers for distributing distros.(This is because each build system is often unique and a one size fits all aproach does not work well).

Once you have the Google Test Framework installed follow this alternative build sequence:-

```
mkdir -p build/libindi
cd build/libindi
cmake -DINDI_BUILD_UNITTESTS=ON -DCMAKE_BUILD_TYPE=Debug ../../libindi
make
cd test
ctest -V
```

For more details refer to the scripts in the travis-ci directory.
