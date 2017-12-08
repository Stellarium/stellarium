In version 2.2 is INDI Server project independent on cmake or homebrew build system, you need XCode 7.2 to build it.

To use it, you need the following folders on the same level as indi-code:

libnova-0.15.0        <= http://downloads.sourceforge.net/project/libnova/libnova/v%200.15.0/libnova-0.15.0.tar.gz
libusb-1.0.19         <= https://sourceforge.net/projects/libusb/files/latest/download?source=files
cfit3380              <= http://heasarc.gsfc.nasa.gov/FTP/software/fitsio/c/cfitsio3380.tar.gz
gsl-2.1               <= http://gnu.prunk.si/gsl/gsl-2.1.tar.gz (run configure and make to create header files)

To build SBIG driver you need SBIGUDrv.framework (SDK available from SBIG web).
To build QSI driver you need libftd2xx.a library (part D2XX driver available from FTDIChip web).

To use it without CloudMakers closed-source drivers remove dependency on the following drivers

indi-atik
indi-shoestring
indi-usbfocus

If you will need some help, contact us on info@cloudmakers.eu.