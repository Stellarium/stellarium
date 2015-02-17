Installation of Stellarium trunk@pre0.13.2 for Raspberry Pi

Georg Zotti, December 12-14, 2014 and Jan 4-29, 2015

1) INSTALL Qt5.4
=================

I assume you run a current Raspbian. If you use Raspbian (alone or on
a NOOBS card), make sure to leave approx 4GB free on your SD
card. Mount this into /home/pi/opt and build Qt here. ((i.e. use a 8GB
for standalone Raspbian installation, or a 16GB card with Raspbian and
OpenElec, RISC OS or whatever you want to try, but leave 4GB or more
free!)


First you must install Qt5.4. You can build ("cross-compile") it on a faster system, 
or build it directly on your Pi. For the latter, follow this: 

http://qt-project.org/wiki/Native_Build_of_Qt5_on_a_Raspberry_Pi


don't forget 

$ sudo apt-get install mtdev-tools (NO, does not help)
MTDEV detects, but is not found during build.

my configure call: 

./configure -v -opengl es2 -device linux-rasp-pi-g++ -device-option CROSS_COMPILE=/usr/bin/ -opensource -confirm-license -optimized-qmake -reduce-exports -release -qt-pcre -make libs -prefix /usr/local/qt5 -skip qtwebkit -skip qtwebengine --enable-egl -strip -qreal float  -no-mtdev  2>&1 | tee GZ_configure-out.txt

We skip qtweb... stuff: not needed and takes lots of time.

N.B. Setting qreal to float may cause incompatibilities with other software, but I don't care for now. maybe it speeds things up somewhat.

Configure takes about 1-2 hours (?)

Now: date > GZ_makeLog.txt;   make 2>&1 | tee -a GZ_makeLog.txt ; date >> GZ_makeLog.txt



WAIT!!! See (8): Prepare wayland/maynard/weston before running qmake!
... not needed. Qt configure explicitly says all setup is done in the module. So Build Qt first.


Packages to install before building: 

apt-get install gperf




Consider overclocking with raspi-config to high (900/950MHz) or even
Turbo (1GHz), but keep an eye on the temperature. For Turbo, a better
power supply (5V/1.5A) is recommended.

I did not use a git clone (was not completely updated on release day),
but the qt5.4-opensource-everywhere.tgz source package.
 
Make sure the cube example works. In case of 

>  EGL Error : Could not create the egl surface: ...

You may need to increase GPU memory in raspi-config. 


2) INSTALL CMAKE
===================
 
Next problem is CMake. Currently Raspbian comes with 2.8.9, we need 2.8.11, so let's build 3.0.2:

$ apt-get remove cmake

I keep the dependend packages for now: cmake-data and some xml stuff.

$ wget http://www.cmake.org/files/v3.0/cmake-3.0.2.tar.gz
$ tar zxvf cmake-3.0.2.tar.gz
$ cd cmake-3.0.2
$ less README.rst
$ mkdir build
$ cd build
$ ../bootstrap
$ make
$ make install



3) Run CMake for STELLARIUM
=============================

$ bzr co lp:~georg-zotti/stellarium/gz_raspi

If that fails with a timeout after downloading 600MB, still: cd gz_raspi, then: bzr co (without other arguments). This should complete the download and unpacking.

Follow http://stellarium.org/wiki/index.php/Compilation_on_Linux with:

$ cd gz_raspi
$ mkdir -p builds/raspi
$ cd builds/raspi
$ cmake ../..

Now we must add some dependencies. 
Edit build/src/CMakeFiles/stellarium.dir/link.txt and add this line to the end:

 -Wl,-rpath-link,/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link,/lib/arm-linux-gnueabihf -Wl,-O1  /opt/vc/lib/libGLESv2.so


TODO: Find out what to change in CMakeLists.txt to make that automatic.

4) Make
=========

$ make
$ sudo make install

This takes again some time but should now compile without further issues.


5) CHANGES MADE TO THE CODE:
================================

CMakeFile.txt: added Raspi settings, minQt=5.4, prevent CMake policy warning for >3.0.2.
               For now, deactivated building most plugins to save build time. Can be reactivated later, 
               on this platform select only what you need.
