/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class ApogeeFilterWheel 
* \brief class for apogee's usb filter wheel 
* 
*/ 

#include "ApogeeFilterWheel.h" 

#include "helpers.h" 

#include "FilterWheelIo.h" 
#include "apgHelper.h" 
#include "helpers.h" 
#include "CamHelpers.h" 
#include "ApgLogger.h" 

#include <map>
#include <sstream>

namespace
{
    

    struct FwInfo
    {
        ApogeeFilterWheel::Type type;
        std::string name;
        uint16_t maxPositions;
    };

    std::map<ApogeeFilterWheel::Type, FwInfo> GetInfoMap()
    {
        std::map<ApogeeFilterWheel::Type, FwInfo>  result;
        FwInfo i = { ApogeeFilterWheel::FW50_7S, "AI FW50 7S", 7 };
        result[ ApogeeFilterWheel::FW50_7S ] = i;

        FwInfo j = { ApogeeFilterWheel::FW50_9R, "AI FW50 9R", 9 };
        result[ ApogeeFilterWheel::FW50_9R ] = j;

        FwInfo k = { ApogeeFilterWheel::AFW50_10S, "AI AFW50 10S", 10 };
        result[ ApogeeFilterWheel::AFW50_10S ] = k;

        FwInfo l = { ApogeeFilterWheel::UNKNOWN_TYPE, "Unknown", 0 };
        result[ ApogeeFilterWheel::UNKNOWN_TYPE ] = l;

        FwInfo m = { ApogeeFilterWheel::AFW31_17R, "AI AFW31 17R", 17 };
        result[ ApogeeFilterWheel::AFW31_17R ] = m;

        return result;
    }

    FwInfo GetInfo( const ApogeeFilterWheel::Type type )
    {
        std::map<ApogeeFilterWheel::Type, FwInfo> filterMap = GetInfoMap();
        std::map<ApogeeFilterWheel::Type, FwInfo>::iterator iter =
                    filterMap.find( type);

        if( iter != filterMap.end() )
        {
                return (*iter).second;
        }

        return filterMap[ ApogeeFilterWheel::UNKNOWN_TYPE ];
    }

}

//////////////////////////// 
// CTOR 
ApogeeFilterWheel::ApogeeFilterWheel() : m_type( ApogeeFilterWheel::UNKNOWN_TYPE ),
                                           m_connected( false )
{ 

} 

//////////////////////////// 
// DTOR 
ApogeeFilterWheel::~ApogeeFilterWheel() 
{ 
    Close();
} 

//////////////////////////// 
//      INIT
void ApogeeFilterWheel::Init( const ApogeeFilterWheel::Type type, 
            const std::string & DeviceAddr )
{
    if( ApogeeFilterWheel::UNKNOWN_TYPE == type )
    {
        apgHelper::throwRuntimeException( __FILE__, 
            "Invalid input filter wheel type", __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    m_Usb =  std::shared_ptr<FilterWheelIo>(new FilterWheelIo( DeviceAddr ) );
    m_type = type;
    m_connected = true;

    std::stringstream msg;
    msg << "Successfully connected to filter wheel " << m_type << " at address " << DeviceAddr.c_str();

    ApgLogger::Instance().Write( ApgLogger::LEVEL_RELEASE,"info",msg.str() ); 
}

//////////////////////////// 
//    CLOSE
void ApogeeFilterWheel::Close()
{
    if( IsConnected() )
    {
        std::stringstream msg;
        msg << "Closing connection to filter wheel " << m_type;

        ApgLogger::Instance().Write( ApgLogger::LEVEL_RELEASE,"info",msg.str() ); 

        m_Usb.reset();
        m_connected = false;
        m_type = ApogeeFilterWheel::UNKNOWN_TYPE;
    }
}

//////////////////////////// 
//      GET     VENDOR         ID
uint16_t ApogeeFilterWheel::GetVendorId()
{
  return m_Usb->GetVendorId();
}
   
//////////////////////////// 
//      GET     PRODUCT         ID
uint16_t ApogeeFilterWheel::GetProductId()
{
    return m_Usb->GetProductId();
}

//////////////////////////// 
//      GET     DEVICE        ID
uint16_t ApogeeFilterWheel::GetDeviceId()
{
    return m_Usb->GetDeviceId();
}

//////////////////////////// 
//      GET        USB     FIRMWARE        REV
std::string ApogeeFilterWheel::GetUsbFirmwareRev()
{
    return m_Usb->GetUsbFirmwareRev();
}

//////////////////////////// 
//          GET    TYPE       NAME
std::string	ApogeeFilterWheel::GetName()
{
    FwInfo info = GetInfo( m_type );
    return info.name;
}

//////////////////////////// 
//          GET    MAX     POSITIONS
uint16_t ApogeeFilterWheel::GetMaxPositions()
{
    FwInfo info = GetInfo( m_type );
    return info.maxPositions;
}

//////////////////////////// 
//          GET     STATUS
ApogeeFilterWheel::Status ApogeeFilterWheel::GetStatus()
{
    if( !IsConnected() )
    {
        return ApogeeFilterWheel::NOT_CONNECTED;
    }

    uint8_t control = 0, pin = 0;
    m_Usb->ReadCtrlPort( control, pin );

    const uint8_t ACTIVE_MASK = 0x01;

    return(  (pin & ACTIVE_MASK) ? ApogeeFilterWheel::ACTIVE : ApogeeFilterWheel::READY );
}

//////////////////////////// 
//      SET     POSITION
void	ApogeeFilterWheel::SetPosition( const uint16_t Position )
{
    if( Position < 1 ) 
	{
		apgHelper::throwRuntimeException( __FILE__, 
            "Cannot set filter to position 0", __LINE__,
            Apg::ErrorType_InvalidUsage );
	}

    if( Position > GetMaxPositions() ) 
	{
		apgHelper::throwRuntimeException( __FILE__, 
            "Input filter position greater than max positions available", 
            __LINE__, Apg::ErrorType_InvalidUsage );
	}

    // hardware is zero-based
    // ui is one based that is why there is a minus 1
    const uint8_t control = help::GetLowByte( Position ) - 1;
    m_Usb->WriteCtrlPort( control, 0 );
}

//////////////////////////// 
//      GET     POSITION
uint16_t ApogeeFilterWheel::GetPosition()
{
     uint8_t control = 0, pin = 0;
    m_Usb->ReadCtrlPort( control, pin );

     // hardware is zero-based
     // ui is one based that is why there is a plus 1
    uint16_t result = (control & 0x0F) + 1;
    return result;
}

//////////////////////////// 
//  IS     CONNECTED
bool ApogeeFilterWheel::IsConnected()
{
    return m_connected;
}
