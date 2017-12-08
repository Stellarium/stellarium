/* The symbol timezone is an int, not a function */
#define TIMEZONE_IS_INT 1

/* Define if you have termios.h */
#cmakedefine   HAVE_TERMIOS_H 1

/* Define if you have fitsio.h */
#cmakedefine   HAVE_CFITSIO_H 1

/* Define Driver version */
#define RADIOSIM_VERSION_MAJOR @RADIOSIM_VERSION_MAJOR@
#define RADIOSIM_VERSION_MINOR @RADIOSIM_VERSION_MINOR@
