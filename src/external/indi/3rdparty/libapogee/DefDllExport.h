/*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#ifdef WIN_OS
  #ifdef LIBAPG_STATIC
        #define DLL_EXPORT
   #elif SWIG
        #define DLL_EXPORT
    #else
         //for windows set the dll export
         #define DLL_EXPORT __declspec( dllexport )
    #endif
#else
      // for linux this is a no op
     #define DLL_EXPORT
#endif
