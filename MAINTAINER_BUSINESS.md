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

## Compression of newly added images

PNG images can frequently be more efficiently compressed by using a tool designed
for the purpose, such as [pngcrush](https://pmt.sourceforge.io/pngcrush/),
[oxipng](https://github.com/shssoichiro/oxipng), or
[ect](https://github.com/fhanau/Efficient-Compression-Tool).

When adding new images to the Stellarium project, contributors should make sure
that added PNGs (and exported images from SVG sources) are compressed as much
as is reasonably achievable.

Benchmarks [suggest](https://css-ig.net/benchmark/png-lossless) that `ect` is the
most efficient recompression tool for full color images, particularly larger ones,
so that is what we recommend.

### Using ECT to recompress images

ECT is an open source tool that compiles and runs on Linux, Windows, and macOS.
To compress a PNG image in place, run the following command:

    ect -9 image.png

The `-9` specifies the maximum effort in searching for optimal compression
parameters, and is not appropriate for use in CIs or build scripts due to its
low speed. `-3` is the default compression speed, which is much faster and
compresses almost as well.

Note: when recompressing an entire directory of existing images, it is advisable
to add the option `--strict`. This prevents `ect` from stripping metadata out of
images. In rare cases, this metadata could affect how the image is displayed.
If you did not create an image and cannot verify changes to it, avoid making
changes to image metadata.
