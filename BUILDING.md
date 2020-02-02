Jump to...

* [Linux](#linux)
* [Mac OS](#macos)
* [Windows](#windows)
* [RPi](#stellarium-on-raspberry-pi)
* [Testing](#testing)

# Linux

Each time Stellarium is released, the source code is released in Github's packaging system.  Building source code that is released in this way should give you a working copy of Stellarium which is functionally identical to the binaries for that release.  

It is also possible to get the "in development" sourcecode using Git.  This may contain new features which have been implemented since the last release of Stellarium, so it's often more fun.  

## Warning: 
*Git versions of the Stellarium sourcecode are work in progress, and as such may produce an unstable program, may not work at all, or may not even compile.*

## build dependencies

Stellarium needs a few things to be installed on your system before you can build it. 

 - [CMake](http://www.cmake.org/)  cmake 
 Version 2.8.11 required for Stellarium 0.13.0 and above; Version 2.8.12 (3.1.3 better) required for Stellarium 0.13.3 and above.
 - [GNU C Compiler](https://gcc.gnu.org/) gcc
 Or compatible alternative (''clang'').
 - [GNU C++ Compiler](https://gcc.gnu.org/) g++
 Or compatible alternative (''clang++'').
 - [OpenGL](https://www.opengl.org/) libgl1-mesa-dev
 - [Zlib](http://www.zlib.net) zlib1g-dev 
 
 - [Qt](http://www.qt.io/) >= 5.4.0 Packages: 
   * qtdeclarative5-dev, qtdeclarative5-dev-tools,  libqt5declarative5, qtquick1-5-dev, qtquick1-qml-plugins needed to build Stellarium prior version 0.15.0
   * qtscript5-dev, libqt5svg5-dev, qttools5-dev-tools, qttools5-dev, libqt5opengl5-dev, 
   * libqt5serialport5-dev (Needed when Telescope Control plugin and/or GPS support enabled; See notes below.)
   * GPS for Qt5: qtpositioning5-dev, libgps-dev (''Optional.''. Needed to build GPS support. )
   * Multimedia for Qt5: qtmultimedia5-dev, libqt5multimedia5-plugins (''Optional.''. Needed to build audio support. )
 - [gettext](https://www.gnu.org/software/gettext/) gettext (''Optional.'' Required for developers for extract of lines for translation. No required for users who want to test the application.)
 - [Doxygen](http://doxygen.org/) doxygen (''Optional''. If you want to build the API documentation you will need this.)
 - [Graphviz](http://www.graphviz.org/) graphviz (''Optional''. Required to build the API documentation and include fancy class diagrams.)
 - [Git](https://git-scm.com) git to access the code repository

### Linux users:
If your distribution separates libraries into normal and development version, *make sure you also get the development ones* (e.g. in Debian/Ubuntu, the '''-dev''' packages).

Debian/Ubuntu users can install all these dependencies in one go by running this command in a terminal:
```
 sudo apt-get install build-essential cmake zlib1g-dev libgl1-mesa-dev gcc g++ \
      graphviz doxygen gettext git \
      qtscript5-dev libqt5svg5-dev qttools5-dev-tools qttools5-dev \
      libqt5opengl5-dev qtmultimedia5-dev libqt5multimedia5-plugins \
      libqt5serialport5 libqt5serialport5-dev qtpositioning5-dev libgps-dev \
      libqt5positioning5 libqt5positioning5-plugins
```

On CentOS 7.4 (First attempt 2018/March! Unfinished yet. No libgps? -- sorry):
```
sudo yum install cmake gcc \
     graphviz doxygen gettext git \
     qt5-qttools-devel.x86_64 qt5-qtscript-devel.x86_64 
     qt5-qtdeclarative-devel.x86_64 qt5-qtmultimedia-devel.x86_64 \
     qt5-qtserialport-devel.x86_64 qt5-qtlocation-devel.x86_64 
```
LibINDI complains about some missing C99 flag. You may need to disable Telescope plugin in the CMakeFile.txt, or solve this and report your solution please. Also note that qDebug() reporting fails giving information about OpenGL on this system. 

## Latest Qt Libraries for Linux distros which don't have them in the repos

Stellarium tracks the recent Qt releases fairly closely and as such many Linux distribution repositories do not contain an up-to-date enough version for building Stellarium.  In the case of Ubuntu, the ''backports'' repository is often good enough, but there may be a need to install it "outside" your package manager.   Here's how.

The Qt development team provides binary installers.  If you want to build Qt yourself from source, this is fine but it will take a ''long'' time.  We recommend the following procedure for manually installing the latest Qt (required: 5.4 or above at the moment):
* Download the Linux/X11 package from [here](http://www.qt.io/download-open-source/).  Choose 32/64 bit as appropriate.
* Install it in `/opt/Qt5`
* When you want to build Stellarium, execute these commands to set up the environment so that the new Qt is used (for 64-bit package):
```
 export QTDIR=/opt/Qt5/5.4/gcc_64
 export PATH=$QTDIR/bin:$PATH
 export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
```
* After installation, you should write a script which sets `LD_LIBRARY_PATH` and then calls Stellarium:
```
 #!/bin/sh
 export QTDIR=/opt/Qt5/5.4/gcc_64
 export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
 ./stellarium
```

Once these essentials are installed the compile should progress with no more problems.

## Qt versions:

* Stellarium 0.13.0 required Qt version 5.1.0 or above for building.
* Stellarium 0.13.1 and 0.13.2 required Qt version 5.2.0 or above for building.
* Stellarium 0.13.3 required Qt version 5.3.0 or above for building (Or, if you disable Scenery3D plugin you can reduce Qt version to 5.2.0).
* Stellarium 0.15.2 required Qt version 5.3.0 or above (recommended: 5.4) for building.
* Stellarium HEAD (as of V0.16.0) required Qt version 5.4.0 or above for building.

## Building latest versions

First make sure all build dependencies are met on your system.  Then decide whether you want to build a release version or follow the bleeding edge of development.

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


# Windows

This is the instruction for building Windows 32/64-bit version of Stellarium using Microsoft Visual Studio 2017 and Qt5 (VC 2017). See below for instructions to build with QtCreator. You can use VS 2015 also, but cannot 2013. If you use VS 2015, your QT msvc version, Path, cmake compiler, and so on are all used by 2015.

## Installation of development tools & libraries
For building Stellarium from source code you need some required tools and libraries.

### Microsoft Visual Studio 2017
You need to download and install [VS 2017 Community](https://www.visualstudio.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) (or "better" - e.g. Professional) from Microsoft website.

### Git

To get the source code of Stellarium you need to install some git environment. Git for Windows seems ok, or the Git Bash and Git GUI, whatever seems suitable for you. But it is not necessary. You can [Download Source Code](https://github.com/Stellarium/stellarium/archive/master.zip) by web.

### Qt5
Stellarium needs Qt5 for building. You can get latest version of Qt from website of [Qt Project](http://www.qt.io/). We recommend to use Qt 5.9 or later, now in the archives section, which you can get Qt from [this](https://www.qt.io/download-qt-installer?hsCtaTracking=9f6a2170-a938-42df-a8e2-a9f0b1d6cdce%7C6cb0de4f-9bb5-4778-ab02-bfb62735f3e5). It is online installer using web. When you excute installer, you should select checkbox. Entire QT 5.11 is not necessary. But you must select QT Script and msvc2017 among so many checkboxs.

**Note:** for following steps we will use path <tt>C:\Qt\Qt5.11.0</tt> as directory with installed Qt5 framework.


### CMake
Stellarium uses [CMake](http://cmake.org/) for configuration of the project and we recommended use [CMake 3.4.1 or better](http://cmake.org/cmake/resources/software.html).

## Configuring the build environment

After installing all required libraries and tools you should configure the build environment.

Add <tt>C:\Qt\Qt5.11.0</tt> to PATH variable - you should add string <tt>C:\Qt\Qt5.11.0\msvc2017\;C:\Qt\Qt5.11.0\msvc2017\bin</tt> for 32-bit or <tt>C:\Qt\Qt5.11.0\msvc2017_64\;C:\Qt\Qt5.11.0\msvc2017_64\bin</tt> for 64-bit to PATH variable. 

**Note:** After changes to the <tt>PATH</tt> variable you should reboot the computer to apply those changes.

## Getting source code
### Release versions

You can download the source code for the release versions from the sourceforge or github download area.  Save the file to the <tt>C:/Devel</tt> directory as example. The file name should be <tt>stellarium-0.18.0.tar.gz</tt> or similar. 
 
You will need a decompression program installed in Windows, for example [7-Zip](http://www.7-zip.org/).

The root of the source tree will be <tt>C:/Devel/stellarium</tt> for simplicity.

### Development versions from the Git repository

We use Git Bash as example. Check out the source by entering the folder where you want to have the stellarium source code.

The root of the source tree typically will be <tt>C:\Devel\stellarium</tt>.

You will first need to get the source code. 

`C:\Devel\stellarium> git clone https://github.com/Stellarium/stellarium`

Actually, This is enough for build. If you want lower size and faster building, then use below.

`C:\Devel\stellarium> git clone --depth 1 https://github.com/Stellarium/stellarium`

When the source code has been loaded into your <code>C:\Devel\stellarium\stellarium</code> folder you can add any updates again from the Git Bash command line.

`C:\Devel\stellarium\stellarium> git pull`

## Building with QtCreator

Qt5 comes with its own very elaborate IDE which depends on Microsoft's Visual Studio compiler. You must have some version of Visual Studio (the free version is enough), and use the fitting version of Qt5. 

Inside QtCreator, you open the CMakeLists.txt inside stellarium's source directory. Default settings create a debug build with all useful plugins. In the Projects tab (button in vertical left bar), you should at least configure Debug and Release builds.

For unclear reasons, QtCreator may not create the configured directories, you have to create them yourself. (Note the message in the "general output" tab in the bottom, when settings have been written to some temp folder, you should create the build directories yourself.)

(TODO: Some further settings!)

Do not forget to load the [Code Style File](http://stellarium.org/files/ide/stellarium-ide.xml) ([TAR.GZ](http://stellarium.org/files/ide/stellarium-ide.xml.tgz)) in Extras/Settings/C++/Coding style. 

## Building with VS2017

### CMake

* Choose directory with source code of Stellarium 
* Choose directory for building of Stellarium Configure project (e.g. you should choose version of compiler) 
* Add below options (You can add option by clicking Add Entry button)

<pre>Name Type Value</pre>
<pre>CMAKE_INSTALL_PREFIX  PATH  C:\stellarium </pre>
<pre>STELLARIUM_RELEASE_BUILD  BOOL  0(not checked)</pre>
<pre>ENABLE_MEDIA  BOOL  1</pre>
To build development snapshot and set specific version number
<pre>STELLARIUM_VERSION  STRING  0.14.73.0 </pre>
We use format XX.YY.ZZ.BB for development snapshots, where XX - current major stable version, YY - current minor stable version, ZZ - current patch version, BB - current build version. 

E.g. first snapshot for version 0.15.0 will be have a number 0.14.50.0. 


* After that, Click Configure and then Click Generate.

If CMake the compile fails with the error "can't find a particular file",
Check the CMakeCache.txt file in the ...\builds\msvc2017 folder and make sure the variable/path/file is directed to the correct source. 

### Visual Studio Build
* Click Open Project button on Cmake GUI or You can Excute <tt>C:\Devel\stellarium\builds\msvc2017\Stellarium.sln</tt>
* Change Build mode from Debug to Release
* Right click <tt>stellarium</tt> soultion among many solutions.
* Build
* If building is O.K. - run ''Build'' for the ''INSTALL'' solution

This will place all the compiled files in <tt>C:\stellarium</tt>. From where they will be found when you run the installer program <tt>stellarium.iss</tt> after you install the Inno Setup Compiler.

## Doing a test run
If the build was successful, you can run the program without installing it, just to test everything is OK.  
Make sure your folder like <code>C:\Qt\5.11.0\msvc2017_64\bin</code> is in PATH, so that the Qt DLLs will be found. Then add a link from the source directory to <code>&lt;build-dir&gt;\src\stellarium.exe</code> which executes inside the source directory.


## Making the installer

* First you need to find all the DLLs which Stellarium uses, and copy them to the root of the source tree(<tt>C:\Devel\stellarium\stellarium</tt>)
* From the <tt>\Qt\5.11.0\msvc2017\bin</tt> (or <tt>\Qt\5.11.0\msvc2017_64\bin</tt> for 64-bit libraries) folder: <tt>Qt5Core.dll</tt>, <tt>Qt5Concurrent.dll</tt>, <tt>Qt5Gui.dll</tt>, <tt>Qt5Network.dll</tt>, <tt>Qt5OpenGL.dll</tt>, <tt>Qt5Widgets.dll</tt>, <tt>Qt5Sql.dll</tt>, <tt>Qt5SerialPort.dll</tt>, <tt>Qt5PrintSupport.dll</tt>, <tt>Qt5XmlPatterns.dll</tt>, <tt>icudt54.dll</tt>, <tt>libEGL.dll</tt>, <tt>libGLESv2.dll</tt>, and <tt>d3dcompiler_47.dll</tt>;

* From the <tt>\Qt\5.11.0\msvc2017\plugins\platforms</tt> (or <tt>\Qt\5.11.0\msvc2017_64\plugins\platforms</tt> for 64-bit libraries) folder to the <tt>/platforms</tt> of the source tree

If you need a language other than English copy the <tt>stellarium\translations</tt> folder from <tt>C:\stellarium\share</tt> (or <tt>\program files\stellarium\share</tt> if you don't set <tt>CMAKE_INSTALL_PREFIX</tt> variable) to the stellarium source tree root folder.

This will place all the necessary files in your program files folder stellarium where the installer expects to find them.

We use the Inno Setup Compiler to create the installer. These details are for builds source later than 6052

* From http://www.jrsoftware.org download version 5.5.5 (unicode) or better, run it to install. 

If you have followed the above procedure the current git/cmake build will generate the necessary <tt>stellarium.iss</tt> file in <tt>C:\Devel\stellarium\builds\msvc2017</tt>. 
* Double click on it, then from the menu bar "build-compile". 
It will build the stellarium installer package and place it in a folder of the stellarium source tree root folder "installers". So you can find in <tt>C:\Devel\stellarium\stellarium\installers</tt>

Run the program generated and Stellarium will be installed in program files\stellarium or wherever you select and place an icon on the desktop.

# Stellarium on Raspberry Pi

The Raspberry Pi is a very popular family of single-board computers for many kinds of programming experiments and hardware tinkering projects. A new open-source graphics driver has been created by Eric Anholt, which enables you to run Stellarium with hardware acceleration on the multi-core models 2 and 3. Don't expect a super computer, but you should get typically over 10 frames per seconds, which should be enough for many applications where you don't want to waste too much energy. (E.g. solar/battery powered observatory.) I get 18fps on a 1400x1050 screen with Satellites not shown. 

For the current Raspbian ("Buster", 2019), just 
 - activate the "OpenGL Driver" in <code>raspi-config</code>, Advanced options, Full KMS. 
 - install the [build requirements](Linux-build-dependencies.md) and 
 - follow the [building instructions for Linux](Compilation-on-Linux.md).

For previous Raspbian ("Stretch" as of early 2018), you must 
 - activate the "Experimental OpenGL Driver" in <code>raspi-config</code>
 - build <code>libdrm</code> and <code>Mesa 17</code> from sources (see below)
 - install the [build requirements](Linux-build-dependencies.md) and 
 - follow the [building instructions for Linux](Compilation-on-Linux.md).

Alternatively, you can run Ubuntu Mate. On version 16.04.3 which has Mesa 17.0.7, I followed https://ubuntu-mate.community/t/tutorial-activate-opengl-driver-for-ubuntu-mate-16-04/7094 to activate VC4, and tried Stellarium 0.16.1 from our ppa. You don't have to compile anything yourself!

I (AF) found this preferable with Stretch 2018-0627 because Mesa would not configure for me.

The following seemed to work on Pi3B+ model 
http://www.raspberryconnect.com/gamessoftware/item/314-trying_out_opengl_on_raspberry_pi_3

2019-08: Ubuntu Mate 18.04.2 LTS also works. However, the arm64 version seems not yet polished. Stellarium shows nice 8-14fps in 1680x1050 but frequently crashes. 


## First Impressions, limitations
The new driver seems to work pretty well in many cases, but some hardware limitations persist.

* You cannot show your own landscapes or 3D sceneries with any textures larger than 2048x2048 pixels. Also the built-in "Sterngarten" scenery will not work. 
* The Scenery3D plugin should be used with Perspective Projection only. All other projections require too much texture memory. 
* It seems that some bug prevents OBJ planets from being rendered. Keep this option disabled!
* If you want higher framerate, consider disabling the Satellites plugin, or at least think about reducing number of satellites to the ones you are critically interested in. Also, don't load too many minor bodies.  
* The DSS layer seems to work for short periods, then the X server fails. It may have to do with limitations of texture memory. You can try to press Ctrl-Alt-Backspace to restart it, but sometimes you will have to reboot if the graphics failed.

***

## Building the external libraries

The following instructions which are required for Raspbian "Stretch" have been taken from Eric Anholt's site (https://github.com/anholt/mesa/wiki/VC4-complete-Raspbian-upgrade) on February 24, 2018. Fortunately we don't need to build a new kernel!  


### libdrm

Until Raspbian gets its libdrm updated, we need to build it ourselves.

    sudo apt-get install \
        xutils-dev libpthread-stubs0-dev \
        automake autoconf libtool git
    git clone git://anongit.freedesktop.org/mesa/drm
    cd drm
    ./autogen.sh \
        --prefix=/usr \
        --libdir=/usr/lib/arm-linux-gnueabihf
    make
    sudo make install

### Mesa
    sudo apt-get install \
        flex bison python-mako \
        libxcb-dri3-dev libxcb-dri2-0-dev \
        libxcb-glx0-dev libx11-xcb-dev \
        libxcb-present-dev libxcb-sync-dev \
        libxshmfence-dev \
        libxdamage-dev libxext-dev libxfixes-dev \
        x11proto-dri2-dev x11proto-dri3-dev \
        x11proto-present-dev x11proto-gl-dev \
        libexpat1-dev libudev-dev gettext \
        libxrandr-dev
    
    git clone git://anongit.freedesktop.org/mesa/mesa
    cd mesa
    ./autogen.sh \
        --prefix=/usr \
        --libdir=/usr/lib/arm-linux-gnueabihf \
        --with-gallium-drivers=vc4 \
        --with-dri-drivers= \
        --with-egl-platforms=x11,drm
    make
    sudo make install

## Notes for building Stellarium

You may want to install a swap file, otherwise you may run out of virtual memory when compiling
[https://www.cyberciti.biz/faq/linux-add-a-swap-file-howto/](https://www.cyberciti.biz/faq/linux-add-a-swap-file-howto/)

Then, make sure to install the [build requirements](Linux-build-dependencies.md) and follow the [building instructions for Linux](Compilation-on-Linux.md).

If you are doing the compile on your Pi, follow the Linux instructions but use the command
<pre>
nice make -j4
</pre>
Otherwise, you may lose control of the pi with all cores busy. Or run with 
<pre>
make -j3
</pre>
to keep one core free for other tasks.

## Raspberry Pi Model 4B

2019-10-10: Stellarium works great out of the box. Install Raspbian, clone the GIT repo, install Linux build dependencies, and 

    mkdir build; cd build
    cmake -DCMAKE_BUILD_TYPE="Release" ../stellarium
    make -j4 

On a model with 4GB you don't need a larger swap file. Within 16 minutes, Stellarium was built, with about 17fps at FullHD resolution. Without satellites, 22fps. 

2020-01-29:
On an aarch64 build, we found (#940) you need to convince MESA to run without issues:

`MESA_GL_VERSION_OVERRIDE=3.0 MESA_GLSL_VERSION_OVERRIDE=130 stellarium`

# Testing

There are several test programs in the repository. To build them, define `-DCMAKE_ENABLE_TESTING=ON`, or configure cmake in QCreator's Projects tab.

Then configure a Debug build and select a test... application to be executed.

Please try to test your changes before committing to master. Our automated Travis builds will signal failure when tests don't complete.

