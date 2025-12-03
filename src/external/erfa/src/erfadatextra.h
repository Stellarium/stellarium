/*
** Copyright (C) 2019, NumFOCUS Foundation.
**
** Licensed under a 3-clause BSD style license - see LICENSE
**
** This file is NOT derived from SOFA sources.
**
*/


#ifndef _ERFA_DAT_EXTRA_H
#define _ERFA_DAT_EXTRA_H

#include "erfaextra.h"

/*
**  Get the leap second table, initializing it to the built-in version
**  if necessary.
**
**  This function is for internal use in dat.c only and should
**  not be used elsewhere.
*/
int eraDatini(const eraLEAPSECOND *builtin, int n_builtin,
              eraLEAPSECOND **leapseconds);

#endif
