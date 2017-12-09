/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class DeviceStrHelpers 
* \brief namespace to assit with parsing device strings 
* 
*/ 


#ifndef DEVICESTRHELPERS_INCLUDE_H__ 
#define DEVICESTRHELPERS_INCLUDE_H__ 

#include <vector>
#include <string>
#include <stdint.h>
#include "DefDllExport.h"

namespace DeviceStr
{ 
    std::vector<std::string> DLL_EXPORT GetVect( std::string data );
    std::vector<std::string> DLL_EXPORT GetCameras( std::string data  );
    std::string DLL_EXPORT GetType( const std::string & data );
    std::string DLL_EXPORT GetName( const std::string & data );
    std::string DLL_EXPORT GetAddr( const std::string & data );
    std::string DLL_EXPORT GetPort( const std::string & data );
    std::string DLL_EXPORT GetInterface( const std::string & data );
    uint16_t DLL_EXPORT GetFwVer( const std::string & data );
}; 

#endif
