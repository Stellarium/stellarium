# Linux

Each time Stellarium is released, the source code is released in Github's packaging system.  Building source code that is released in this way should give you a working copy of Stellarium which is functionally identical to the binaries for that release.  

It is also possible to get the "in development" sourcecode using Git.  This may contain new features which have been implemented since the last release of Stellarium, so it's often more fun.  

## Warning: 
*Git versions of the Stellarium sourcecode are work in progress, and as such may produce an unstable program, may not work at all, or may not even compile.*

## Building latest versions

First make sure all [build dependencies](Linux-build-dependencies.md) are met on your system.  Then decide whether you want to build a release version or follow the bleeding edge of development.

### Getting the source code
Open a terminal and cd to the directory where you wish to build Stellarium. 
Let us assume <code>~/DEV/Stellarium</code> here.  

#### Extract the tarball or ZIP containing the source code:
You can find the source code from
<pre>
https://github.com/Stellarium/stellarium/releases
</pre>

 Do this command in a terminal (if you prefer, you might use arK or some other graphical archive tool): 
<pre>
  $ tar zxf stellarium-0.17.0.tar.gz
</pre>
You should now have a directory <code>stellarium-0.17.0</code> with the source code in it.

#### Clone project from github:
If you want to build the developmental version, which may have bugs, you can download sources from github
Execute this command:
 <pre>
git clone https://github.com/Stellarium/stellarium.git
</pre>

This will create the directory <code>stellarium</code> which contains the source code.

### Build
We prefer to build the program not inside the source directory, but you can adapt these instructions to build whereever you want. 

In a terminal, change into the <code>DEV/Stellarium</code> directory, 
make the build directory and change into it
<pre>
 cd ~/DEV/Stellarium/
 mkdir -p builds/unix
 cd builds/unix
</pre>

[[Configuring Build Options|Configure]] the build using ''cmake''. Let's assume you want to build only a fast "release" version.
<pre>
 cmake -DCMAKE_BUILD_TYPE="Release" ../../stellarium
