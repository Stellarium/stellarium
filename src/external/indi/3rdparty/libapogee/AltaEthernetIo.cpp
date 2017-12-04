/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class AltaEthernetIo 
* \brief First generation implemenation (alta) of the ethernet interface 
* 
*/ 

#include "AltaEthernetIo.h" 

#include <sstream>
#include <iomanip>
#include <cstring>  //for memset

#include "libCurlWrap.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include "helpers.h"
#include "CamHelpers.h" 

namespace
{
    const int32_t MAX_WRITES_PER_URL = 40;
    const int32_t MAX_READS_PER_URL = 40;

     // GET    PORT      STR
    std::string GetPortStr( const uint16_t PortId )
    {
        std::string result;

        switch( PortId )
        {
            case 0:
                result.append("A");
            break;

            case 1:
                result.append("B");
            break;

            default:
            {
                std::stringstream errMsg;
                errMsg << "Invalid port " << PortId;
                apgHelper::throwRuntimeException( __FILE__, errMsg.str(), 
                        __LINE__, Apg::ErrorType_InvalidUsage );
            }
            break;
        }

        return result;
    }

    //      UINT32    TO      STR
    std::string uint32ToStr( const uint32_t value )
    {
        std::stringstream ss;
        ss << value;
        return ss.str();
    }

    //  FIND     VAL   IN     MAP
    uint16_t FindValInMap(  const uint16_t Reg, 
        const std::map<uint16_t,uint16_t> & theMap )
    {
        std::map<uint16_t,uint16_t>::const_iterator iter = 
            theMap.find( Reg );

        if( iter != theMap.end() )
        {
            //found a value before we reached the end
            return (*iter).second;
        }
        else
        {
            std::stringstream errMsg;
            errMsg << "Failed to find register " << Reg << " in status map";
            apgHelper::throwRuntimeException( __FILE__, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
            return 0;
        }

    }

    //  MK   HEX   REG      STR
    std::string mkHexRegStr( const uint16_t Reg )
    {
        if( !Reg )
        {
            std::string result("0x0000");
            return result;
        }
        else
        {
            std::stringstream ss;
            ss <<  std::hex << std::setw(6) << std::setfill('0') << std::internal << std::showbase << Reg;
            return ss.str();
        }
    }
}

//////////////////////////// 
// CTOR 
AltaEthernetIo::AltaEthernetIo( const std::string url ) : m_url( url ),
                                                          m_fileName( __FILE__ )

{ 
    //open a session with the camera
    OpenSession();

    m_StatusRegs.push_back(CameraRegs::TEMP_DRIVE);
    m_StatusRegs.push_back(CameraRegs::INPUT_VOLTAGE);
    m_StatusRegs.push_back(CameraRegs::SEQUENCE_COUNTER);
    m_StatusRegs.push_back(CameraRegs::STATUS);
    m_StatusRegs.push_back(CameraRegs::TDI_COUNTER);
    m_StatusRegs.push_back(CameraRegs::TEMP_CCD);
    m_StatusRegs.push_back(CameraRegs::TEMP_HEATSINK);
}

//////////////////////////// 
// DTOR 
AltaEthernetIo::~AltaEthernetIo() 
{ 
    CloseSession();
} 

//////////////////////////// 
//  OPEN    SESSION 
void AltaEthernetIo::OpenSession()
{
    const std::string fullUrl = m_url + "/SESSION?Open";

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

     if( std::string::npos == result.find("SessionId=") )
    {
        std::string errMsg = "Invalid open session response = " + result;
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical  );
    }

     //log device we are connected to
    const std::string msg = "Connected to device " + m_url;
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      msg ); 
}

//////////////////////////// 
//  CLOSE    SESSION 
void AltaEthernetIo::CloseSession()
{
    const std::string fullUrl = m_url + "/SESSION?Close";

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

     if( std::string::npos == result.find("SessionId=") )
    {
        //no throwing b/c this is called by the dtor
        //log the error
         std::string errMsg = "Invalid close session response = " + result;
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE, "error", errMsg);
    }

     //log what device we have disconnected from
    const std::string msg = "Connection to device " + m_url + " is closed.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      msg ); 

}

