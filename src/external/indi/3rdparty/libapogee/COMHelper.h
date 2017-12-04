/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class COMHelper 
* \brief namespace for helping of the new libapogee communicate with the old com interface, especially on camera creation 
* 
*/ 


#ifndef COMHELPER_INCLUDE_H__ 
#define COMHELPER_INCLUDE_H__ 

#include <string>
#include "CameraInfo.h"
#include "DefDllExport.h"

namespace COMHelper 
{ 
    void DLL_EXPORT GetCamInfo( const std::string & intrfcStr, 
        const unsigned long idOne, 
        const unsigned long idTwo, 
        CamModel::PlatformType & platform, 
        std::string & addr, 
        uint16_t & frmwrRev,
        uint16_t & camId );

    std::string ULong2IpStr( const unsigned long ipAddr );

    unsigned long DLL_EXPORT IpStr2ULong( const std::string & ipAddr );

}; 

#endif
