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

#include "COMHelper.h" 
#include "AltaIo.h" 
#include "AscentBasedIo.h" 
#include "AspenIo.h" 
#include "apgHelper.h"
#include "helpers.h"
#include "windozeHelpers.h"
#include "CamHelpers.h"
#include <sstream>


//----------------------------------------------
//      GET    CAM        INFO
void COMHelper::GetCamInfo( const std::string & intrfcStr, 
                           const unsigned long idOne, 
                           const unsigned long idTwo,  
                           CamModel::PlatformType & platform, 
                           std::string & addr, uint16_t & frmwrRev,
                           uint16_t & camId )
{
     std::shared_ptr<CameraIo> cam;

     bool IsEthernet = false;
    
     if( 0 == intrfcStr.compare( "ethernet" ) )
     {
            //make address string
            std::stringstream ss;
            ss << ":" << idTwo;
            addr = COMHelper::ULong2IpStr( idOne );
            addr.append( ss.str() );
           
            // TODO get this fixed for Aspen
            // HACK!!!!!! Need to change this to be more 
            // robust when we have more time
            try
            {
                cam = std::shared_ptr<CameraIo>(
                    new AltaIo( CamModel::ETHERNET, addr) );
            }
            catch( std::exception & err )
            {
                 cam = std::shared_ptr<CameraIo>(
                    new AspenIo( CamModel::ETHERNET, addr) );
            }

            IsEthernet = true;
     }
     else if( 0 == intrfcStr.compare( "usb" ) )
     {
            std::stringstream ss;
            ss << idOne;
            addr = ss.str();

            uint16_t usb_enum = static_cast<uint16_t>( idOne & 0x00FF );
            uint16_t vid = 0;
            uint16_t pid = 0;
            windozeHelpers::GetVidAndPid( usb_enum, vid, pid );
            if( UsbFrmwr::ALTA_USB_PID == pid )
            {
                cam = std::shared_ptr<CameraIo>(
                    new AltaIo(CamModel::USB, addr ) );
            }
            else if( UsbFrmwr::ASCENT_USB_PID == pid )
            {
                 cam = std::shared_ptr<CameraIo>(
                    new AscentBasedIo(CamModel::USB, addr ) );
            }
            else if(  UsbFrmwr::ASPEN_USB_PID == pid )
            {
                 cam = std::shared_ptr<CameraIo>(
                    new AspenIo(CamModel::USB, addr ) );
            }
            else
            {
                std::runtime_error except( "COMHelper::GetCamInfo - Invalid pid" );
                throw except;
            }
    }
    else
    {
        apgHelper::throwRuntimeException( __FILE__, 
            "Invalid interface type", __LINE__, Apg::ErrorType_InvalidUsage );
    }

    frmwrRev = cam->GetFirmwareRev();
    camId = cam->GetId();
    platform = CamModel::GetPlatformType( camId, IsEthernet ) ;

}

//----------------------------------------------
//     ULONG       2      IP      STR 
std::string COMHelper::ULong2IpStr( const unsigned long ipAddr )
{
      std::stringstream ss;
      ss << ( (ipAddr >> 24) & 0xFF) << "." << ( (ipAddr >> 16) & 0xFF) << ".";
      ss << ( (ipAddr >> 8) & 0xFF)  << "." << (ipAddr & 0xFF);

      return ss.str();
}

//----------------------------------------------
//      IP STR         2      ULONG       
unsigned long COMHelper::IpStr2ULong( const std::string & ipAddr )
{
    std::vector<std::string> parts = help::MakeTokens( ipAddr, "." );

    std::vector<std::string>::iterator iter;
    int shift = 24;
    unsigned long result = 0;

    for( iter = parts.begin(); iter != parts.end(); ++iter, shift -= 8 )
    {
        //cannot use unsigned char, because the stringstream only converts
        //the first char of the input string...not 100% why, ushort type works the
        //way i woud expect.
        uint16_t num = 0;

        std::stringstream ss( (*iter) );
        ss >> num;

        result |= ( num << shift );
    }

    return result;
}