//////////////////////////// 
// READ     REG 
uint16_t AltaEthernetIo::ReadReg( uint16_t reg ) const
{

    const std::string finalUrl = m_url + "/FPGA?RR="+ help::uShort2Str( reg );
        
    CLibCurlWrap theCurl;
    std::string result;
    theCurl.HttpGet( finalUrl, result );

    std::vector<std::string> tokens = help::MakeTokens(result,"=");

    uint16_t FpgaData = 0;
    std::istringstream is( tokens[1] );
    is >> std::hex >> FpgaData;

    return FpgaData;
}

//////////////////////////// 
// READ     REGS 
std::map<uint16_t,uint16_t> AltaEthernetIo::ReadRegs(const std::vector<uint16_t> & Regs )
{
    //build the get string with all of the registers
    std::string base = m_url + "/FPGA?";
    std::string finalUrl = base;

    std::vector<uint16_t>::const_iterator iter;

    bool first = true;
    std::string finalResult;
    int32_t count = 0;

    for( iter = Regs.begin(); iter != Regs.end(); ++iter )
    {
        if( first )
        {
            std::string vv = "RR=" +  help::uShort2Str( *iter );
            finalUrl.append(vv);
            first = false;
        }
        else
        {
            std::string vv = "&RR=" +  help::uShort2Str( *iter );
            finalUrl.append(vv);
        }

         if( MAX_READS_PER_URL-1 == count )
        {
            //send the max data
            CLibCurlWrap theCurl;
            std::string result;
            theCurl.HttpGet( finalUrl, result );
            finalResult.append( result );

            //reset
            count = 0;
            finalUrl = base;
            first = true;
        }
        else
        {
            ++count;
        }
            
    }

    if( count )
    {
        //send the cmd
        CLibCurlWrap theCurl;
        std::string result;
        theCurl.HttpGet( finalUrl, result );
        finalResult.append( result );
    }

    //convert the string into a map
    std::vector<std::string> tokens = help::MakeTokens(finalResult,"\n");

    std::vector<std::string>::iterator siter;

    std::map<uint16_t,uint16_t> mapOut;

    for( siter = tokens.begin(); siter != tokens.end(); ++siter )
    {
        if( (*siter).empty() )
        {
            continue;
        }
        
        int32_t regStart = apgHelper::SizeT2Int32( (*siter).find("[") );
        int32_t regEnd = apgHelper::SizeT2Int32( (*siter).find("]") );
        int32_t len = regEnd - regStart - 1;
        std::string regStr = (*siter).substr(regStart+1,len);   

        uint16_t theReg = 0;
        std::istringstream is1( regStr );
        is1 >> theReg;

        int32_t valStart = apgHelper::SizeT2Int32( (*siter).find("x") );
        int32_t valEnd = apgHelper::SizeT2Int32( (*siter).size() );
        len = valEnd - valStart - 1;
        std::string valStr = (*siter).substr(valStart+1,len);

        uint16_t theValue = 0;
        std::istringstream is2( valStr );
        is2 >> std::hex >> theValue;

        mapOut[theReg] = theValue;
    }

    return mapOut;
}
   
//////////////////////////// 
// WRITE        REG 
void AltaEthernetIo::WriteReg( uint16_t reg, uint16_t val ) 
{
    std::string fullUrl = m_url + "/FPGA?WR=" +
        help::uShort2Str(reg) + "&WD=" + help::uShort2Str(val, true);

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

}

//////////////////////////// 
// WRITE        MRMD 
void AltaEthernetIo::WriteMRMD(const uint16_t reg, const std::vector<uint16_t> & data )
{
     std::vector<uint16_t>::const_iterator iter;
    uint16_t offset = 0;
    for( iter = data.begin(); iter != data.end(); ++iter, ++offset )
    {
        WriteReg( reg+offset, *iter );
    }
}

