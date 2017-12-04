/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AltaIo 
* \brief derived class for alta u and e camera io 
* 
*/ 

#include "AltaIo.h" 
#include "AltaEthernetIo.h" 
#include "AltaUsbIo.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include <sstream>
#include <algorithm>

namespace
{
    std::vector<uint32_t> GetValidBaudRates()
    {
        std::vector<uint32_t> result;
        result.push_back(1200);
        result.push_back(2400);
        result.push_back(4800);
        result.push_back(9600);
        result.push_back(19200);
        result.push_back(38400);
        result.push_back(57600);
        result.push_back(115200);
        return result;
    }

    bool IsBaudRateValid( const uint32_t rate )
    {
        std::vector<uint32_t> ratesVect = GetValidBaudRates();

        return( std::find( ratesVect.begin(), ratesVect.end(), rate ) != 
            ratesVect.end() ? true : false );
    }
}

//////////////////////////// 
// CTOR 
AltaIo::AltaIo( CamModel::InterfaceType type, 
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
              m_Interface = std::shared_ptr<ICamIo>( new AltaEthernetIo( deviceAddr ) );
        break;

        case CamModel::USB:
            m_Interface = std::shared_ptr<ICamIo>( new AltaUsbIo( deviceAddr ) );
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
AltaIo::~AltaIo() 
{ 

} 

//////////////////////////// 
//      PROGRAM    
void AltaIo::Program(const std::string & FilenameCamCon,
            const std::string & FilenameBufCon, const std::string & FilenameFx2,
            const std::string & FilenameGpifCamCon,const std::string & FilenameGpifBufCon,
            const std::string & FilenameGpifFifo, bool Print2StdOut )
{
 
    if( CamModel::ETHERNET == m_type )
    {
         std::string errStr("cannot program camera via ethernet");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::dynamic_pointer_cast<AltaUsbIo>(m_Interface)->Program(FilenameCamCon,
        FilenameBufCon, FilenameFx2, FilenameGpifCamCon,
        FilenameGpifBufCon, FilenameGpifFifo, Print2StdOut);
}

//////////////////////////// 
// GET       ID
uint16_t AltaIo::GetId()
{
    const uint16_t rawId = GetIdFromReg();
    return( rawId & CamModel::ALTA_CAMERA_ID_MASK );
}

//////////////////////////// 
// GET   MAC   ADDRESS
std::string AltaIo::GetMacAddress()
{
    if( CamModel::ETHERNET != m_type )
    {
         std::string errStr("cannot read mac address via usb");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
         __LINE__, Apg::ErrorType_InvalidOperation );
    }

    std::string result;
    std::dynamic_pointer_cast<AltaEthernetIo>(
            m_Interface)->GetMacAddress( result );

    return result;
}

//////////////////////////// 
//      SET     SERIAL       BAUD     RATE
void AltaIo::SetSerialBaudRate( const uint16_t PortId , const uint32_t BaudRate )
{
    VerifyPortIdGood(  PortId );
    if( !IsBaudRateValid( BaudRate ) )
    {
         std::stringstream errMsg;
        errMsg << "Invalid baud rate " << BaudRate;
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->SetSerialBaudRate( PortId, BaudRate );
}

//////////////////////////// 
//      GET     SERIAL       BAUD     RATE
uint32_t AltaIo::GetSerialBaudRate(  const uint16_t PortId  )
{
    VerifyPortIdGood(  PortId );

    return std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->GetSerialBaudRate( PortId );
}

//////////////////////////// 
//   GET    SERIAL   FLOW     CONTROL
Apg::SerialFC AltaIo::GetSerialFlowControl( uint16_t PortId )
{
    VerifyPortIdGood(  PortId );

    return std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->GetSerialFlowControl( PortId );
}

//////////////////////////// 
//   SET    SERIAL   FLOW     CONTROL
void AltaIo::SetSerialFlowControl( uint16_t PortId, 
            const Apg::SerialFC FlowControl )
{
    VerifyPortIdGood( PortId );

    std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->SetSerialFlowControl( PortId, FlowControl );
}

//////////////////////////// 
//  GET    SERIAL    PARITY
Apg::SerialParity AltaIo::GetSerialParity( uint16_t PortId )
{
    VerifyPortIdGood(  PortId );

    return std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->GetSerialParity( PortId );
}

//////////////////////////// 
//  SET    SERIAL    PARITY
void AltaIo::SetSerialParity( uint16_t PortId, Apg::SerialParity Parity )
{
    VerifyPortIdGood(  PortId );

    std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->SetSerialParity( PortId, Parity );
}

//////////////////////////// 
//      READ       SERIAL         
void AltaIo::ReadSerial( uint16_t PortId, std::string & buffer )
{
    VerifyPortIdGood(  PortId );

    std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->ReadSerial( PortId, buffer );
}

//////////////////////////// 
//      WRITE       SERIAL   
void AltaIo::WriteSerial( uint16_t PortId, const std::string & buffer )
{
    VerifyPortIdGood(  PortId );

    std::dynamic_pointer_cast<IAltaSerialPortIo>(
            m_Interface)->WriteSerial( PortId, buffer );
}

//////////////////////////// 
//      VERIFY       PORT      ID        GOOD   
void AltaIo::VerifyPortIdGood(  const uint16_t PortId )
{
    if( PortId > 1 )
    {
        std::stringstream errMsg;
        errMsg << "Invalid port " << PortId;
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }
}
