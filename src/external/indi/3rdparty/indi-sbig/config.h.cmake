/* Define to 1 if you have the <linux/videodev2.h> header file. */
#cmakedefine HAVE_LINUX_VIDEODEV2_H 1

/* The symbol timezone is an int, not a function */
#define TIMEZONE_IS_INT 1

/* Define if you have termios.h */
#cmakedefine   HAVE_TERMIOS_H 1

/* Define if you have fitsio.h */
#cmakedefine   HAVE_CFITSIO_H 1

/* Define if you have libnova.h */
#cmakedefine   HAVE_NOVA_H 1

#define VERSION_MAJOR @VERSION_MAJOR@
#define VERSION_MINOR @VERSION_MINOR@