Installation of Stellarium trunk@pre0.13.2 for Raspberry Pi

Georg Zotti, December 12-14, 2014 and Jan 4-6, 2015

1) INSTALLING Qt5.4
=====================

I assume you run a current Raspbian. If you use Raspbian (alone or on
a NOOBS card), make sure to leave approx 4GB free on your SD
card. Mount this into /home/pi/opt and build Qt here. ((i.e. use a 8GB
for standalone Raspbian installation, or a 16GB card with Raspbian and
OpenElec, RISC OS or whatever you want to try, but leave 4GB or more
free!)


First you must install Qt5.4. Follow this: 

http://qt-project.org/wiki/Native_Build_of_Qt5_on_a_Raspberry_Pi

Consider overclocking with raspi-config to high (900/950MHz) or even
Turbo (1GHz), but keep an eye on the temperature. For Turbo, a better
power supply (5V/1.5A) is recommended.

I did not use a git clone (was not completely updated on release day),
but the qt5.4-opensource-everywhere.tgz source package.
 
Make sure the cube example works. In case of 

  EGL Error : Could not create the egl surface: ...

You may need to increase GPU memory in raspi-config. 


2) INSTALL CMAKE
===================
 
Next problem is CMake. Currently Raspbian comes with 2.8.9, we need 2.8.11, so let's build 3.0.2:

> apt-get remove cmake

I keep the dependend packages for now: cmake-data and some xml stuff.

> wget http://www.cmake.org/files/v3.0/cmake-3.0.2.tar.gz
> tar zxvf cmake-3.0.2.tar.gz
> cd cmake-3.0.2
> less README.rst
> mkdir build
> cd build
> ../bootstrap
> make
> make install



3) Run CMake for STELLARIUM
=============================

bzr co lp:~georg-zotti/stellarium/gz_raspi

If that fails with a timeout after downloading 600MB, still: cd gz_raspi, then: bzr co (without other arguments).

Follow http://stellarium.org/wiki/index.php/Compilation_on_Linux with:

cd gz_raspi
mkdir -p builds/raspi
cd builds/raspi
cmake ../..

Now we must add some dependencies. 
Edit build/src/CMakeFiles/stellarium.dir/link.txt and add this line to the end:

-Wl,-rpath-link,/usr/lib/arm-linux-gnueabihf -Wl,-rpath-link,/lib/arm-linux-gnueabihf -Wl,-O1  /opt/vc/lib/libGLESv2.so


TODO: Find out what to change in CMakeLists.txt to make that automatic.

4) Make
=========

make
make install

This takes again some time but should now compile without further issues.


5) CHANGES MADE TO THE CODE:
================================

CMakeFile.txt: added Raspi settings, minQt=5.4, prevent CMake policy warning for >3.0.2.
               For now, deactivated building most plugins to save build time. Can be reactivated later, 
               on this platform select only what you need.
StelOpenGL.hpp add some lines to define GL_DOUBLE, set that to regular float. Hopefully effect is small...

6) PROBLEMS DETECTED
======================

After make install, running it displays the splash screen, but aborts with:

[This is Stellarium 0.13.2 - ...]
EGLFS: OpenGL windows cannot be mixed with others.
Aborted

This also occurs if launched from the command line. In this case, system returns to command line prompt but keyboard is deactivated. 

Some explanation is here, and may be another thing to consider in Qt5.4 porting. 
http://comments.gmane.org/gmane.comp.lib.qt.user/11116
http://qt-project.org/forums/viewthread/44673