StelOpenGL.hpp add some lines to emergency-define GL_DOUBLE, set that to regular float. Hopefully effect is small. This has in parallel been added to official trunk!

6) PROBLEMS DETECTED
======================

After make install, running it displays the splash screen, but aborts with:

> [This is Stellarium 0.13.2 - ...]
> EGLFS: OpenGL windows cannot be mixed with others.
> Aborted

This also occurs if launched from the command line. In this case, system returns to command line prompt but keyboard is deactivated. 
To avoid this, set environment variable QT_QPA_ENABLE_TERMINAL_KEYBOARD to 1. [http://doc-snapshot.qt-project.org/qt5-dev/embedded-linux.html#eglfs]



Some explanation is here, and may be another thing to consider in Qt5.4 porting. 
http://comments.gmane.org/gmane.comp.lib.qt.user/11116
http://qt-project.org/forums/viewthread/44673

So it seems we need a "window compositor". QtWayland seems to be available for this.

7) TRY MAYNARD
=================

On your Raspberry Pi, make sure to boot into text mode (use raspi-config to reconfigure).

Follow innstructions from  http://raspberrypi.collabora.co.uk/maynard.html:

$ wget http://raspberrypi.collabora.co.uk/setup-maynard.sh
$ bash ./setup-maynard.sh 

This reconfigures your Raspberry Pi and downloads quite a lot of additional packages (235MB)
After a reboot, start it with 'maynard' from the command line

 
8) we need QtWayland now. 
Build Wayland:
=================
Follow http://www.ics.com/blog/building-qt-and-qtwayland-raspberry-pi:

Some dirnames have to be adapted, we are building on-board

The following line may be not necessary, all packages should have been installed for Qt5 building.

$ sudo apt-get install libxcb1 libxcb1-dev libx11-xcb1 libx11-xcb-dev libxcb-keysyms1 libxcb-keysyms1-dev libxcb-image0 libxcb-image0-dev libxcb-shm0 libxcb-shm0-dev libxcb-icccm4 libxcb-icccm4-dev libxcb-sync0 libxcb-sync0-dev libxcb-render-util0 libxcb-render-util0-dev libxcb-xfixes0-dev libxrender-dev libxcb-shape0-dev libxcb-randr0-dev libxcb-glx0-dev

More packages. Some of these are new, so just type this line. 

$ sudo apt-get install libxkbcommon-dev libudev-dev libwayland-dev libpng12-dev libjpeg8-dev libmtdev-dev autotools-dev autoconf automake bison flex libasound2-dev libxext-dev x11proto-xext-dev libxcursor-dev libxfixes-dev x11proto-fixes-dev libffi-dev libsm-dev libxcomposite-dev

More was missing from above:

$ sudo apt-get install libtool xutils-dev [ contains xorg-macros ]

get&build Wayland: (dir as example just in homedir, when mounted ~/opt is already full with Qt...)
$ mkdir ~/setups
$ cd ~/setups
$ git clone git://anongit.freedesktop.org/wayland/wayland
$ cd wayland
$ ./autogen.sh
$ ./configure --disable-documentation
$ make
$ cp wayland-scanner /usr/local/bin

get&build xkbcommon:
$ git clone git://people.freedesktop.org/xorg/lib/libxkbcommon.git
$ cd libxkbcommon/
$ ./autogen.sh [--prefix=some_prefix --host=arm-linux-gnueabihf]
[ There may be warnings to update git config. do that, rerun autogen.]
Currently there is a warning about xcb-xkb<=1.10. Run again with --disable-x11 and see if that helps:
$ ./autogen --disable-x11
$ make && make install

Build QtWayland: This *should* have worked:
$ cd ~/opt/qt54/qtwayland
$ which qmake
> /opt/qt5-rpi/bin/qmake (or /usr/local/qt5/bin/qmake)
$ which wayland-scanner
> /opt/qt5-rpi/bin/wayland-scanner (or /usr/local/bin/wayland-scanner)
$ cd $BDIR/qtwayland
$ qmake CONFIG+=wayland-compositor
$ make
$ sudo make install

Unfortunately, the qmake line indicates it cannot find the installed wayland. It is in $PATH, so something else must be configured. 
TODO: Find out how to build QtWayland on Raspberry Pi, then continue...



