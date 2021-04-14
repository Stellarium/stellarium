# Building Stellarium

Hello, and thank you for your interest to Stellarium! 

If you want to test a prebuilt beta version, take a look at https://github.com/Stellarium/stellarium-data/releases/tag/weekly-snapshot

## Why build from source?

Each time Stellarium is released, the source code is released in Github's 
packaging system. Building source code that is released in this way 
should give you a working copy of Stellarium which is functionally 
identical to the binaries for that release.

It is also possible to get the source code "in development" using Git. 
This may contain new features or bugfixes which have been implemented since the last 
release of Stellarium, so it's often more fun.

**Warning:** Git versions of the Stellarium source code are work in progress, 
and as such may produce an unstable program, may not work at all, or may 
(rarely) not even compile.

## Integrated Development Environment (IDE)

If you plan to develop Stellarium, it is highly recommended to utilize 
an IDE. You can use any IDE of your choice, but QtCreator is recommended 
as best suited for Qt development.

Inside QtCreator, you open the `CMakeLists.txt` inside Stellarium's source 
directory. Default settings create a debug build with all useful plugins. 
In the Projects tab (button in vertical left bar), you should at least 
configure Debug and Release builds.

Do not forget to load the [Code Style File](https://stellarium.org/files/ide/stellarium-ide.xml) ([TAR.GZ](https://stellarium.org/files/ide/stellarium-ide.xml.tgz)) in Extras/Settings/C++/Coding style.

## Prerequisite Packages

To build and develop Stellarium, several packages may be required from 
your distribution. Here's a list.

### Required dependencies

- A C++ compiler able to compile C++11 code ([GCC](https://gcc.gnu.org/) 4.8.1 or later, Clang 3.3 or later, MSVC 2015 or later)
- [CMake](http://www.cmake.org/) 2.8.12 or later - buildsystem used by many open source projects
- [Qt Framework](http://www.qt.io/) 5.7.0 or later
- [OpenGL](https://www.opengl.org/) - graphics library
- [Zlib](http://www.zlib.net) - compression library

### Optional dependencies

- [Git](https://git-scm.com) - required for obtaining latest changes in source code
- [gettext](https://www.gnu.org/software/gettext/) - required for developers for extract of lines for translation
- [Doxygen](http://doxygen.org/) - if you want to build the API documentation you will need this
- [Graphviz](http://www.graphviz.org/) - required to build the API documentation and include fancy class diagrams
- [libgps](https://gpsd.gitlab.io/gpsd/index.html) - if you want to build Stellarium with GPS support (Linux/macOS only)

### Installing these packages

To install all of these, use the following commands:

#### Debian / Ubuntu

```
sudo apt install build-essential cmake zlib1g-dev libgl1-mesa-dev libdrm-dev gcc g++ \
                 graphviz doxygen gettext git \
                 qtbase5-dev qtscript5-dev libqt5svg5-dev qttools5-dev-tools qttools5-dev libqt5opengl5-dev \
                 qtmultimedia5-dev libqt5multimedia5-plugins libqt5serialport5 libqt5serialport5-dev qtpositioning5-dev \
                 libgps-dev libqt5positioning5 libqt5positioning5-plugins
```

#### Fedora / CentOS

```
sudo yum install cmake gcc graphviz doxygen gettext git \
                 qt5-base-devel qt5-qttools-devel qt5-qtscript-devel qt5-qtsvg-devel qt5-qtmultimedia-devel \
                 qt5-qtserialport-devel qt5-qtlocation-devel qt5-qtpositioning-devel
```

#### Linux with outdated Qt

Stellarium tracks the recent Qt releases fairly closely and as such many Linux distribution repositories do not contain an up-to-date enough version for building Stellarium. In the case of Ubuntu, the ''backports'' repository is often good enough, but there may be a need to install it "outside" your package manager. Here's how.

The Qt development team provides binary installers. If you want to build Qt yourself from source, this is fine but it will take a ''long'' time. We recommend the following procedure for manually installing the latest Qt (required: 5.7 or above at the moment):
- Download the Linux/X11 package from [Qt Company](http://www.qt.io/download-open-source/). Choose 32/64 bit as appropriate.
- Install it to `/opt/Qt5`
- When you want to build Stellarium, execute these commands to set up the environment so that the new Qt is used (for 64-bit package):
```
export QTDIR=/opt/Qt5/5.12.2/gcc_64
export PATH=$QTDIR/bin:$PATH
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
```
- After installation, you should write a script which sets `LD_LIBRARY_PATH` and then calls Stellarium:
```
#!/bin/sh
export QTDIR=/opt/Qt5/5.12.2/gcc_64
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
./stellarium
```

#### MacOS

- Install the latest version of [Apple's Developer Tools](https://developer.apple.com/xcode/). 
- Install [Homebrew](https://brew.sh/). 
- Install required packages:
```
$ brew install cmake git gettext
$ brew link gettext --force
```

#### OSX 11 and above
If 
```
$ brew link gettext --force
```

failed due to :
```
Linking /usr/local/Cellar/gettext/0.21...
Error: Could not symlink include/autosprintf.h
/usr/local/include is not writable.
```
Try the following:

```
$ sudo mkdir /usr/local/include
$ sudo chown -R $(whoami) $(brew --prefix)/*

```

- Install latest Qt:
```
$ brew install qt
```
- Add Qt to your PATH environment variable, adding to your `.bash_profile` file the following line:
```
export PATH=/usr/local/opt/qt/bin:$PATH
```

You may using the distribution from the Qt Company to install the [latest stable version](https://www.qt.io/download-qt-installer) of Qt. In this case adding Qt to your PATH environment variable will to adding to your `.bash_profile` file the following line (for example we installed Qt 5.12.2):
```
export PATH=~/Qt/5.12/clang_64/bin:$PATH
```

#### Windows

- Install the [Microsoft Visual Studio Community 2017](https://visualstudio.microsoft.com/thank-you-downloading-visual-studio/?sku=Community&rel=15) or [Microsoft Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) (or "better" - e.g. Professional) from Microsoft Website. Qt5.15 requires MSVC2019.
- To get the source code of Stellarium you need to install some git environment. [Git for Windows](https://git-scm.com/download/win) seems ok, or the Git Bash and Git GUI, whatever seems suitable for you. But it is not necessary.
- Get the [latest version of Qt from Qt Company](http://www.qt.io/download-open-source/). We recommend to use Qt 5.9 or later.  You must select Qt Script and msvc2017/msvc2019 among so many checkboxes.

After installing all required libraries and tools you should configure the build environment.

Add `C:\Qt\Qt5.15.2` to your `PATH` variable - you should add string `C:\Qt\Qt5.15.2\msvc2019;C:\Qt\Qt5.15.2\msvc2019\bin` for 32-bit or `C:\Qt\Qt5.15.2\msvc2019_64;C:\Qt\Qt5.15.2\msvc2019_64\bin` for 64-bit to `PATH` variable.
(Replace the version numbers of Qt and the version of Visual Studio (2017/2019) with the version that you have installed)

**Known limitations with Qt 5.15.x:**
- The ANGLE library should be taken from Qt 5.6, all later versions don't work and can be downloaded for [x64](https://github.com/Stellarium/stellarium-data/releases/download/qt-5.6/libGLES-x64.zip) and [x32](https://github.com/Stellarium/stellarium-data/releases/download/qt-5.6/libGLES-Win32.zip). (Don't ask us why. Find a solution!)
- Qt 5.15.0 and 5.15.1 have a buggy `lconvert` and should not be used. Also `lconvert` on Qt 5.15.2 still allocates GBs of memory for translation of a few MBs of strings, if it can get it.

**Note:** After changes to the `PATH` variable you should reboot the computer to apply those changes.

#### Windows (static)

You can build a static version using MSVC-static kit (for example we installed Qt 5.15.2 with MSVC2019):

To prepare a static kit, prepare src package of Qt 5.15.2, and configure compilation tool (Python, Ruby, Perl and Visual Studio 2019). Enter src folder:

```
configure.bat -static -prefix "D:\Qt\msvc2019_static" -confirm-license -opensource  -debug-and-release -platform win32-msvc  -nomake examples -nomake tests  -plugin-sql-sqlite -plugin-sql-odbc -qt-zlib -qt-libpng -qt-libjpeg -opengl desktop -mp
nmake
nmake install
```

When finishing compilation, configure kit in Qt Creator. Clone Kit "Desktop Qt5.15.2 MSVC" to "Desktop Qt5.15.2 MSVC(static)". Then configure CMake Generator with NMake Makefiles JOM + Extra generator: CodeBlocks.

Finally, just open CMakeLists.txt in Qt Creator and build it with MSVC-static kit.

## Getting the source code

We recommend using a copy of our git repository to build your own installation 
as it contains some dependencies required for building.

### Extract the tarball or ZIP containing the source code

You can find the source code from

```
https://github.com/Stellarium/stellarium/releases
```

Do this command in a terminal (if you prefer, you might use arK or some other graphical archive tool): 

```
$ tar zxf stellarium-0.20.2.tar.gz
```
You should now have a directory `stellarium-0.20.2` with the source code in it.


### Clone project from GitHub

To create the copy install git from your OS distribution repository or from 
https://git-scm.com/.

The git repository has become quite large (about 2GB). You do not need the complete 
history to build or continue development, but can try a *blobless clone* 
(https://github.blog/2020-12-21-get-up-to-speed-with-partial-clone-and-shallow-clone/)

```
$ git clone --filter=blob:none https://github.com/Stellarium/stellarium.git
$ cd stellarium
```

Else, to get the full repository, execute the following commands:

```
$ git clone https://github.com/Stellarium/stellarium.git
$ cd stellarium
```

### Download source code from GitHub

You can [download](https://github.com/Stellarium/stellarium/archive/master.zip) fresh source code from GitHub by web.

#### Windows specifics

On Windows save the file (`master.zip` or `stellarium-0.20.2.tar.gz`) to the `C:/Devel` directory as example. You will need 
a decompression program installed in Windows, for example [7-Zip](http://www.7-zip.org/). 
The root of the source tree will be `C:/Devel/stellarium` for simplicity.

## Building

Assuming you have collected all the necessary libraries, here's
what you need to do to build and run Stellarium:

### On Linux
```
$ mkdir -p build/unix
$ cd build/unix
$ cmake -DCMAKE_INSTALL_PREFIX=/opt/stellarium ../.. 
$ make -jN
```

### On macOS
```
$ mkdir -p build/macosx
$ cd build/macosx
$ cmake ../.. 
$ make -jN
```

### On Windows
```
$ md build
$ cd build
$ md msvc
$ cd msvc
$ cmake -DCMAKE_INSTALL_PREFIX=c:\stellarium-bin -G "Visual Studio 16 2019" ../..
$ cmake --build . --  /maxcpucount:N /nologo
```

For Visual Studio 2017:
```
$ cmake -DCMAKE_INSTALL_PREFIX=c:\stellarium-bin -G "Visual Studio 15 2017 Win64" ../..
```

Instead of `N` in `-j` (`N` in `/maxcpucount`) pass a number of CPU cores you want to use during
a build.

If you have Qt5 installed using official Qt installer, then pass parameter
`CMAKE_PREFIX_PATH` to cmake call used to configure Stellarium, e.g.

```
$ cmake -DCMAKE_PREFIX_PATH=/opt/Qt5 ../..
```

When you are using QtCreator IDE, build directories are created by the IDE. It appears that on Windows, a directory name is proposed, but you have to create it manually. 

You can keep your copy up-to-date by typing `git pull --rebase` in ~/stellarium. 
Feel free to send patches to our mailing list stellarium@googlegroups.com

### Supported CMake parameters

List of supported parameters (passed as `-DPARAMETER=VALUE`):

 Parameter                      | TYPE   | Default | Description
--------------------------------| -------|---------|-----------------------------------------------------
| CMAKE_INSTALL_PREFIX          | path   | *       | Prefix where to install Stellarium
| CMAKE_PREFIX_PATH             | path   |         | Additional path to look for libraries
| CMAKE_BUILD_TYPE              | string | Release | Build type of Stellarium
| ENABLE_NLS                    | bool   | ON      | Enable interface translation
| ENABLE_GPS                    | bool   | ON      | Enable GPS support
| ENABLE_LIBGPS                 | bool   | ON      | Enable GPS support with libGPS library (N/A on Windows)
| ENABLE_MEDIA                  | bool   | ON      | Enable sound and video support
| ENABLE_SCRIPTING              | bool   | ON      | Enable the scripting feature
| ENABLE_RELEASE_BUILD          | bool   | OFF     | This option flags the build as an official release
| ENABLE_TESTING                | bool   | OFF     | Enable unit tests
| USE_PLUGIN_ANGLEMEASURE       | bool   | ON      | Enable building the Angle Measure plugin
| USE_PLUGIN_COMPASSMARKS       | bool   | ON      | Enable building the Compass Marks plugin
| USE_PLUGIN_SATELLITES         | bool   | ON      | Enable building the Satellites plugin
| USE_PLUGIN_TELESCOPECONTROL   | bool   | ON      | Enable building the Telescope Control plugin
| USE_PLUGIN_OCULARS            | bool   | ON      | Enable building the Oculars plugin
| USE_PLUGIN_TEXTUSERINTERFACE  | bool   | ON      | Enable building the Text User Interface plugin
| USE_PLUGIN_SOLARSYSTEMEDITOR  | bool   | ON      | Enable building the Solar System Editor plugin
| USE_PLUGIN_SUPERNOVAE         | bool   | ON      | Enable building the Historical Supernovae plugin
| USE_PLUGIN_QUASARS            | bool   | ON      | Enable building the Quasars plugin
| USE_PLUGIN_PULSARS            | bool   | ON      | Enable building the Pulsars plugin
| USE_PLUGIN_EXOPLANETS         | bool   | ON      | Enable building the Exoplanets plugin
| USE_PLUGIN_OBSERVABILITY      | bool   | ON      | Enable building the Observability Analysis plugin
| USE_PLUGIN_ARCHAEOLINES       | bool   | ON      | Enable building the Archaeo Lines plugin
| USE_PLUGIN_EQUATIONOFTIME     | bool   | ON      | Enable building the Equation Of Time plugin
| USE_PLUGIN_METEORSHOWERS      | bool   | ON      | Enable building the Meteor Showers plugin
| USE_PLUGIN_NAVSTARS           | bool   | ON      | Enable building the Navigational Stars plugin
| USE_PLUGIN_NOVAE              | bool   | ON      | Enable building the Bright Novae plugin
| USE_PLUGIN_POINTERCOORDINATES | bool   | ON      | Enable building the Pointer Coordinates plugin
| USE_PLUGIN_SCENERY3D          | bool   | ON      | Enable building the 3D Scenery plugin
| USE_PLUGIN_REMOTECONTROL      | bool   | ON      | Enable building the Remote Control plugin
| USE_PLUGIN_REMOTESYNC         | bool   | ON      | Enable building the Remote Sync plugin

Notes:
 \* `/usr/local` on Unix-like systems, `C:\Program Files` or `C:\Program Files (x86)`
   on Windows depending on OS type (32 or 64 bit) and build configuration.

## Code testing

There are several test programs in the repository. To build them, define `-DENABLE_TESTING=ON` (or `-DENABLE_TESTING=1`), or configure cmake in QtCreator's Projects tab.

Then configure a Debug build and select a test... application to be executed.

Please try to test your changes before committing to master. Our automated [Travis](https://travis-ci.org/github/Stellarium/stellarium) and 
[AppVeyor](https://ci.appveyor.com/project/alex-w/stellarium) builds will signal failure when tests don't complete.

To execute all unit tests in terminal please run:
```
$ make test
```
or 
```
$ ctest --output-on-failure
```

## Packaging

OK, you have built the program from source code and now you may want to install the executable file into your operating system or create a package for distribution.

To install the executable file (with necessary libraries and data files) into the directory defined in parameter `CMAKE_INSTALL_PREFIX`, run:

```
$ sudo make install
```

### Linux specifics

To create a source package (TGZ) on linux you need run:
```
$ make package_source
```

To create a binary package (TGZ) on linux you need run:
```
$ make package
```

After building of TGZ binary package you may create a DEB or RPM package also: 
```
$ cpack -G DEB
```
or 
```
$ cpack -G RPM
```

### macOS specifics

**IMPORTANT**: you should delete or move aside the old `Stellarium.app` before each new build:
```
$ rm -r Stellarium.app
```

Then build the macOS application:
```
$ make install
```

You'll find now an application `Stellarium.app` with the correct icon in the build directory.

To create the DMG file (Apple Disk Image) run:
```
$ mkdir Stellarium
$ cp -r Stellarium.app Stellarium
$ hdiutil create -format UDZO -srcfolder Stellarium Stellarium.dmg
```

### Windows specifics

To create a Windows installer you need to have installed [Inno Setup](http://www.jrsoftware.org/).

If you have followed all the above procedures the current build will generate the necessary `stellarium.iss` file in `C:\Devel\stellarium\builds\msvc`.

Double click on it, then from the menu bar "build-compile". It will build the stellarium installer package and place it in a folder of the stellarium source tree root folder `installers`. So you can find it in `C:\Devel\stellarium\stellarium\installers`

Or you can use cmake command for create an installer:
```
$ cmake --build c:\devel\stellarium\build\msvc --target stellarium-installer
```

### Supported make targets

Make groups various tasks as "targets". Starting make without any arguments causes make to build the default target - in our case, building Stellarium, its tests, the localization files, etc.
 Target          | Description
-----------------|----------------------------------------------------------------------------------------------------
| install        | Installation of all binaries and related files to the directory determined by `CMAKE_INSTALL_PREFIX`
| test           | Launch the suite of test executables
| apidoc         | Generate an API documentation
| package_source | Create a source package for distributions
| package        | Create a binary packages for distributions on linux/UNIX
| installer      | Create a binary packages for distributions on Windows

Thanks!

\- *The Stellarium development team*