</pre>
(Of course, with the numbered source code directory, you would specify <code>../../stellarium-0.17.0</code>.

By default this will configure Stellarium to be installed in the <code>/usr/local</code> area.  If you want another location, use this [[Configuring Build Options|option]] to ''cmake'', e.g.:
<pre>
 cmake -DCMAKE_INSTALL_PREFIX=/opt/stellarium ../..
</pre>

Now build using make (and make use of a few more cores your computer may have):
<pre>
 make -j6
</pre>

To run Stellarium from the source tree, change back to the root of the source tree, and call the binary like this:
<pre>
 cd ../../stellarium
 ../builds/unix/src/stellarium
</pre>

If you want to run from the stellarium folder without using the terminal, copy the stellarium binary from <code>builds/unix/src</code> to the stellarium folder. Then a click on the binary will start stellarium.

To finish installation and let other users use the program, from the <code>builds/unix</code> directory enter this command:
<pre>
 sudo make install
</pre>

That's it.  You should now have Stellarium built and installed.



### Updating the source code
If you have previously built the Git code, but want to see what changes have been made since you did that, just cd into the stellarium directory and use the command:
<pre>
 git pull
</pre>
This will download just the changes which were made since you last retrieved files from the repository.  Often, all that will be required is to build from the make stage, but if there are new files you will need to build from the CMake stage.


## Creating source packages
After building of Stellarium you can create a source package for distributions:
<pre>
 make package_source
</pre>

## Creating binary packages
After building of Stellarium you can create a binary packages for distributions.
### TGZ
* After building of source code (simple binary package):
<pre>
 make package
</pre>
### RPM
* After building of TGZ binary package (Note: a building recommended with <code>-DCMAKE_INSTALL_PREFIX=/usr</code>):
<pre>
 cpack -G RPM
</pre>

### DEB
After building of TGZ binary package (Note: a building recommended with <code>-DCMAKE_INSTALL_PREFIX=/usr</code>):
<pre>
 cpack -G DEB
</pre>


## Current Build Issues
Note that the development code is a work in progress, and as such, please don't expect it to build straight off the bat.  Often it will be fine, but sometimes the build will be broken.

If the build seems to be broken for extended periods, try a thorough clean of the build directory (i.e. remove <code>builds/unix</code>), and start from the beginning.  Check cmake output that there are no new dependencies which you are missing.  If you still have trouble, post to the [forums](https://sourceforge.net/p/stellarium/discussion/278769) or [mailing list](https://lists.sourceforge.net/lists/listinfo/stellarium-pubdevel stellarium-pubdevel).

### Enabling Sound and Video Support
Stellarium's sound and video support is a compile time option.  To use sound in Stellarium you need to have a version of Qt which supports the multimedia (You should install the ''qtmultimedia5-dev'' and ''libqt5multimedia5-plugins'' packages).  You also need a recent version of ''cmake''.

Sound support is controlled by the ''cmake'' option <code>ENABLE_MEDIA</code>.  You can change this setting using ''cmake'', by editing the <code>CMakeCache.txt</code> file, or by supplying the <code>-DENABLE_MEDIA=1</code> option when you first run ''cmake''.

Tested on Ubuntu: You need to install GStreamer plugins. Most critical seems to be gstreamer0.10-ffmpeg from https://launchpad.net/~mc3man/+archive/ubuntu/gstffmpeg-keep, then it plays MP4 (h264), Apple MOV(Sorenson) and WMV. Some type of AVI failed.
If Stellarium is built with ENABLE_MEDIA, package managers should also make sure to change dependencies to include these. (This is however a nonstandard ppa, not sure how to proceed here!)


### Could not find a package configuration file provided by "Qt5Positioning"
If you get the following error when executing cmake:
<pre>
  -- Platform: Linux-4.4.0-64-generic
  -- Found Qt5: /usr/lib/x86_64-linux-gnu/qt5/bin/qmake (found suitable version "5.5.1")
  -- GPS: support by Qt's NMEA handling enabled.
  CMake Error at CMakeLists.txt:435 (FIND_PACKAGE):
  By not providing "FindQt5Positioning.cmake" in CMAKE_MODULE_PATH this
  project has asked CMake to find a package configuration file provided by
  "Qt5Positioning", but CMake did not find one.
  Could not find a package configuration file provided by "Qt5Positioning"
  with any of the following names:
    Qt5PositioningConfig.cmake
    qt5positioning-config.cmake
  Add the installation prefix of "Qt5Positioning" to CMAKE_PREFIX_PATH or set
  "Qt5Positioning_DIR" to a directory containing one of the above files.  If
  "Qt5Positioning" provides a separate development package or SDK, be sure it
  has been installed.
</pre>
Then either install the latest dependencies: 
<pre>
  sudo apt-get install libgps-dev libqt5positioning5 qtpositioning5-dev libqt5positioning5-plugins
</pre>
or, if you don't want GPS support, execute instead:
<pre>
  cmake -DENABLE_GPS=0 -DENABLE_LIBGPS=0 ../..
</pre>

# MacOS

This document describes how to build Stellarium from sources on macOS. This process ought to improve over time.

The set of instructions was written for the 0.18.x and later releases and using an Intel machine with macOS High Sierra (10.13.x) or MacOS Mojave (10.14.4 at the moment).

## Warning

__Development versions of Stellarium are work in progress, and as such may produce an unstable program, may not work at all, or may not even compile.__

## Prepare macOS to build Stellarium

### First step: Install Apple Developer Tools

1. Install the latest version of Apple's Developer Tools: [https://developer.apple.com/xcode/](https://developer.apple.com/xcode/).

### Second step: Install Qt

#### Option 1 (preferred): With HomeBrew

1. Install HomeBrew, if you didn't yet. Follow instructions on: [https://brew.sh](http://brew.sh).
1. Install latest Qt: `$ brew install qt`
1. Add Qt to your PATH environment variable, adding to your `.bash_profile` file the following line:

   `export PATH=/usr/local/opt/qt/bin:$PATH`

#### Option 2: Using the distribution from the Qt Company

1. Install the [latest stable version of Qt](https://www.qt.io/download-qt-installer) (5.12.2 at the moment), or use the [offline installer](http://download.qt.io/official_releases/qt/5.12/5.12.2/qt-opensource-mac-x64-5.12.2.dmg).
1. Add Qt5 to your PATH environment variable, adding to your `.bash_profile` file the following line:

   `export PATH=~/Qt/5.12/clang_64/bin:$PATH`

### Third step: Install CMake and Git

You need some tools to build Stellarium on macOS. CMake and Git are required, Gettext is optional. There are (at least) two simple options to install these tools on macOS. Choose only one of them (HomeBrew or MacPorts).

#### Option 1 (preferred): With HomeBrew

Simpler, lighter and safer (doesn't need `sudo`) than MacPorts.

1. Install HomeBrew, if you didn't yet. Follow instructions on: [https://brew.sh](http://brew.sh).
1. Install CMake: `$ brew install cmake`
1. Install Git: `$ brew install git`
1. (optional) Install (and link) Gettext: `$ brew install gettext; brew link gettext --force`

#### Option 2: With MacPorts

1. Install MacPorts. Follow instructions on: [https://www.macports.org/install.php](https://www.macports.org/install.php)
1. Install CMake: `$ sudo port install cmake`
1. Install Git: `$ sudo port install git`

### Last step: Restart terminal session

Don't forget to restart your terminal session, so that your new `PATH` setting is taken in account.

## Build Stellarium

Create a build directory with your favorite shell (the following directory is just an example, you can pick any name and path you want)
```bash
$ mkdir -p ~/Documents/Development
$ cd ~/Documents/Development
```

### Get Stellarium source code

In that directory checkout the sources with the git command:
```bash
$ git clone https://github.com/Stellarium/stellarium.git
```
If you have already done it once, don't do it again: you have just to update your copy:
```bash
$ cd stellarium
$ git pull
```

#### Alternative method: extract the tarball or ZIP containing the source code:

Download the source code from [https://github.com/Stellarium/stellarium/releases](https://github.com/Stellarium/stellarium/releases).

Then launch this command in a terminal (don't forget to adapt the version number!):
```bash
$ tar zxf stellarium-0.19.0.tar.gz
```
You should now have a directory `stellarium-0.19.0` with the source code in it. Change its name to `stellarium`.

### Time to compile Stellarium

We setup the build directory
```bash
$ cd stellarium
$ mkdir -p builds/macos
$ cd builds/macos
```
We run cmake ...
```bash
$ cmake ../..
```
... or, if we want a _release_ build (instead of a _debug_ one), ...
```bash
$ cmake -DCMAKE_BUILD_TYPE=Release ../..
```
... and we compile (and make use of a few more cores the computer may have):
```bash
$ make -j6
```

## Packaging

### Build the macOS application

__IMPORTANT__: you should delete or move aside the old Stellarium.app before each new build:
```bash
$ rm -r Stellarium.app/
```
Then build the macOS application:
```bash
$ make install
```
You'll find now an application `Stellarium.app` with the correct icon in the build directory.

### Create the DMG (Apple Disk Image)

```bash
$ mkdir Stellarium
$ cp -r Stellarium.app Stellarium
$ hdiutil create -format UDZO -srcfolder Stellarium Stellarium.dmg
```

