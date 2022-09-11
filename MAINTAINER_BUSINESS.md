## Using external libraries

Stellarium uses [CPM](https://github.com/cpm-cmake/CPM.cmake) to automatically
download several dependencies during build process, if they are missing from
the target system. Follow that page for more details, but here's summary.

### Developer point of view

If you want to use an external library, use
[`find_package()`](https://cmake.org/cmake/help/latest/command/find_package.html).
If you want to provide a fallback of downloading it when they are missing, use
`CPMFindPackage()`. If `find_package()` is impossible to use because the
library doesn't provide `<Foo>Config.cmake` and there is no `Find<Foo>.cmake`,
use e.g. `find_library()` followed by `CPMAddPackage()`: `CPMFindPackage()`
itself is essentially `find_package()` followed by `CPMAddPackage()`.

Then it depends on the library. If it can be used without any changes and
provides a good `CMakeLists.txt`, the simplest way is to just use that file.
This is the default operation mode of CPM. However, if `CMakeLists.txt` is
missing or poorly-written, or if some other changes (patches etc) are
necessary, use `DOWNLOAD_ONLY YES` option, and use the files from
`${foo_SOURCE_DIR}` as you please. After making use of the files provide an
alias for the library to match the name exported by `find_package(Foo)`, so
that the rest of cmake config doesn't need to care whether the dependency was
found locally, or downloaded automatically.

### Distributions / packaging point of view

If you already have the dependent library packaged, it should be picked up normally.
It may be a good idea to provide `-DCPM_USE_LOCAL_PACKAGES=yes` to cmake to
ask CPM to show an error if the package is missing, instead of trying to download
anything.
