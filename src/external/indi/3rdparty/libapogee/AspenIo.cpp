/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
* 
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AspenIo 
* \brief io class for alta-g, aka aspen, aka ??, cameras 
* 
*/ 

#include "AspenIo.h" 
#include "AspenEthernetIo.h" 
#include "AspenUsbIo.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include <sstream>
#include <cstring>  //for memset

//////////////////////////// 
// CTOR 
AspenIo::AspenIo( CamModel::InterfaceType type, 
               const std::string & deviceAddr ) :
                CameraIo( type ),
                m_fileName( __FILE__ )
{ 

     //log that we are trying to connect
    std::string msg = "Try to connection to device " + deviceAddr;
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      apgHelper::mkMsg ( m_fileName, msg, __LINE__) ); 

     //create the camera interface
    switch( m_type )
    {
        case CamModel::ETHERNET:
              m_Interface = std::shared_ptr<ICamIo>( new AspenEthernetIo( deviceAddr ) );
        break;

        case CamModel::USB:
            m_Interface = std::shared_ptr<ICamIo>( new AspenUsbIo( deviceAddr ) );
        break;

        default:
        {
            std::string errStr("Undefined camera interface type");
            apgHelper::throwRuntimeException( m_fileName, errStr, 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }
} 

//////////////////////////// 
// DTOR 
AspenIo::~AspenIo() 
{ 

} 

//////////////////////////// 
// PROGRAM

void AspenIo::Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, const std::string & FilenameDescriptor,
            const std::string & FilenameWebPage, const std::string & FilenameWebServer,
            const std::string & FilenameWebCfg, bool Print2StdOut)
{
    if( CamModel::ETHERNET == m_type )
    {
         std::string errStr("cannot program camera via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

      std::dynamic_pointer_cast<AspenUsbIo>(m_Interface)->Program(
          FilenameFpga, FilenameFx2, FilenameDescriptor, FilenameWebPage,
          FilenameWebServer, FilenameWebCfg, Print2StdOut );
}

//////////////////////////// 
//      GET ID
uint16_t AspenIo::GetId()
{
    const uint16_t rawId = GetIdFromReg();
    return( rawId & CamModel::GEN2_CAMERA_ID_MASK );
}

//////////////////////////// 
// GET   ID   FROM   STR   DB
uint16_t AspenIo::GetIdFromStrDB()
{
    CamInfo::StrDb db = ReadStrDatabase();
    
    if( 0 != db.Id.compare("Not Set") )
    {
            uint16_t id = 0;
            std::stringstream ss( db.Id );
            ss >> id;
            return id;
    }
  
    return 0;
}

//////////////////////////// 
// GET   MAC   ADDRESS
std::string AspenIo::GetMacAddress()
{
    if( CamModel::ETHERNET != m_type )
    {
         std::string errStr("cannot read mac address via usb");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::string result;
    std::dynamic_pointer_cast<AspenEthernetIo>(
            m_Interface)->GetMacAddress( result );

    return result;
}

//////////////////////////// 
//      WRITE        STR         DATA      BASE
void AspenIo::WriteStrDatabase( const CamInfo::StrDb & info )
{
    if( CamModel::ETHERNET == m_type )
    {
         std::string errStr("cannot write string db via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<AspenUsbIo>(
        m_Interface)->WriteStrDatabase( 
         CamInfo::MkStrVectFromStrDb( info )  );
}

//////////////////////////// 
//      READ        STR         DATA      BASE
CamInfo::StrDb AspenIo::ReadStrDatabase()
{
     std::vector<std::string> result;
     
    if( CamModel::ETHERNET == m_type )
    {
        result = std::dynamic_pointer_cast<AspenEthernetIo>(
           m_Interface)->ReadStrDatabase();
    }
    else
    {
        result = std::dynamic_pointer_cast<AspenUsbIo>(
           m_Interface)->ReadStrDatabase();
    }

     return(  CamInfo::MkStrDbFromStrVect( result ) );
}


 std::vector<uint8_t>  AspenIo::GetFlashBuffer( const uint32_t StartAddr, const uint32_t numBytes )
 {
      if( CamModel::ETHERNET == m_type )
    {
         std::string errStr("cannot read flash ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    return std::dynamic_pointer_cast<AspenUsbIo>(
        m_Interface)->GetFlashBuffer( StartAddr, numBytes );
 }
 
//////////////////////////// 
//      READ        NET         DATA      BASE
CamInfo::NetDb AspenIo::ReadNetDatabase()
{
     CamInfo::NetDb result;
     
    if( CamModel::ETHERNET == m_type )
    { // this can be done, but has not been implemented yet.
         std::string errStr("cannot write net db via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    result = std::dynamic_pointer_cast<AspenUsbIo>(
		m_Interface)->ReadNetDatabase();

     return result;
}

//////////////////////////// 
//      WRITE        NET         DATA      BASE
void AspenIo::WriteNetDatabase( const CamInfo::NetDb & input )
{
    if( CamModel::ETHERNET == m_type )
    { // this can be done, but has not been implemented yet.
         std::string errStr("cannot write net db via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<AspenUsbIo>(
        m_Interface)->WriteNetDatabase( input );
}
