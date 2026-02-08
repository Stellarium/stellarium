/*
** Copyright (C) 2016-2017, NumFOCUS Foundation.
**
** Licensed under a 3-clause BSD style license - see LICENSE
**
** This file is NOT derived from SOFA sources
*/

/*
 * config.h defines the version information macros;
 * it is auto-generated in the autotools build process.
 * without it, the macros have to be defined explicitly
 * in the call to the compiler.
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

// (SS) 2025-11-27: include erfaextra.h header to access macros
#include "erfaextra.h"

const char* eraVersion(void) {
  return ERFA_VERSION;
}


int eraVersionMajor(void) {
  return ERFA_VERSION_MAJOR;
}


int eraVersionMinor(void) {
  return ERFA_VERSION_MINOR;
}


int eraVersionMicro(void) {
  return ERFA_VERSION_MICRO;
}


const char* eraSofaVersion(void) {
  return SOFA_VERSION;
}
