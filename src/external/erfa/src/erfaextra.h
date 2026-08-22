/*
** Copyright (C) 2016-2019, NumFOCUS Foundation.
**
** Licensed under a 3-clause BSD style license - see LICENSE
**
** This file is NOT derived from SOFA sources.
**
** The functions here provide an interface to ERFA and SOFA version
** information, and for updating the leap second table.
**
** The eraGetLeapSeconds and eraSetLeapSeconds functions are used as an
** experimental interface for getting and setting the leap second table
** starting in astropy 4.0.  They will be supported as long as needed for
** astropy.  Comments and ideas about the best way to keep the leap
** second tables up to date for all users of erfa are welcome
** (https://github.com/liberfa/erfa).
**
*/


#ifndef _ERFA_EXTRA_H
#define _ERFA_EXTRA_H

#ifdef __cplusplus
extern "C" {
#endif

// (SS) 2025-11-27: Added macros definitions manually
#define ERFA_VERSION "2.0.1"
#define ERFA_VERSION_MAJOR 2
#define ERFA_VERSION_MINOR 0
#define ERFA_VERSION_MICRO 1
#define SOFA_VERSION "20231011"

/*
** Returns the package version
** as defined in configure.ac
** in string format
*/
const char* eraVersion(void);

/*
** Returns the package major version
** as defined in configure.ac
** as integer
*/
int eraVersionMajor(void);

/*
** Returns the package minor version
** as defined in configure.ac
** as integer
*/
int eraVersionMinor(void);

/*
** Returns the package micro version
** as defined in configure.ac
** as integer
*/
int eraVersionMicro(void);

/*
** Returns the orresponding SOFA version
** as defined in configure.ac
** in string format
*/
const char* eraSofaVersion(void);


/*
** Get and set leap seconds (not supported by SOFA; EXPERIMENTAL)
*/

typedef struct {
   int iyear, month;
   double delat;
} eraLEAPSECOND;

int eraGetLeapSeconds(eraLEAPSECOND **leapseconds);
void eraSetLeapSeconds(eraLEAPSECOND *leapseconds, int count);

#ifdef __cplusplus
}
#endif

#endif /* _ERFA_EXTRA_H */
