Celestron NexStar AUX protocol driver
-------------------------------------

Author: Paweł T. Jochym <jochym@wolf.ifj.edu.pl>

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
Do not use this driver unattended! It is ALPHA/BETA software!
Always have your power switch ready!
It is NOT SUITABLE for autonomous operation!
You use this software ON YOUR OWN RISK!
THERE ARE NO SLEW LIMITS IMPLEMENTED AT THIS STAGE!
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

This is eqmod-style driver for the NexStar and other Celestron AUX-protocol 
mounts. At the moment it only works over Wi-Fi link to the NexStar-Evolution 
mount. It should also work over serial link to AUX port if someone writes the
serial comm procedures. Since I do not have AUX-port adapter for my scope, 
I cannot write and test this part. I intend to implement the hand controller (HC)
serial link mode communication - it is supposedly more reliable than the direct 
AUX port communication and requires only standard USB-RS232 adapter and a 
serial cable with RJ connector. Furthermore RS-232 transmission is probably 
more robust than AUX-port connection.

The driver is in the alpha/beta stage.
It is functional and should work as intended but it is not complete.
I am using it and will be happy to help any brave testers.
I of course welcome any feedback/contribution.

What works:
- N-star alignment (with INDI alignment module)
- Basic tracking, slew, park/unpark
- GPS simulation. If you have HC connected and you have active gps driver 
  it can simulate Celestron GPS device and serve GPS data to HC. Works quite 
  nicely on RaspberryPi with a GPS module. You can actually use it as 
  a replacement for the Celestron GPS.

What does not work/is not implemented:
- Joystick control
- Slew limits
- Serial link
- HC interaction (tracking HC motor commands to function as joystick)
- Probably many other things

Install
-------

The driver is not included in the PPA distribution yet - due to its alpha/beta 
state. So to use it you need to compile it from the source yourself.

You can make a stand-alone compilation or build the debian packages for your 
system. It should be fairly easy. Let me know if something in the following
guide is wrong or if you have problem with compiling the driver.

Get the source
==============

You can get the source from the SVN repository of the system on sourceforge
maintained by the INDI project (see the website of the project) or get it
from the github mirror of the sourceforge repository maintained by the author 
of this driver. Both will do fine. The github repository lets you track the 
development of the driver more closely in the nse branch of the repository, 
since only master branch is uploaded back to the upstream SVN repository.

- Make some working directory and change into it.
- Get the source from the SVN or from github master branch - it takes a while 
  the repo is 64MB in size. These should at least compile and are kept 
  consistent - in the nse branch you may encounter WIP state with inconsistent 
  state of the driver. From github you get it like this:

  ```
  git clone https://github.com/jochym/indilib.git
  ```
- Create `build` directory at the same level as indilib directory created above.

Compiling on any linux system
=============================

The compilation is simple. You will need indi libraries installed. The best way
is to install libindi-dev package from the PPA. You may also want to have
indi-gpsd and gpsd packages installed (not strictly required). If you cannot use
the PPA you need to install libindi-dev from your distribution or compile the
indi libraries yourself using instructions from the INDI website. I have not
tested the backward compatibility but the driver should compile and work at
least with the 1.1 version of the library. My recommendation: use PPA if you
can. To compile the driver you will need also: cmake, cdbs, libindi-dev,
libnova-dev, zlib1g-dev. Run following commands (you can select other install
prefix):

```sh
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local . ../indilib/3rdparty/indi-nexstarevo/
make
```
You can run `make install` optionally at the end if you like to have the driver 
properly installed.


Building debian/ubuntu packages
===============================

To build the debian package you will need the debian packaging tools: 
`build-essential, devscripts, debhelper, fakeroot`

Create `package` directory at the same level as indilib directory with the 
cloned source. Then execute:

```sh
cd package
ln -s ../indilib/cmake_modules .
cp -r ../indilib/debian/indi-nexstarevo debian
cp -r ../indilib/3rdparty/indi-nexstarevo/* .
fakeroot debian/rules binary
fakeroot debian/rules clean
```
this should produce two packages in the main build directory (above `package`),
which you can install with `sudo dpkg -i indi-nexstarevo_*.deb`.

