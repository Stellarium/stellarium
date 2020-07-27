# Building Stellarium

Hello, and thank you for your interest to Stellarium! 

## Why build from source?

Each time Stellarium is released, the source code is released in Github's 
packaging system. Building source code that is released in this way 
should give you a working copy of Stellarium which is functionally 
identical to the binaries for that release.

It is also possible to get the "in development" source code using Git. 
This may contain new features which have been implemented since the last 
release of Stellarium, so it's often more fun.

*Warning:* Git versions of the Stellarium source code are work in progress, 
and as such may produce an unstable program, may not work at all, or may 
not even compile.

## Integrated Development Environment (IDE)

If you plan to develop Stellarium, it is highly recommended to utilize 
an IDE. You can use any IDE of your choice, but QtCreator is recommended 
as more suited for Qt development.

## Prerequisite Packages

To build and develop Stellarium, several packages may be required from 
your distribution. Here's a list.

### Required dependencies

- A C++ compiler able to compile C++11 code ([GCC](https://gcc.gnu.org/) 4.8.1 or later, Clang 3.3 or later, MSVC 2015 or later)
- [CMake](http://www.cmake.org/) 2.8.12 or later -- buildsystem used by many open source projects
- Qt Framework 5.7.0 or later
- [OpenGL](https://www.opengl.org/) -- graphics library
- [Zlib](http://www.zlib.net) -- compression library

### Optional dependencies

- [gettext](https://www.gnu.org/software/gettext/) -- required for developers for extract of lines for translation
- [Doxygen](http://doxygen.org/) -- if you want to build the API documentation you will need this
- [Graphviz](http://www.graphviz.org/) -- required to build the API documentation and include fancy class diagrams.

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
You should now have a directory <code>stellarium-0.20.2</code> with the source code in it.

### Clone project from GitHub

To create the copy install git from your OS distribution repository or from 
https://git-scm.com/ and then execute the following commands:

```
git clone https://github.com/Stellarium/stellarium.git
cd stellarium
```
## Building

OK, assuming you've collected all the necessary libraries, here's
what you need to do to build and run Stellarium on linux:

```
mkdir -p build/unix
cd build/unix
cmake -DCMAKE_INSTALL_PREFIX=/opt/stellarium ../.. 
make -jN
sudo make install
```
Instead of N in -j pass a number of CPU cores you want to use during
a build.

If you have Qt5 installed using official Qt installer, then pass parameter
CMAKE_PREFIX_PATH to cmake call used to configure Stellarium, e.g.

```
cmake -DCMAKE_PREFIX_PATH=/opt/Qt5 ../..
```

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
| ENABLE_GPS                    | bool   | ON      | Enable GPS support.
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
 \* /usr/local on Unix-like systems, c:\Program Files or c:\Program Files (x86)
   on Windows depending on OS type (32 or 64 bit) and build configuration.

### Supported make targets

Make groups various tasks as "targets". Starting make without any arguments causes make to build the default target - in our case, building Stellarium, its tests, the localization files, etc.
 Target          | Description
-----------------|----------------------------------------------------------------------------------------------------
| install        | Installation of all binaries and related files to the directory determined by CMAKE_INSTALL_PREFIX
| test           | Launch the suite of test executables
| apidoc         | Generate an API documentation
| package_source | Create a source package for distributions
| package        | Create a binary packages for distributions on linux/UNIX*
| installer      | Create a binary packages for distributions on Windows

Notes:
 \* After building of TGZ binary package you may create a DEB or RPM package: cpack -G DEB or cpack -G RPM.

Thanks!

\- *The Stellarium development team*
