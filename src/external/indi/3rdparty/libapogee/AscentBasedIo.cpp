/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AscentBasedIo 
* \brief io class for ascent, alta f and other ascent based cameras 
* 
*/ 

#include "AscentBasedIo.h" 
#include "AscentBasedUsbIo.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include <sstream>

//////////////////////////// 
// CTOR 
AscentBasedIo::AscentBasedIo(CamModel::InterfaceType type, 
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
        case CamModel::USB:
            m_Interface = std::shared_ptr<ICamIo>( new AscentBasedUsbIo( deviceAddr ) );
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
AscentBasedIo::~AscentBasedIo() 
{ 

} 

//////////////////////////// 
//  PROGRAM
void AscentBasedIo::Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, 
            const std::string & FilenameDescriptor,
            bool Print2StdOut)
{
    std::dynamic_pointer_cast<AscentBasedUsbIo>(m_Interface)->Program(
        FilenameFpga, FilenameFx2, FilenameDescriptor,Print2StdOut );
}

//////////////////////////// 
//      WRITE        STR         DATA      BASE
void AscentBasedIo::WriteStrDatabase( const CamInfo::StrDb & info )
{
    std::dynamic_pointer_cast<AscentBasedUsbIo>(
        m_Interface)->WriteStrDatabase( 
         CamInfo::MkStrVectFromStrDb( info )  );
}

//////////////////////////// 
//      READ        STR         DATA      BASE
CamInfo::StrDb AscentBasedIo::ReadStrDatabase()
{
     std::vector<std::string> result = std::dynamic_pointer_cast<AscentBasedUsbIo>(
        m_Interface)->ReadStrDatabase();

     return(  CamInfo::MkStrDbFromStrVect( result ) );
}

//////////////////////////// 
//      GET ID
uint16_t AscentBasedIo::GetId()
{
    CamInfo::StrDb db = ReadStrDatabase();
    
    if( 0 != db.Id.compare("Not Set") )
    {
            uint16_t id = 0;
            std::stringstream ss( db.Id );
            ss >> id;
            return id;
    }
  
    const uint16_t rawId = GetIdFromReg();
    return( rawId & CamModel::GEN2_CAMERA_ID_MASK );

}