//////////////////////////// 
// GET  IMAGE   DATA
void AltaEthernetIo::GetImageData(std::vector<uint16_t> & ImageData)
{
    const int32_t NumBytesExpected = 
        apgHelper::SizeT2Int32( ImageData.size() )*sizeof(uint16_t);

    //grab the data
    std::string fullUrl = m_url + "/UE/image.bin";

    CLibCurlWrap theCurl;
    std::string result;
    theCurl.HttpGet( fullUrl, result );

    if( NumBytesExpected !=  apgHelper::SizeT2Int32( result.size() ) )
    {
        std::stringstream received;
        received <<  result.size();

        std::stringstream requested;
        requested << NumBytesExpected;

        std::string errMsg = fullUrl + " error - " + requested.str() \
            + " bytes requsted " + received.str() + " bytes received.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }
    
    std::string::iterator strIter;
    std::string::iterator strIterNext;
    std::vector<uint16_t>::iterator dataIter;
    int32_t i=0;

    for(strIter = result.begin(); strIter != result.end(); strIter+=2, ++i)
    {
        strIterNext = strIter+1;
        uint8_t a = (*strIter);
        uint8_t b = (*strIterNext);

        //TODO verify this works on mac...
        uint16_t v = ((a << 8) | b);
        ImageData.at(i) = v;
    }
}

//////////////////////////// 
// SETUP     IMG     XFER
void AltaEthernetIo::SetupImgXfer(const uint16_t Rows,
            const uint16_t Cols, const uint16_t NumOfImages, 
            const bool IsBulkSeq)
{
 
    if( !IsBulkSeq)
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "Bulk sequence must be active for AltaEthernetIo", 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //in alta code the number of frames is rolled into the image height
    uint32_t rolledPixels = NumOfImages * Rows;

    std::stringstream rolled;
    rolled << rolledPixels;

    const std::string fullUrl = m_url + "/FPGA?CI=0,0," + help::uShort2Str(Cols)
        + "," + rolled.str() + ",0xFFFFFFFF"; 

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

}

//////////////////////////// 
//      CANCEL     IMG         XFER
void AltaEthernetIo::CancelImgXfer()
{
    std::string errStr("CancelImgXfer not supported on alta ethernet cameras.");
    apgHelper::throwRuntimeException( m_fileName, errStr, 
        __LINE__, Apg::ErrorType_InvalidOperation );
}

//////////////////////////// 
// GET      STATUS
void AltaEthernetIo::GetStatus(CameraStatusRegs::BasicStatus & status)
{    
    
    std::map<uint16_t,uint16_t> statusMap = 
        ReadRegs( m_StatusRegs );

    //TODO - verify that this is OK
    //setting the values we don't have to zero
    status.CoolerDrive =  FindValInMap( 
        CameraRegs::TEMP_DRIVE, statusMap);

    status.FetchCount = 0;

    status.InputVoltage = FindValInMap( 
        CameraRegs::INPUT_VOLTAGE, statusMap);

    status.SequenceCounter = FindValInMap( 
        CameraRegs::SEQUENCE_COUNTER, statusMap);

    status.Status =  FindValInMap( 
        CameraRegs::STATUS, statusMap);

    status.TdiCounter = FindValInMap( 
        CameraRegs::TDI_COUNTER, statusMap);

    status.TempCcd = FindValInMap( 
        CameraRegs::TEMP_CCD, statusMap);

    status.TempHeatSink = FindValInMap( 
        CameraRegs::TEMP_HEATSINK, statusMap);

    status.uFrame = 0;

    status.DataAvailFlag = (status.Status & CameraRegs::STATUS_IMAGE_DONE_BIT);
}

//////////////////////////// 
// GET      STATUS
void AltaEthernetIo::GetStatus(CameraStatusRegs::AdvStatus & status)
{
    CameraStatusRegs::BasicStatus basic;
    memset(&basic,0,sizeof(basic));
    GetStatus( basic );

     //TODO - verify that this is OK 
     //setting the values we don't have to zero
    status.CoolerDrive = basic.CoolerDrive;
    status.CurrentFrame = 0;
    status.DataAvailFlag = basic.DataAvailFlag;
    status.FetchCount = basic.FetchCount;
    status.InputVoltage = basic.InputVoltage;
    status.MostRecentFrame = 0;
    status.ReadyFrame = 0;
    status.SequenceCounter = basic.SequenceCounter;
    status.Status = basic.Status;
    status.TdiCounter = basic.TdiCounter;
    status.TempCcd = basic.TempCcd;
    status.TempHeatSink = basic.TempHeatSink;
    status.uFrame = basic.uFrame;
}

//////////////////////////// 
//  GET      MAC      ADDRESS
void AltaEthernetIo::GetMacAddress( std::string & Mac )
{
   
    const std::string fullUrl = m_url + "/NVRAM?Tag=10&Length=6&Get";

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

    const std::string dataUrl = m_url + "/UE/nvram.bin";
    theCurl.HttpGet( dataUrl, Mac );

}

//////////////////////////// 
//  REBOOT
void AltaEthernetIo::Reboot()
{
    const std::string fullUrl = m_url + "/REBOOT?Submit=Reboot";

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

}

//////////////////////////// 
// WRITE        SRMD 
void AltaEthernetIo::WriteSRMD(const uint16_t reg, const std::vector<uint16_t> & data )
{
    std::string base = m_url + "/FPGA?WR=" + help::uShort2Str(reg);
    std::string fullUrl = base;
    std::vector<uint16_t>::const_iterator iter;

    int32_t count = 0;

    for( iter = data.begin(); iter != data.end(); ++iter )
    {
        std::string temp = "&WD=" + mkHexRegStr( *iter );
        fullUrl.append( temp );

        if( MAX_WRITES_PER_URL-1 == count )
        {
            //send the max data
            CLibCurlWrap theCurl;
            std::string result;
            theCurl.HttpGet( fullUrl, result );

            //reset
            count = 0;
            fullUrl = base;
        }
        else
        {
            ++count;
        }
    }

    //send any remaining data
    if( count )
    {
        CLibCurlWrap theCurl;
        std::string result;
        theCurl.HttpGet( fullUrl, result );
    }
}

//////////////////////////// 
//  GET    NETWORK     SETTINGS
std::string AltaEthernetIo::GetNetworkSettings()
{
    std::string mac;
    GetMacAddress( mac );

    std::string result = "Mac Address: " + mac + "\n";

    return result;
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string AltaEthernetIo::GetDriverVersion()
{
    CLibCurlWrap theCurl;
    return theCurl.GetVerison();
}
        
//////////////////////////// 
// GET       FIRMWARE    REV
uint16_t AltaEthernetIo::GetFirmwareRev()
{
    return ReadReg( CameraRegs::FIRMWARE_REV );
}


//////////////////////////// 
//  GET  INFO
std::string AltaEthernetIo::GetInfo()
{
    std::stringstream result;

    result << "Interface: Ethernet\n";
    result << "Camera Firmware: " << GetFirmwareRev() << "\n";
    result << GetNetworkSettings().c_str() << "\n";
    return result.str();
}

//////////////////////////// 
//      SET     SERIAL       BAUD     RATE
void AltaEthernetIo::SetSerialBaudRate( const uint16_t PortId , const uint32_t BaudRate )
{
     std::string fullUrl = m_url + "/SERCFG?SetBitRate=" +
        GetPortStr( PortId ) + "," + uint32ToStr( BaudRate );

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );
}

//////////////////////////// 
//      GET     SERIAL       BAUD     RATE
uint32_t AltaEthernetIo::GetSerialBaudRate(  const uint16_t PortId  )
{
    const std::string finalUrl = m_url + "/SERCFG?GetBitRate="+ GetPortStr( PortId );
        
    CLibCurlWrap theCurl;
    std::string result;
    theCurl.HttpGet( finalUrl, result );

    std::vector<std::string> tokens = help::MakeTokens(result,",");

    uint32_t BaudRate = 0;
    std::istringstream is( tokens[2] );
    is >>  BaudRate;

    return BaudRate;
    
}

//////////////////////////// 
//   GET    SERIAL   FLOW     CONTROL
Apg::SerialFC AltaEthernetIo::GetSerialFlowControl( uint16_t PortId )
{
    const std::string finalUrl = m_url + "/SERCFG?GetFlowControl="+ GetPortStr( PortId );
        
    CLibCurlWrap theCurl;
    std::string result;
    theCurl.HttpGet( finalUrl, result );

    std::vector<std::string> tokens = help::MakeTokens(result,",");

    Apg::SerialFC flow = Apg::SerialFC_Unknown;

    if( 0 == tokens[2].compare( "N" ) )
    {
        flow = Apg::SerialFC_Off;
    }
    
    if( 0 == tokens[2].compare( "S" ) )
    {
        flow = Apg::SerialFC_On;
    }

    return flow;
}

//////////////////////////// 
//   SET    SERIAL   FLOW     CONTROL
void AltaEthernetIo::SetSerialFlowControl( uint16_t PortId, 
            const Apg::SerialFC FlowControl )
{
    std::string cflowStr;
    switch( FlowControl )
    {
        case Apg::SerialFC_Off:
            cflowStr.append("N");
        break;

        case Apg::SerialFC_On:
            cflowStr.append("S");
        break;

        default:
        {
            std::stringstream msg;
            msg <<  "Invalid SerialFlowControl value = " << FlowControl;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    const std::string fullUrl = m_url + "/SERCFG?SetFlowControl="+ GetPortStr( PortId ) +
        "," + cflowStr;

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

}

//////////////////////////// 
//  GET    SERIAL    PARITY
Apg::SerialParity AltaEthernetIo::GetSerialParity( uint16_t PortId )
{
    const std::string finalUrl = m_url + "/SERCFG?GetParityBits="+ GetPortStr( PortId );
        
    CLibCurlWrap theCurl;
    std::string result;
    theCurl.HttpGet( finalUrl, result );

    std::vector<std::string> tokens = help::MakeTokens(result,",");
    
    Apg::SerialParity parity = Apg::SerialParity_Unknown;  

    if( 0 == tokens[2].compare( "N" ) )
    {
        parity = Apg::SerialParity_None;
    }
    
    if( 0 == tokens[2].compare( "O" ) )
    {
        parity = Apg::SerialParity_Odd;
    }

     if( 0 == tokens[2].compare( "E" ) )
    {
        parity = Apg::SerialParity_Even;
    }

    return parity;
}

//////////////////////////// 
//  SET    SERIAL    PARITY
void AltaEthernetIo::SetSerialParity( uint16_t PortId, 
                                const Apg::SerialParity Parity )
{
    std::string parityStr;
    switch( Parity  )
    {
        case Apg::SerialParity_Even:
            parityStr.append("E");
        break;

        case Apg::SerialParity_Odd:
            parityStr.append("O");
        break;

        case Apg::SerialParity_None:
           parityStr.append("N");
        break;

        default:
        {
            std::stringstream msg;
            msg <<  "Invalid Parity value = " << Parity;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    const std::string fullUrl = m_url + "/SERCFG?SetParityBits="+ GetPortStr( PortId ) +
        "," + parityStr;

    CLibCurlWrap theCurl;

    std::string result;
    theCurl.HttpGet( fullUrl, result );

}

//////////////////////////// 
//      READ       SERIAL         
void AltaEthernetIo::ReadSerial( uint16_t PortId, std::string & buffer )
{
        apgHelper::throwRuntimeException( m_fileName, "Not implemented", 
                __LINE__, Apg::ErrorType_InvalidUsage );
}

//////////////////////////// 
//      WRITE       SERIAL   
void AltaEthernetIo::WriteSerial( uint16_t PortId, const std::string & buffer )
{
   apgHelper::throwRuntimeException( m_fileName, "Not implemented", 
                __LINE__, Apg::ErrorType_InvalidUsage );
}

