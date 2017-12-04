/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class FindDeviceEthernet 
* \brief class that tries to find apogee devices on the ethernet 
* 
*/ 

#include "FindDeviceEthernet.h" 
#include "UdpSocketBase.h" 

#include "CamHelpers.h" 
#include "apgHelper.h" 
#include "helpers.h"
#include "CameraInfo.h" 

#include <sstream>

#if defined (WIN_OS)
    #include "UdpSocketWin.h" 
#else
     #include "linux/UdpSocketLinux.h"
#endif


namespace
{
    const uint16_t APOGEE_IP_PORT_NUMBER = 2571;
}

//////////////////////////// 
// CTOR 
FindDeviceEthernet::FindDeviceEthernet() : m_fileName(__FILE__),
                                           m_CamResponse("Discovery::Response: \"Apogee\"; 0x12345678;"),
                                           m_socketPtr(0)

{ 
    //create the right type of 
    //socket for the platform
    #if defined (WIN_OS)
        m_socketPtr = new UdpSocketWin();
	#else
        m_socketPtr = new UdpSocketLinux();
    #endif

} 

//////////////////////////// 
// DTOR 
FindDeviceEthernet::~FindDeviceEthernet() 
{ 
    delete m_socketPtr;
    m_socketPtr = 0;
} 

//////////////////////////// 
// GET  ELAPSED   SECS
int32_t FindDeviceEthernet::GetElapsedSecs()
{
    return m_socketPtr->GetElapsedSecs();
}

//////////////////////////// 
// GET  TIMEOUT
int32_t FindDeviceEthernet::GetTimeout()
{
    return m_socketPtr->GetTimeout();
}

//////////////////////////// 
// FIND 
std::string FindDeviceEthernet::Find(const std::string & subnet)
{

    std::vector<std::string> CameraMsgVect = m_socketPtr->Search4ApogeeDevices(subnet,
        APOGEE_IP_PORT_NUMBER);


    //return an no op string to the caller
    std::string noop("<d></d>");
    if( CameraMsgVect.empty() )
    {
        return noop;
    }

    
    //loop through each message and build up
    //the device string
    std::string devices;

    try
    {
        std::vector<std::string>::iterator msgIter;
        for(msgIter = CameraMsgVect.begin(); msgIter != CameraMsgVect.end(); ++msgIter)
        {
            //check that it is a camera that responded
            if( 0 == (*msgIter).compare( 0, m_CamResponse.size(), m_CamResponse ) )
            {
                devices.append( MakeDeviceStr( (*msgIter) ) );
            }
        }
    }
    catch( std::exception & err )
    {
        // catch, log, and rethrow errors from parsecfgfile
        apgHelper::LogErrorMsg( __FILE__, err.what(), __LINE__ );

        throw;
    }

    //return an no op string if there are no devices
    if( devices.empty() )
    {
        return noop;
    }

    return devices;
}

//////////////////////////// 
// MAKE     DEVICE      STR 
std::string FindDeviceEthernet::MakeDeviceStr(const std::string & input)
{

    std::string id("0xFFFF");
    std::string firmwareRev( CamModel::GetNoOpFirmwareRev() );
    std::string ipAddr("0.0.0.0");
    std::string port("0");
    std::string mac("0");
    std::string interfaceStatus("NA");


    std::vector<std::string> lines = help::MakeTokens(input,"\r\n");

    std::vector<std::string>::iterator iter;

    for(iter = lines.begin(); iter != lines.end(); ++iter)
    {
        GetId( (*iter), id );
        GetFirmwareRev( (*iter), firmwareRev );
        GetIpAddr((*iter), ipAddr );
        GetPort((*iter), port );
        GetMacAddr((*iter), mac );
        GetInterfaceStatus((*iter), interfaceStatus );
    }
    
    std::string result = "<d>interface=ethernet,deviceType=camera,address=" + ipAddr  +
        ",port=" + port + ",mac=" + mac + ",interfaceStatus=" + interfaceStatus + "," +
        CameraInfo(id, firmwareRev) +"</d>";

    return result;
}

