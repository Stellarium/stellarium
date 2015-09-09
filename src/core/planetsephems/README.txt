README
=======
This Readme gives instructions on how to enable DE430 and/or DE431 for Stellarium for 
more accurate planetary positions.

DE430 covers the dates from 1.Jan 1550 to 22. Jan 2650 with higher precision, whereas 
DE431 covers a longer period of time, from 13201 BC to AD 17191. 
Both are more precise than the standard VSOP87 algorithm used in Stellarium. 

When one or both files are used as described below, 
the algorithm will switch according to the current time displayed in Stellarium and
calculate the planetary positions using the preferred method.

Instructions
============

File description
================
The following files, which can be obtained from 
ftp://ssd.jpl.nasa.gov/pub/eph/planets/Linux/, have to be used:

DE430: linux_p1550p2650.430 (md5 hash 707c4262533d52d59abaaaa5e69c5738)
DE431: lnxm13000p17000.431 (md5 hash fad0f432ae18c330f9e14915fbf8960a)

File Placement
==============
The above mentioned files have to be placed in a folder named 'ephem' inside the 
'Installation Data Directory' (see http://www.stellarium.org/doc/head/fileStructure.html).

This is directory is different on the supported operating systems:
    - On Linux / BSD / other POSIX: 
    It depends on the installation prefix used when building Stellarium. 
    If you built from source, and didn't explicitly specify an install prefix, 
    the prefix will be /usr/local. Typically, pre-built packages for distros will use 
    the /usr prefix. The Installation Data Directory is $PREFIX/share/stellarium
    
    - On Windows:
    It depends on where Stellarium is installed. 
    The main Stellarium installation directory is the Installation Data Directory. 
    Typically this will be C:\Program Files\Stellarium\
    
    - On MacOS X:
    The Installation Data Directory is found inside the application bundle. 

Already placed in the 'Installation Data Directory' should be folders such as 'data', 
'landscapes', 'stars', 'textures',...

_ IDD
  \_ data
  \_ landscapes
  \_ stars
  \_ ephem
    \_ linux_p1550p2650.430
    \_ lnxm13000p17000.431
