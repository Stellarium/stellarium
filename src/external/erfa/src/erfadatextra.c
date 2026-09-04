/*
** Copyright (C) 2019, NumFOCUS Foundation.
**
** Licensed under a 3-clause BSD style license - see LICENSE
**
** This file is NOT derived from SOFA sources.
**
** The eraGetLeapSeconds and eraSetLeapSeconds functions are used as an
** experimental interface for getting and setting the leap second table in
** astropy 4.0.  They will be supported as long as astropy 4.0 is supported
** (until 2021), but not necessarily beyond.  Comments and ideas about the
** best way to keep the leap second tables up to date for all users of erfa
** are welcome (https://github.com/liberfa/erfa).
**
** The eraDatini function is used internally in dat.c; it is strictly an
** implementation detail and should not be used elsewhere.
*/
#include "erfa.h"
#include "erfaextra.h"

static eraLEAPSECOND *changes;
static int NDAT = -1;


int eraGetLeapSeconds(eraLEAPSECOND **leapseconds)
/*
**  Get the current leap second table.
**
**  Returned:
**     leapseconds eraLEAPSECOND*  Array of year, month, TAI minus UTC
**
**  Returned (function value):
**            int  NDAT  Number of entries/status
**                       >0 = number of entries
**                       -1 = internal error
*/
{
    if (NDAT <= 0) {
        double delat;
        int stat = eraDat(2000, 1, 1, 0., &delat);
        if (stat != 0 || NDAT <= 0) {
            return -1;
        }
    }
    *leapseconds = changes;
    return NDAT;
}

void eraSetLeapSeconds(eraLEAPSECOND *leapseconds, int count)
/*
**  Set the current leap second table.
**
**  Given:
**     leapseconds eraLEAPSECOND*  Array of year, month, TAI minus UTC
**     count       int             Number of entries.  If <= 0, causes
**                                 a reset of the table to the built-in
**                                 version.
**
**  Notes:
**     *No* sanity checks are performed.
*/
{
    changes = leapseconds;
    NDAT = count;
}

int eraDatini(const eraLEAPSECOND *builtin, int n_builtin,
              eraLEAPSECOND **leapseconds)
/*
**  Get the leap second table, initializing it to the built-in version
**  if necessary.
**
**  This function is for internal use in dat.c only and should
**  not be used elsewhere.
**
**  Given:
**     builtin     eraLEAPSECOND   Array of year, month, TAI minus UTC
**     n_builtin   int             Number of entries of the table.
**
**  Returned:
**     leapseconds eraLEAPSECOND*  Current array, set to the builtin one
**                                 if not yet initialized.
**
**  Returned (function value):
**            int  NDAT  Number of entries
*/
{
    if (NDAT <= 0) {
        eraSetLeapSeconds((eraLEAPSECOND *)builtin, n_builtin);
    }
    *leapseconds = changes;
    return NDAT;
}