//////////////////////////// 
// GET  ID 
void FindDeviceEthernet::GetId( const std::string & input, std::string & id )
{
    if( input.find("CameraModel:") != std::string::npos )
    {
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        id = ff.at(1);
    }
}

//////////////////////////// 
// GET          FIRMWARE        REV 
void FindDeviceEthernet::GetFirmwareRev( const std::string & input, 
                                        std::string & firmwareRev )
{
    if( input.find("FirmwareRev:") != std::string::npos )
    {
        //split string in half and get the desired data
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        firmwareRev = ff.at(1);
    }
}
        
//////////////////////////// 
//GET   IP  ADDR
void FindDeviceEthernet::GetIpAddr( const std::string & input, 
                                   std::string & ipAddr )
{
    if( input.find("Configure-Tcp-Ip::IPv4-Address-1-Current:") != std::string::npos )
    {
        //split string in half and get the desired data
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        ipAddr = ff.at(1);
    }
}
        
//////////////////////////// 
// GET  PORT
void FindDeviceEthernet::GetPort( const std::string & input, 
                                 std::string & port )
{
    if( input.find("Configure-Tcp-Ip::IPv4-Port-1:") != std::string::npos )
    {
        //split string in half and get the desired data
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        port = ff.at(1);
    }
}
        
//////////////////////////// 
// GET  MAC ADDR
void FindDeviceEthernet::GetMacAddr( const std::string & input, std::string & mac )
{
    if( input.find("Monitor-Camera::Name-Camera-1:") != std::string::npos )
    {
        //split string in half and get the desired data
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        mac = ff.at(1);

        //remove the " on the mac
        std::string searchString( "\"" ); 
        std::string replaceString( "" );

        std::string::size_type pos = 0;
        while ( (pos = mac.find(searchString, pos)) != std::string::npos ) 
        {
            mac.replace( pos, searchString.size(), replaceString );
            ++pos;
        }
    }
}

//////////////////////////// 
// GET  INTERFACE   STATUS
void FindDeviceEthernet::GetInterfaceStatus( const std::string & input, 
                        std::string & interfaceStatus )
{
    if( input.find("InterfaceStatus:") != std::string::npos )
    {
        //split string in half and get the desired data
        std::vector<std::string> ff = help::MakeTokens( input, ": " );
        interfaceStatus = ff.at(1);

        //remove the " on the interfaceStatus
        std::string searchString( "\"" ); 
        std::string replaceString( "" );

        std::string::size_type pos = 0;
        while ( (pos = interfaceStatus.find(searchString, pos)) != std::string::npos ) 
        {
            interfaceStatus.replace( pos, searchString.size(), replaceString );
            ++pos;
        }
    }
}

//////////////////////////// 
// CAMERA     INFO
std::string FindDeviceEthernet::CameraInfo(const std::string & rawIdStr, 
                                           const std::string & frmwRevStr)
{
    
    const uint16_t rev = help::Str2uShort( frmwRevStr );

    // AltaE's don't send the firmware rev, so if
    // we get the no op rev, then assume (I know this is bad...)
    // it is an Alta camera and use the standard firmware rev
    // of 33 to get the correct id mask
    uint16_t tempRev = rev;
    if( rev == help::Str2uShort( CamModel::GetNoOpFirmwareRev() ) )
    {
        tempRev = 33;
    }
    
    uint16_t fixedId = 6500;

    if( std::string::npos != rawIdStr.find("0x") )
    {
        const uint16_t rawId = help::Str2uShort( rawIdStr, true );
        fixedId = CamModel::MaskRawId( tempRev, rawId );  
    }
    else
    {
        //Aspen ids are in decimal in flash, so they don't need to
        //be masked or converted
        fixedId =  help::Str2uShort( rawIdStr, false );
    }

    std::stringstream idStr;
    idStr << std::hex << std::showbase << fixedId;

    std::stringstream revStr;
    revStr << std::hex << std::showbase << rev;

    std::string modelStr = CamModel::GetPlatformStr( fixedId, true  );
   
    std::string camInfoStr = "id=" + idStr.str() +",firmwareRev=" + revStr.str() +
        ",model=" + modelStr+ "-" + CamModel::GetModelStr(fixedId);

    return camInfoStr;
}
