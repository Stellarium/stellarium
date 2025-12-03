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

Do not forget to load the [Code Style File](doc/stellarium-ide.xml) in
Extras/Settings/C++/Coding style (Import... button).

## Prerequisite Packages

To build and develop Stellarium, several packages may be required from 
your distribution. Here's a list.

### Required dependencies

- A C++ compiler able to compile C++17 code ([GCC](https://gcc.gnu.org/) 7 or later, 
  Clang 6 or later, MSVC 2017 (15.7) or later; MSVC2019 required for Qt6)
- [CMake](https://www.cmake.org/) 3.16.0 or later - buildsystem used by many open source projects
- [Qt Framework](https://www.qt.io/) 5.12.0/6.2.0 or later. We recommend 5.15.2 or 6.5.1
- [OpenGL](https://www.opengl.org/) - graphics library
- [Zlib](https://www.zlib.net) - compression library

### Dependencies

### Optional dependencies

- [Git](https://git-scm.com) - required for obtaining latest changes in source code
- [gettext](https://www.gnu.org/software/gettext/) - required for developers for extract of lines for translation
- [Doxygen](http://doxygen.org/) - if you want to build the API documentation you will need this
- [Graphviz](http://www.graphviz.org/) - required to build the API documentation and include fancy class diagrams
- [libgps](https://gpsd.gitlab.io/gpsd/index.html) - if you want to build Stellarium with GPS support (Linux/macOS only)

### Optionally bundled dependencies

If these are not found on the system, they are downloaded automatically.
See [MAINTAINER BUSINESS](MAINTAINER_BUSINESS.md) for details.

- [INDI](https://indilib.org)
- [QXlsx](https://github.com/QtExcel/QXlsx)
- [ShowMySky](https://10110111.github.io/CalcMySky/), can be disabled with
  `-DENABLE_SHOWMYSKY=OFF` CMake parameter. If enabled (the default), it also requires `libglm-dev libeigen3-dev`.

### Manually Download Dependencies (CPM)

If you are unable to download dependencies automatically due to network restrictions, you can manually fetch them. Dependencies are usually listed with URLs like `URL https://github.com/...` in all `CMakeLists.txt` files using `CPMAddPackage`, `CPMFindPackage`, etc.

After downloading, place them in the following directory structure (example for `QXlsxQt6` in Windows):

```
<build_dir>/_deps/qxlsxqt6-subbuild/qxlsxqt6-populate-prefix/src/v1.5.0.tar.gz
```

> The filename must match the URL exactly, and the directory name is the package name in lowercase.

You can also write a script to automate this process.

### Installing these packages

To install all of these, use the following commands:

#### Debian / Ubuntu

##### Qt5

```
sudo apt install build-essential cmake zlib1g-dev libgl1-mesa-dev libdrm-dev gcc g++ \
                 graphviz doxygen gettext git libgps-dev libqt5qxlsx-dev \
                 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-pulseaudio \
                 gstreamer1.0-libav gstreamer1.0-vaapi qtbase5-dev \
                 qtbase5-private-dev qtscript5-dev libqt5svg5-dev qttools5-dev-tools \
                 qttools5-dev libqt5opengl5-dev qtmultimedia5-dev libqt5multimedia5-plugins \
                 libqt5serialport5 libqt5serialport5-dev qtpositioning5-dev libqt5positioning5 \
                 libqt5positioning5-plugins qtwebengine5-dev libqt5charts5-dev \
                 libexiv2-dev libnlopt-cxx-dev libtbb-dev libtbb2 libqt5concurrent5 \
                 libmd4c-dev libmd4c-html0-dev qt5-image-formats-plugins
```

##### Qt6

Ubuntu 22.04 comes with Qt5.15 and Qt6.2. To build with Qt6:

```
sudo apt install build-essential cmake zlib1g-dev libgl1-mesa-dev libdrm-dev libglx-dev \
                 gcc g++ graphviz doxygen gettext git libxkbcommon-x11-dev libgps-dev \
                 gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-pulseaudio \
                 gstreamer1.0-libav gstreamer1.0-vaapi qt6-image-formats-plugins \
                 qt6-base-private-dev qt6-multimedia-dev qt6-positioning-dev qt6-tools-dev \
                 qt6-tools-dev-tools qt6-base-dev-tools qt6-qpa-plugins qt6-image-formats-plugins \
                 qt6-l10n-tools qt6-webengine-dev qt6-webengine-dev-tools libqt6charts6-dev \
                 libqt6charts6 libqt6opengl6-dev libqt6positioning6-plugins libqt6serialport6-dev \
                 qt6-base-dev libqt6webenginecore6-bin libqt6webengine6-data \
                 libexiv2-dev libnlopt-cxx-dev libqt6concurrent6 libmd4c-dev libmd4c-html0-dev
```


#### Fedora / CentOS / AlmaLinux / Rocky Linux

Note: This should work on RHEL/CentOS/AlmaLinux/Rocky Linux 8 or later and recent versions of Fedora. To build on CentOS 7 or older,
see [Linux with outdated Qt](#linux-with-outdated-qt).

```
sudo dnf install cmake gcc graphviz doxygen gettext git \
                 qt5-qtbase-devel qt5-qtbase-private-devel qt5-qttools-devel qt5-qtscript-devel \
                 qt5-qtsvg-devel qt5-qtmultimedia-devel qt5-qtserialport-devel qt5-qtlocation-devel \
                 qt5-qtcharts-devel qt5-qtwebengine-devel exiv2-devel
```

#### Linux with outdated Qt

Stellarium tracks the recent Qt releases fairly closely and as such many Linux distribution repositories do 
not contain an up-to-date enough version for building Stellarium. In the case of Ubuntu, the ''backports'' 
repository is often good enough, but there may be a need to install it "outside" your package manager. 
Here's how.

The Qt development team provides binary installers. If you want to build Qt yourself from source, this is 
fine but it will take a ''long'' time. We recommend the following procedure for manually installing the 
latest Qt (required: 5.12 or above at the moment):
- Download the Linux/X11 package from [Qt Company](http://www.qt.io/download-open-source/). Choose 32/64 bit 
  as appropriate.
- Install it to `/opt/Qt5`
- When you want to build Stellarium, execute these commands to set up the environment so that the new Qt 
  is used (for 64-bit package):
```
export QTDIR=/opt/Qt5/5.12.12/gcc_64
export PATH=$QTDIR/bin:$PATH
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
```
- After installation, you should write a script which sets `LD_LIBRARY_PATH` and then calls Stellarium:
```
#!/bin/sh
export QTDIR=/opt/Qt5/5.12.12/gcc_64
export LD_LIBRARY_PATH=$QTDIR/lib:$LD_LIBRARY_PATH
./stellarium
```

#### Linux without QtWebEngine

On some distributions (known for ARM systems, like Raspberry OS (Raspbian)) there is no QtWebEngine. The 
CMake script will check installed `qtwebengine5` package and if it is not found emits a warning, but 
Stellarium will be built without QtWebEngine support. The result is shown in the system web browser.

#### MacOS

- Install the latest version of [Apple's Developer Tools](https://developer.apple.com/xcode/). 
- Install [Homebrew](https://brew.sh/). 
- Install required packages:
  ```
  brew install cmake git gettext
  brew link gettext --force
  ```

  On MacOS 11 and above, if
  ```
  brew link gettext --force
  ```

  failed due to :
  ```
  Linking /usr/local/Cellar/gettext/0.21...
  Error: Could not symlink include/autosprintf.h
  /usr/local/include is not writable.
  ```
  Try the following:

  ```
  sudo mkdir /usr/local/include
  sudo chown -R $(whoami) $(brew --prefix)/*

  ```

- Install latest Qt 5:
  ```
  brew install qt@5
  ```
- Add Qt to your PATH environment variable  
  Intel Mac: add the following line to `~/.bash_profile` (Bash) or `~/.zprofile` (Zsh):
  ```
  export PATH=/usr/local/opt/qt@5/bin:$PATH
  ```
  ARM-based (Apple Silicon) Mac: add the following line to `~/.bash_profile` (Bash) or `~/.zprofile` (Zsh):
  ```
  export PATH=/opt/homebrew/opt/qt@5/bin:$PATH
  ```

You may using the distribution from the Qt Company to install the [latest stable version](https://www.qt.io/download-qt-installer)
of Qt. In this case adding Qt to your PATH environment variable will to adding to your `~/.bash_profile` (Bash) or `~/.zprofile` (Zsh)
file the following line (for example we installed Qt 5.12.12):
```
export PATH=~/Qt/5.12/clang_64/bin:$PATH
```

#### MacOS without QtWebEngine

On ARM-based (Apple Silicon) Macs, there is no support for QtWebEngine or it buggy (QtWebEngine in Qt 5.15.2). 
The CMake script will check installed `QtWebEngine` libraries and if then is not in system then 
Stellarium will build without QtWebEngine support. The result is shown in the system web browser.

#### Windows

- Install the [Microsoft Visual Studio Community 2019 or 2022](https://visualstudio.microsoft.com/downloads/) 
  (or "better" - e.g. Professional) from Microsoft Website. Qt 5.15 requires MSVC2019.
- To get the source code of Stellarium you need to install some git environment. 
  [Git for Windows](https://git-scm.com/download/win) seems ok, or the Git Bash and Git GUI, whatever 
  seems suitable for you. But it is not necessary.
- Get the [latest version of Qt from Qt Company](http://www.qt.io/download-open-source/). We recommend 
  to use Qt 5.15.2 or, better, Qt6. For Qt5 you must select Qt Script and msvc2019 among so many checkboxes.

After installing all required libraries and tools you should configure the build environment.

Add `C:\Qt\Qt5.15.2` to your `PATH` variable - you should add string `C:\Qt\Qt5.15.2\msvc2019;C:\Qt\Qt5.15.2\msvc2019\bin` 
for 32-bit or `C:\Qt\Qt5.15.2\msvc2019_64;C:\Qt\Qt5.15.2\msvc2019_64\bin` for 64-bit to `PATH` variable.
(Replace the version numbers of Qt and the version of Visual Studio (2017/2019) with the version that you 
have installed)
If you also want to run the ShowMySky sky model, add another directory to the PATH variable. This depends on your build environment. 
If builds are made into `D:\StelDev\GIT\build-stellarium-Desktop_Qt_6_5_1_MSVC2019_64bit-Release\`, this would be 
`D:\StelDev\GIT\build-stellarium-Desktop_Qt_6_5_1_MSVC2019_64bit-Release\_deps\showmysky-qt6-build\ShowMySky`

**ANGLE issues:**

- The ANGLE library for Qt5-based builds should be taken from Qt 5.6 (all later versions don't work) and can be downloaded 
- for [x64](https://github.com/Stellarium/stellarium-data/releases/download/qt-5.6/libGLES-x64.zip) 
- and [x32](https://github.com/Stellarium/stellarium-data/releases/download/qt-5.6/libGLES-Win32.zip). 
- (Don't ask us why. Find a solution!)

**WSL: libQt5Core.so.5 not found**

Fresh installations of WSL may have issues not finding libQt5Core.so.5. Run
```
sudo strip --remove-section=.note.ABI-tag /usr/lib/x86_64-linux-gnu/libQt5Core.so.5
```
(https://superuser.com/questions/1347723/arch-on-wsl-libqt5core-so-5-not-found-despite-being-installed)

**Known limitations with Qt 5.15.x:**

- Qt 5.15.0 and 5.15.1 have a buggy `lconvert` and should not be used. Also `lconvert` on Qt 5.15.2 
still allocates GBs of memory for translation of a few MBs of strings, if it can get it.

**Note:** After changes to the `PATH` variable you should reboot the computer to apply those changes.

#### Windows (static)

You can build a static version using MSVC-static kit (for example we installed Qt 5.15.12 with MSVC2019):

To prepare a static kit, prepare src package of Qt 5.15.12, and configure compilation tool (Python, Ruby, 
Perl and Visual Studio 2019). Enter src folder:

```
configure.bat -static -prefix "D:\Qt\msvc2019_static" -confirm-license -opensource  -debug-and-release -platform win32-msvc  -nomake examples -nomake tests  -plugin-sql-sqlite -plugin-sql-odbc -qt-zlib -qt-libpng -qt-libjpeg -opengl desktop -mp
nmake
nmake install
```

When finishing compilation, configure kit in Qt Creator. Clone Kit "Desktop Qt 5.15.12 MSVC" to "Desktop Qt 
5.15.12 MSVC (static)". Then configure CMake Generator with NMake Makefiles JOM + Extra generator: CodeBlocks.

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
$ tar zxf stellarium-25.3.tar.gz
```
You should now have a directory `stellarium-25.3` with the source code in it.


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

If you ever want to contribute from a Windows environment, you *must* configure git to use Unix-style line endings. 
(The --global applies to all projects.)
(https://docs.github.com/en/get-started/getting-started-with-git/configuring-git-to-handle-line-endings)

```
$ git config [--global] core.autocrlf true
```

### Download source code from GitHub

You can [download](https://github.com/Stellarium/stellarium/archive/master.zip) fresh source code from 
GitHub by web.

#### Windows specifics

On Windows save the file (`master.zip` or `stellarium-25.3.tar.gz`) to the `C:/Devel` directory as 
example. You will need a decompression program installed in Windows, for example [7-Zip](http://www.7-zip.org/). 
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

When you are using QtCreator IDE, build directories are created by the IDE. It appears that on Windows, 
a directory name is proposed, but you have to create it manually. 

You can keep your copy up-to-date by typing `git pull --rebase` in ~/stellarium. 
Feel free to send patches to our mailing list stellarium@googlegroups.com

#### For Visual Studio 2022 (multi-configuration):

Select a working directory <work_dir> close to the drive root, e.g <work_dir> = "E:\Dev"
**Note** : VS 2022 will generate very long path for some sub-directory, so keep <work_dir> short to avoid 
exceeding the limit of 260 characters.

Download Stellarium from GitHub into : <stel_dir> = <work_dir> + "\stellarium"

Create a CMakePresets.json file and save the file in <stel_dir>. This file must be in the same directory 
as the top level CMakeLists.txt of Stellarium. The CMakePresets.json file defines all required build 
configurations (e.g. Debug, Release, RelWithDebInfo). It shall be structured for a multi-configuration
build process (see example below). However, it must be customized to your rig and software setup. 

Here is an example that was created to build Stellarium using Qt 6.7.3 framework. CMake parameters (-D flags) 
are defined within the "cacheVariables" section. Modify/Add your own as required. Some parameters were added 
to indicate to VS 2022 where to find some libraries (.lib) or include files (.hpp), those are specific to this
example and may not apply to your setup.

<CMakePresets.json file example>

{
  "version": 3,
  "cmakeMinimumRequired": { "major": 3, "minor": 21, "patch": 0 },
  "configurePresets": [
    {
      "name": "vs2022-multi-config",
      "displayName": "Stellarium (VS2022 Multi-Config)",
      "description": "Unified build tree for Debug, Release, and RelWithDebInfo using Visual Studio 2022",
      "generator": "Visual Studio 17 2022",
      "architecture": {
        "value": "x64"
      },
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_CONFIGURATION_TYPES": "Debug;Release;RelWithDebInfo",
	    "CMAKE_BUILD_TYPE": "Debug",
	    "CMAKE_SUPPRESS_DEVELOPER_WARNINGS": "1",
	    "CMAKE_PREFIX_PATH": "C:/Qt/6.7.3/msvc2022_64;C:/Dev/Libs/exiv2-0.28.7/lib/cmake/exiv2",
        "QT_QMAKE_EXECUTABLE": "C:/Qt/6.7.3/msvc2022_64/bin/qmake.exe",
        "ENABLE_SCRIPTING": "ON",
        "ENABLE_GPS": "ON",
        "ENABLE_TESTING": "ON",
        "ENABLE_NLS": "ON",
        "SCM_SHOULD_ENABLE_CONVERTER": "TRUE",
        "GETTEXTPO_LIBRARY": "C:/Dev/Libs/gettextpo/lib/libgettextpo.lib",
        "GETTEXTPO_INCLUDE_DIR": "C:/Dev/Libs/gettextpo/include",
        "LIBTIDY_LIBRARY": "C:/Dev/Libs/libtidy/lib/libtidy.lib",
        "LIBTIDY_INCLUDE_DIR": "C:/Dev/Libs/libtidy/include",
        "EXIV2_LIBRARY": "C:/Dev/Libs/exiv2-0.28.7/lib/exiv2.lib",
        "EXIV2_INCLUDE_DIR": "C:/Dev/Libs/exiv2-0.28.7/include",
        "exiv2_DIR": "C:/Dev/Libs/exiv2-0.28.7/lib/cmake/exiv2",
        "Qt6LinguistTools_DIR": "C:/Qt/6.7.3/msvc2022_64/lib/cmake/Qt6LinguistTools"
      }
    }
  ],
  "buildPresets": [
  {
    "name": "Debug",
    "configurePreset": "vs2022-multi-config",
    "configuration": "Debug"
  },
  {
    "name": "Release",
    "configurePreset": "vs2022-multi-config",
    "configuration": "Release"
  },
  {
    "name": "RelWithDebInfo",
    "configurePreset": "vs2022-multi-config",
    "configuration": "RelWithDebInfo"
  }
  ]
}

**Note** Dynamic libraries (.dll) needed to resolve runtime functionalities shall be placed in a common location 
on your main system drive. Add the path to your user/system environment variables PATH to allow Stellarium to 
discover them during runtime.

Open Visual Studio 2022. Continue without code.

If you want to use a specific cmake.exe go to Tools -> Options, navigate to Cmake -> General and fill the appropriate
checkbox and path. Otherwise VS 2022 will use it own cmake.exe tool. The path of an external cmake should be included
in your system environment variables PATH as well.

At this point you are ready to start the building process. Go to File -> Open -> Folder... and select <stel_dir>

VS 2022 will automatically detect the CMakePresets.json file and start to configure the first Preset Configuration which
is 'Debug'. You can follow the configuration process in the CMake Output Window. If successful (or with acceptable minor 
errors) you can build this configuration by opening a Developer Command Prompt in VS 2022. Navigate to the <stel_dir> 
and type:

cmake --build --preset Debug

**Note** : Do not use the top menu of VS 2022 (i.e. Build -> Build All). While this may work for this first build, eventually
this sub-menu will disappear... a bug in VS 2022. Rely on the Developer Command Prompt for all builds and re-build.

If the Debug build was successful, select from the Configuration dropdown menu of VS 2022, the next configuration specified in
the CMakePresets.json file, i.e Release in this case.

As soon as you will change this setting in the Configuration dropdown list, VS 2022 will start cmake to configure the build. 
Then build by typing this command in the Developer Command Prompt:

cmake --build --preset Release

Redo the same process for the last build, i.e. RelWithDebInfo.

cmake --build --preset RelWithDebInfo

To run test suites (if ENABLE_TESTING = ON), use again the Developer Command Prompt. Navigate to <stel_dir>+"\build. 
Then type one of these three commands:

ctest -C Debug
ctest -C Release
ctest -C RelWithDebInfo

Once all three builds are completed and unit testing is verified you can close the folder view and open the Stellarium Solution
which is located here : stellarium/build/Stellarium.sln 

File --> Close Folder
File --> Open --> Project/Solution...

Visual Studio 2022 should open Stellarium in Debug/x64 configuration. The Solution Explorer should display all the Stellarium projects. 
Right click on the stellarium project to 'Set as Startup Project'. At this stage you are ready to further modify/rebuild the code, test and 
run Stellarium, or any of the test cases projects listed in the Solution Explorer view, after selecting them as start-up project.

Note: To work on the Release or RelWithDebInfo configuration first select it from the dropdown top menu of Visual Studio 2022.

#### For Visual Studio 2026 (multi-configuration):

You can also used this newly released version of Visual Studio (November 2025) to build and develop Stellarium. This release fixes the bug 
of VS 2022 for the building steps and in a sense simplify the building process of Stellarium. Here are the changes to the process described
above for Visual Studio 2022 (multi-configuration)

Use a slightly different CMakePresets.json file, and adapt it to your development environment.

<CMakePresets.json file example for VS 2026>
{
  "version": 3,
  "cmakeMinimumRequired": { "major": 3, "minor": 21, "patch": 0 },
  "configurePresets": [
    {
      "name": "vs2026-multi-config",
      "displayName": "Stellarium (VS2026 Multi-Config)",
      "description": "Unified build tree for Debug, Release, and RelWithDebInfo using Visual Studio 2026",
      "generator": "Visual Studio 18 2026",
      "architecture": {
        "value": "x64"
      },
      "binaryDir": "${sourceDir}/build",
      "cacheVariables": {
        "CMAKE_CONFIGURATION_TYPES": "Debug;Release;RelWithDebInfo",
	    "CMAKE_BUILD_TYPE": "Debug",
	    "CMAKE_SUPPRESS_DEVELOPER_WARNINGS": "1",
	    "CMAKE_PREFIX_PATH": "C:/Qt/6.7.3/msvc2022_64;C:/Dev/Libs/exiv2-0.28.7/lib/cmake/exiv2",
        "QT_QMAKE_EXECUTABLE": "C:/Qt/6.7.3/msvc2022_64/bin/qmake.exe",
        "ENABLE_SCRIPTING": "ON",
        "ENABLE_GPS": "ON",
        "ENABLE_TESTING": "ON",
        "ENABLE_NLS": "ON",
        "SCM_SHOULD_ENABLE_CONVERTER": "TRUE",
        "GETTEXTPO_LIBRARY": "C:/Dev/Libs/gettextpo/lib/libgettextpo.lib",
        "GETTEXTPO_INCLUDE_DIR": "C:/Dev/Libs/gettextpo/include",
        "LIBTIDY_LIBRARY": "C:/Dev/Libs/libtidy/lib/libtidy.lib",
        "LIBTIDY_INCLUDE_DIR": "C:/Dev/Libs/libtidy/include",
        "EXIV2_LIBRARY": "C:/Dev/Libs/exiv2-0.28.7/lib/exiv2.lib",
        "EXIV2_INCLUDE_DIR": "C:/Dev/Libs/exiv2-0.28.7/include",
        "exiv2_DIR": "C:/Dev/Libs/exiv2-0.28.7/lib/cmake/exiv2",
        "Qt6LinguistTools_DIR": "C:/Qt/6.7.3/msvc2022_64/lib/cmake/Qt6LinguistTools"
      }
    }
  ],
  "buildPresets": [
  {
    "name": "Debug",
    "configurePreset": "vs2026-multi-config",
    "configuration": "Debug"
  },
  {
    "name": "Release",
    "configurePreset": "vs2026-multi-config",
    "configuration": "Release"
  },
  {
    "name": "RelWithDebInfo",
    "configurePreset": "vs2026-multi-config",
    "configuration": "RelWithDebInfo"
  }
  ]
}

Note: To use 'Visual Studio 18 2026' generator, you need the latest version of CMake, version 4.2.0

Open Visual Studio 2026. Open the Stellarium folder.

File -> Open -> Folder... and select <stel_dir>

VS 2026 will automatically detect the CMakePresets.json file and start to configure the first Preset Configuration which
is 'Debug'. To build the Debug configuration, select Build --> Build All from the top menu of VS 2026. Building information should be 
displayed in the Build Output window. If the Debug build was successful you should see the executable stellarium.exe generated in the 
sub-directory <stel_dir>\build\src\Debug.

If the Debug build was successful, select from the Configuration dropdown menu of VS 2026, the next configuration specified in
the CMakePresets.json file, i.e Release in this case.  VS 2026 will start cmake to configure the build. Upon completion, to build the 
Release configuration select Build --> Build All from the top menu of VS 2026. If the Release build was successful you should see the
executable stellarium.exe generated in the sub-directory <stel_dir>\build\src\Release. Repeat the same steps for the RelWithDebInfo 
configuration. If the RelWithDebInfo build was successful you should see the executable stellarium.exe generated in the 
sub-directory <stel_dir>\build\src\RelWithDebInfo.

Upon completion of the build process for all three configurations, you can close the folder and open the Stellarium solution, 
the same way as it is described for VS 2022 above.


### Supported CMake parameters

List of supported parameters (passed as `-DPARAMETER=VALUE`):

 Parameter                           | TYPE   | Default | Description
-------------------------------------| -------|---------|-----------------------------------------------------
| CMAKE_INSTALL_PREFIX               | path   | *       | Prefix where to install Stellarium
| CMAKE_PREFIX_PATH                  | path   |         | Additional path to look for libraries
| CMAKE_BUILD_TYPE                   | string | Release | Build type of Stellarium
| CMAKE_OSX_ARCHITECTURES            | string | arm64;x86_64 | macOS architecture**
| CMAKE_OSX_DEPLOYMENT_TARGET        | string | 11.0    | Minimum macOS deployment version**
| OPENGL_DEBUG_LOGGING               | bool   | OFF     | Enable to log OpenGL information using the GL_KHR_debug extension/QOpenGLLogger
| ENABLE_QT6                         | bool   | ON      | Enable building Qt6-based Stellarium
| ENABLE_NLS                         | bool   | ON      | Enable interface translation
| ENABLE_SHOWMYSKY                   | bool   | ON      | Enable support for ShowMySky module that implements a realistic atmosphere model
| ENABLE_GPS                         | bool   | ON      | Enable GPS support
| ENABLE_LIBGPS                      | bool   | ON      | Enable GPS support with libGPS library (N/A on Windows)
| ENABLE_MEDIA                       | bool   | ON      | Enable sound and video support
| ENABLE_XLSX                        | bool   | ON      | Enable support for XLSX (Excel) files
| ENABLE_SCRIPTING                   | bool   | ON      | Enable the scripting feature
| ENABLE_RELEASE_BUILD               | bool   | OFF     | This option flags the build as an official release
| ENABLE_TESTING                     | bool   | OFF     | Enable unit tests
| ENABLE_QTWEBENGINE                 | bool   | ON      | Enable QtWebEngine module support if it installed
| ENABLE_INDI                        | bool   | ON      | Activate support for INDI client in Telescope Control plugin
| USE_BUNDLED_QTCOMPRESS             | bool   | ON      | Use bundled version of qtcompress
| USE_PLUGIN_ANGLEMEASURE            | bool   | ON      | Enable building the Angle Measure plugin
| USE_PLUGIN_ARCHAEOLINES            | bool   | ON      | Enable building the ArchaeoLines plugin
| USE_PLUGIN_CALENDARS               | bool   | ON      | Enable building the Calendars plugin
| USE_PLUGIN_EQUATIONOFTIME          | bool   | ON      | Enable building the Equation Of Time plugin
| USE_PLUGIN_EXOPLANETS              | bool   | ON      | Enable building the Exoplanets plugin
| USE_PLUGIN_HELLOSTELMODULE         | bool   | OFF     | Enable building the HelloStelModule plugin (example of simple plugin)
| USE_PLUGIN_LENSDISTORTIONESTIMATOR | bool   | ON      | Enable building the Lens Distortion Estimator plugin
| USE_PLUGIN_METEORSHOWERS           | bool   | ON      | Enable building the Meteor Showers plugin
| USE_PLUGIN_MISSINGSTARS            | bool   | ON      | Enable building the Missing Stars plugin
| USE_PLUGIN_MOSAICCAMERA            | bool   | OFF     | Enable building the Mosaic Camera plugin
| USE_PLUGIN_NAVSTARS                | bool   | ON      | Enable building the Navigational Stars plugin
| USE_PLUGIN_NOVAE                   | bool   | ON      | Enable building the Bright Novae plugin
| USE_PLUGIN_OBSERVABILITY           | bool   | ON      | Enable building the Observability Analysis plugin
| USE_PLUGIN_OCULARS                 | bool   | ON      | Enable building the Oculars plugin
| USE_PLUGIN_OCULUS                  | bool   | OFF     | Enable building the Oculus plugin (support for Oculus Rift - outdated)
| USE_PLUGIN_ONLINEQUERIES           | bool   | ON      | Enable building the Online Queries plugin
| USE_PLUGIN_POINTERCOORDINATES      | bool   | ON      | Enable building the Pointer Coordinates plugin
| USE_PLUGIN_PULSARS                 | bool   | ON      | Enable building the Pulsars plugin
| USE_PLUGIN_QUASARS                 | bool   | ON      | Enable building the Quasars plugin
| USE_PLUGIN_REMOTECONTROL           | bool   | ON      | Enable building the Remote Control plugin
| USE_PLUGIN_REMOTESYNC              | bool   | ON      | Enable building the Remote Sync plugin
| USE_PLUGIN_SATELLITES              | bool   | ON      | Enable building the Satellites plugin
| USE_PLUGIN_SCENERY3D               | bool   | ON      | Enable building the 3D Scenery plugin
| USE_PLUGIN_SIMPLEDRAWLINE          | bool   | OFF     | Enable building the SimpleDrawLine plugin (example of simple graphics plugin)
| USE_PLUGIN_SKYCULTUREMAKER         | bool   | ON      | Enable building the Sky Culture Maker plugin
| USE_PLUGIN_SOLARSYSTEMEDITOR       | bool   | ON      | Enable building the Solar System Editor plugin
| USE_PLUGIN_SUPERNOVAE              | bool   | ON      | Enable building the Historical Supernovae plugin
| USE_PLUGIN_TELESCOPECONTROL        | bool   | ON      | Enable building the Telescope Control plugin
| USE_PLUGIN_TEXTUSERINTERFACE       | bool   | ON      | Enable building the Text User Interface plugin
| USE_PLUGIN_VTS                     | bool   | OFF     | Enable building the Vts plugin (allow to use Stellarium as a plugin in CNES VTS)

Notes:
 \* `/usr/local` on Unix-like systems, `C:\Program Files` or `C:\Program Files (x86)`
   on Windows depending on OS type (32 or 64 bit) and build configuration.
 \** Default values for Qt6 environment on macOS

## Test-run compiled program without installing

After compilation, you may run the program when you are in the right directory. 

### Linux
Assuming the stellarium sources are in DEV/stellarium and build in DEV/stellarium/build/unix
```
cd DEV/stellarium
./build/unix/src/stellarium
```

### Windows

Most users will work with QtCreator which sets its own paths for debug and release builds. 
Running with the designated buttons (green arrow) from inside QtCreator should work. 
You can also create a link for the executable in the src subdirectory of the build directory. 
Move this link to the source directory and edit its properties to run inside the source directory. 
Then you can double-click this link, or even place it in your task bar.

## Code testing

There are several test programs in the repository. To build them, define `-DENABLE_TESTING=ON` (or 
`-DENABLE_TESTING=1`), or configure cmake in QtCreator's Projects tab.

Then configure a Debug build and select a test... application to be executed.

Please try to test your changes before committing to master. Our automated 
[GitHub Actions](https://github.com/Stellarium/stellarium/actions/workflows/ci.yml) and 
[AppVeyor](https://ci.appveyor.com/project/alex-w/stellarium) builds will signal failure when 
tests don't complete.

To execute all unit tests in terminal please run:
```
$ make test
```
or 
```
$ ctest --output-on-failure
```

## Packaging

OK, you have built the program from source code and now you may want to install the executable 
file into your operating system or create a package for distribution.

To install the executable file (with necessary libraries and data files) into the directory 
defined in parameter `CMAKE_INSTALL_PREFIX`, run:

```
$ sudo make install
```

### Linux specifics

To create a source packages on linux you need run:
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

On ARM-based (Apple Silicon) Macs, ARM application need to be code signed. To sign the application with an ad-hoc signature:
```
codesign --force --deep -s - Stellarium.app
```

To create the DMG file (Apple Disk Image) run:
```
$ mkdir Stellarium
$ cp -r Stellarium.app Stellarium
$ hdiutil create -format UDZO -srcfolder Stellarium Stellarium.dmg
```

### Windows specifics

To create a Windows installer you need to have installed [Inno Setup](http://www.jrsoftware.org/).

If you have followed all the above procedures the current build will generate the necessary 
`stellarium.iss` file in `C:\Devel\stellarium\builds\msvc`.

Double click on it, then from the menu bar "build-compile". It will build the stellarium installer 
package and place it in a folder of the stellarium source tree root folder `installers`. So you 
can find it in `C:\Devel\stellarium\stellarium\installers`

Or you can use cmake command for create an installer:
```
$ cmake --build c:\devel\stellarium\build\msvc --target stellarium-installer
```

### Supported make targets

Make groups various tasks as "targets". Starting make without any arguments causes make to build 
the default target - in our case, building Stellarium, its tests, the localization files, etc.
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
