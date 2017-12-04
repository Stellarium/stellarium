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

#include "DeviceStrHelpers.h" 
#include "helpers.h" 
#include <regex>

//----------------------------------------------
//      GET    DEVICE       VECT
std::vector<std::string> DeviceStr::GetVect( std::string data )
{
    std::string pattern( "<d>(.*?)</d>" );
    std::regex re( pattern );
    std::sregex_iterator m1( data.begin(), data.end(), re );
    std::sregex_iterator m2;

    std::sregex_iterator iter;
    std::vector<std::string> matchStrs;
    for(iter = m1; iter != m2; ++iter)
    {
        if( (*iter)[1].str().size() )
        {
            matchStrs.push_back( (*iter)[1].str() ); 
        }
    }

    return matchStrs;
}

//----------------------------------------------
//      GET    CAMERAS       VECT
std::vector<std::string> DeviceStr::GetCameras( std::string data  )
{
    std::vector<std::string> devices = DeviceStr::GetVect( data );

    std::vector<std::string>::iterator iter;
    std::vector<std::string> cameras;
    std::string c("camera");
    for( iter = devices.begin(); iter != devices.end(); ++iter )
    {
        std::string type =  DeviceStr::GetType( (*iter) );
        if( 0 == type.compare( c ) )
        {
            cameras.push_back( (*iter) );
        }
    }

    return cameras;
}

//----------------------------------------------
//      GET    DEVICE   TYPE
std::string DeviceStr::GetType( const std::string & data )
{
    return help::GetItemFromFindStr( data, "deviceType=" );
}

//----------------------------------------------
//      GET    DEVICE   NAME
std::string DeviceStr::GetName( const std::string & data )
{
    return help::GetItemFromFindStr( data, "model=" );
}

//----------------------------------------------
//      GET    DEVICE  ADDR (id 1)
std::string DeviceStr::GetAddr( const std::string & data )
{
    return help::GetItemFromFindStr( data, "address=" );
}

//----------------------------------------------
//      GET    DEVICE     PORT (id 2)
std::string DeviceStr::GetPort( const std::string & data )
{
      std::string port = help::GetItemFromFindStr( data, "port=" );

        if( port.size() )
        {
            return port;    
        }
        else
        {
           return "0";
        }
}

//----------------------------------------------
//      GET    DEVICE       INTERFACE
std::string DeviceStr::GetInterface( const std::string & data )
{
    return help::GetItemFromFindStr( data, "interface=" );
}

//----------------------------------------------
//      GET    FW       VER
uint16_t DLL_EXPORT DeviceStr::GetFwVer( const std::string & data )
{
    std::string fwVerStr = help::GetItemFromFindStr( data, "firmwareRev=" );
    return help::Str2uShort( fwVerStr, true);
}
