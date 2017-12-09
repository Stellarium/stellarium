/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class AspenEthernetIo 
* \brief Class for talking with apsen generation cameras with libcurl 
* 
*/ 

#include "AspenEthernetIo.h" 

#include "libCurlWrap.h" 
#include "apgHelper.h" 
#include "helpers.h"
#include "ApgLogger.h" 
#include "CamHelpers.h" 
#include <sstream>
#include <ctime>
#include <algorithm>

// TODO figure out what to do
// with the pretty formatting.  g++ doesn't
// seem to have full regex support
//#ifdef WIN_OS
//#include <regex>
//#else
//#include <tr1/regex>
//#endif

#include <cstring>  //for memcpy

namespace
{
    bool IsResultOk(const std::string & response)
    {
         if( std::string::npos != response.find("OK") )
         {
             return true;
         }
         else
         {
             return false;
         }
    }

    bool IsSessionOk(const std::string & response, const std::string & key )
    {
        size_t found = response.find("=");
  
        if( std::string::npos != found )
        {
             if( 0 == response.compare( found+1, key.size(), key) )
             {
                 return true;
             }
        }

        return false;

    }

    const uint32_t MAX_STRDB_READ_BYTES = 1280;

}

//////////////////////////// 
// CTOR 
AspenEthernetIo::AspenEthernetIo( const std::string url ) : m_url( url ),
                                                          m_fileName( __FILE__ ),
                                                          m_libcurl( new CLibCurlWrap ),
                                                          m_sessionKey(""),
                                                          m_sessionKeyUrlStr("")

{ 

    //make the guid for the session key from
    //the date/time
    time_t rawtime;
    time( &rawtime );
    struct tm * timeinfo = localtime( &rawtime );
    char buffer [80];
    strftime(buffer,80,"%Y%m%d%H%M%S", timeinfo);
    m_sessionKey.append( buffer );
    m_sessionKeyUrlStr.append("&keyval=");
    m_sessionKeyUrlStr.append( m_sessionKey );
    
    StartSession();

	m_lastExposureTimeRegHigh = 0;
	m_lastExposureTimeRegLow = 0;
}

//////////////////////////// 
// DTOR 
AspenEthernetIo::~AspenEthernetIo() 
{ 
    EndSession();
} 


//////////////////////////// 
// START     SESSION
void AspenEthernetIo::StartSession()
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=Start_Session" + m_sessionKeyUrlStr;

    std::string result;
    m_libcurl->HttpGet( fullUrl, result );

    if( !IsSessionOk( result, m_sessionKey ) )
    {
        std::string errMsg = "ERROR - command " + fullUrl + " failed.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }

     //log device we are connected to
    const std::string msg = "Connected to device " + m_url;
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      msg ); 
}

//////////////////////////// 
// END     SESSION
void AspenEthernetIo::EndSession()
{
     const std::string fullUrl = m_url + "/camcmd.cgi?req=End_Session" + m_sessionKeyUrlStr;

    std::string result;
    m_libcurl->HttpGet( fullUrl, result );

    if( !IsSessionOk( result, m_sessionKey ) )
    {
        std::string errMsg = "ERROR - command " + fullUrl + " failed.";
        std::string msg2Log = apgHelper::mkMsg( m_fileName, errMsg, __LINE__ );
        
        //since this is called by the dtor all I can do is 
        //log that something bad happened
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE, "error", msg2Log);
    }

    //log what device we have disconnected from
    const std::string msg = "Connection to device " + m_url + " is closed.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      msg ); 
}

//////////////////////////// 
// READ     REG 
uint16_t AspenEthernetIo::ReadReg( uint16_t reg ) const
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=CC_Reg_Rd&wIndex=" +
        help::uShort2Str(reg) + "&wValue=1" + m_sessionKeyUrlStr;

    std::string result;
    m_libcurl->HttpGet(fullUrl, result );

    uint16_t FpgaData = 0;
    
    std::stringstream is( result );
    is >> std::hex >> FpgaData;

    return FpgaData;
    
}
   
//////////////////////////// 
// WRITE        REG 
void AspenEthernetIo::WriteReg( uint16_t reg, uint16_t val ) 
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=CC_Reg_Wr&wIndex=" +
        help::uShort2Str(reg) + "&wValue=1&param=" + 
        help::uShort2Str(val) + m_sessionKeyUrlStr;

    std::string result;
    m_libcurl->HttpGet( fullUrl, result );

    if( !IsResultOk( result ) )
    {
        std::string errMsg = "ERROR - command " + fullUrl + " failed.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }
   
	
	if (CameraRegs::TIMER_UPPER == reg)
	{
		m_lastExposureTimeRegHigh = val;
	}
	if (CameraRegs::TIMER_LOWER == reg)
	{
		m_lastExposureTimeRegLow = val;
	}

}

//////////////////////////// 
// GET  IMAGE   DATA
void AspenEthernetIo::GetImageData(std::vector<uint16_t> & ImageData)
{
    const int32_t NumBytesExpected = apgHelper::SizeT2Uint32(ImageData.size())*sizeof(uint16_t);

    //grab the data
    std::string fullUrl = m_url + "/aspen.bin?keyval=" + m_sessionKey;
    
    std::vector<uint8_t> result;
	// set container capacity so it doesn't waste time
	// dynamically allocating space
	result.reserve( NumBytesExpected );
	m_libcurl->setTimeout( 60 + getLastExposureTime() ); // set extended timeout
    m_libcurl->HttpGet( fullUrl, result );
	m_libcurl->setTimeout( -1 ); // restore default timeout

    if( NumBytesExpected !=  apgHelper::SizeT2Int32( result.size() ) )
    {
        std::stringstream msg;
        msg <<  fullUrl.c_str() << " error -  requested ";
        msg << NumBytesExpected << " bytes, but received ";
        msg << result.size() << " bytes.";

        apgHelper::throwRuntimeException( m_fileName, msg.str() , 
            __LINE__, Apg::ErrorType_Critical );
    }

    //faster than for loop
    memcpy(&(*ImageData.begin()), &(*result.begin()), NumBytesExpected);
}


//////////////////////////// 
// SETUP     IMG     XFER
void AspenEthernetIo::SetupImgXfer(uint16_t Rows,
            uint16_t Cols, 
            uint16_t NumOfImages, bool IsBulkSeq)
{
	std::string fullUrl;
    std::stringstream ss;
    std::stringstream FrameCount;


	if ( IsBulkSeq || NumOfImages == 1)
    {
		// download all as 1 image
		const uint32_t numPixels = Rows * Cols * NumOfImages;

		ss << numPixels;

		fullUrl = m_url + "/camcmd.cgi?req=Start_Image&imgSize=" +
			ss.str() + "&param=0" + m_sessionKeyUrlStr;
	}
	else
    {
		// download as a set of individual images
		const uint32_t numPixels = Rows * Cols;

		ss << numPixels;
		FrameCount << NumOfImages;

		fullUrl = m_url + "/camcmd.cgi?req=Start_Image&imgSize=" +
			ss.str() + "&frmCount=" + FrameCount.str() + 
			"&param=0" + m_sessionKeyUrlStr;
	}
      
    
    std::string result;
    m_libcurl->HttpGet( fullUrl, result );

    if( !IsResultOk( result ) )
    {
        std::string errMsg = "ERROR - command " + fullUrl + " failed.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }
}

//////////////////////////// 
//      CANCEL     IMG         XFER
void AspenEthernetIo::CancelImgXfer()
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=Stop_Image" + 
        m_sessionKeyUrlStr;
    
    std::string result;
    m_libcurl->HttpGet( fullUrl, result );

    if( !IsResultOk( result ) )
    {
        std::string errMsg = "ERROR - command " + fullUrl + " failed.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }
}

//////////////////////////// 
// GET      STATUS
void AspenEthernetIo::GetStatus(CameraStatusRegs::BasicStatus & status)
{
   CameraStatusRegs::AdvStatus advStatus;
 
   GetStatus( advStatus );

   status.CoolerDrive = advStatus.CoolerDrive;
   status.DataAvailFlag = advStatus.DataAvailFlag;
   status.FetchCount = advStatus.FetchCount;
   status.InputVoltage = advStatus.InputVoltage;
   status.SequenceCounter = advStatus.SequenceCounter;
   status.Status = advStatus.Status;
   status.TdiCounter = advStatus.TdiCounter;
   status.TempCcd = advStatus.TempCcd;
   status.TempHeatSink = advStatus.TempHeatSink;
   status.uFrame = advStatus.uFrame;
 
}

//////////////////////////// 
// GET      STATUS
void AspenEthernetIo::GetStatus(CameraStatusRegs::AdvStatus & status)
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=Get_Status" + m_sessionKeyUrlStr;

    std::vector<uint8_t> result;
    m_libcurl->HttpGet( fullUrl, result );

    status.TempHeatSink = ((result.at(0) << 8) | result.at(1));
    status.TempCcd = ((result.at(2) << 8) | result.at(3));
    status.CoolerDrive = ((result.at(4) << 8) | result.at(5));
    status.InputVoltage = ((result.at(6) << 8) | result.at(7));
    status.TdiCounter = ((result.at(8) << 8) | result.at(9));
    status.SequenceCounter = ((result.at(10) << 8) | result.at(11));
    status.Status = ((result.at(12) << 8) | result.at(13));
    status.uFrame = ((result.at(14) << 8) | result.at(15));
    status.MostRecentFrame = ((result.at(16) << 8) | result.at(17));
    status.ReadyFrame = ((result.at(18) << 8) | result.at(19));
    status.CurrentFrame = ((result.at(20) << 8) | result.at(21));
    status.FetchCount = ((result.at(22) << 24) | (result.at(23) << 16) | (result.at(24) << 8) | result.at(25));
    status.DataAvailFlag = result.at(26);

}

//////////////////////////// 
//  GET      MAC      ADDRESS
void AspenEthernetIo::GetMacAddress( std::string & Mac )
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=Get_Mac" + m_sessionKeyUrlStr;

    m_libcurl->HttpGet( fullUrl, Mac );
}

//////////////////////////// 
// WRITE        SRMD 
void AspenEthernetIo::WriteSRMD(const uint16_t reg, const std::vector<uint16_t> & data )
{
    std::vector<uint16_t>::const_iterator iter;
    for( iter = data.begin(); iter != data.end(); ++iter )
    {
        WriteReg( reg, *iter );
    }
}

//////////////////////////// 
// WRITE        MRMD 
void AspenEthernetIo::WriteMRMD(const uint16_t reg, const std::vector<uint16_t> & data )
{
     std::vector<uint16_t>::const_iterator iter;
    uint16_t offset = 0;
    for( iter = data.begin(); iter != data.end(); ++iter, ++offset )
    {
        WriteReg( reg+offset, *iter );
    }
}

//////////////////////////// 
//  GET    NETWORK     SETTINGS
std::string AspenEthernetIo::GetNetworkSettings()
{
    const std::string fullUrl = m_url + "/camcmd.cgi?req=Net_Param_Rd" + m_sessionKeyUrlStr;

    std::string info;
    m_libcurl->HttpGet( fullUrl, info );

    // TODO figure out what to do
   // with the pretty formatting.  g++ doesn't
   // seem to have full regex support
    //make the format pretty
//    std::regex rx("[\"\\}\\{ ]");
//    std::string replacement("");
//    info = std::regex_replace(info, rx, replacement);

    return info;
}

//////////////////////////// 
//  GET    LAST   EXPOSURE  TIME
unsigned int AspenEthernetIo::getLastExposureTime()
{
    uint32_t LastExpTime = ((uint32_t)m_lastExposureTimeRegHigh << 16) | (uint32_t)m_lastExposureTimeRegLow;
    return LastExpTime * 0.00000133; 
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string AspenEthernetIo::GetDriverVersion()
{
    CLibCurlWrap theCurl;
    return theCurl.GetVerison();
}


//////////////////////////// 
// GET       FIRMWARE    REV
uint16_t AspenEthernetIo::GetFirmwareRev()
{
    return ReadReg( CameraRegs::FIRMWARE_REV );
}


//////////////////////////// 
//  GET  INFO
std::string AspenEthernetIo::GetInfo()
{
    std::stringstream result;

    result << "Interface: Ethernet\n";
    result << "Camera Firmware: " << GetFirmwareRev() << "\n";
    result << GetNetworkSettings().c_str() << "\n";
    return result.str();
}

//////////////////////////// 
//      READ       STR     DATABASE
std::vector<std::string> AspenEthernetIo::ReadStrDatabase()
{
    // read the first 1280 bytes (the maximum that can be read from
    // the flash via ethernet see if there is anything in the str db
    // if so how big is it
    std::vector<uint8_t> out;
    
    if( !ReadStrDatabase( 0, MAX_STRDB_READ_BYTES, out ) )
    {
        std::string errMsg = "Initial read of str db via ethernet failed.";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );

    }

    const uint16_t totalSize = out.at(1) << 8 | out.at(0);

    if( CamStrDb::MAX_STR_DB_BYTES < totalSize )
    {
         //log error and return an empty vector
        std::stringstream ss;
        ss << "Read of Aspen string db returned an in valid size (" << totalSize << " ).  ";
        apgHelper::LogErrorMsg( __FILE__, ss.str(), __LINE__ );
        std::vector<std::string> out;
        return out;
    }

    // now read the full db
    std::vector<uint8_t> result( totalSize );

    uint32_t NumBytesExpected  =  totalSize;
    uint32_t offset = 0;

    while( NumBytesExpected > 0 )
    {
        uint32_t SizeToRead = std::min<uint32_t>(NumBytesExpected,
            MAX_STRDB_READ_BYTES );   
        
        std::vector<uint8_t> strDb;

        if( !ReadStrDatabase( offset, SizeToRead, strDb ) )
        {
             std::stringstream ss;
            ss << "Read string db failed at offset " << offset << " with a read of ";
            ss << SizeToRead << " bytes.";
            apgHelper::throwRuntimeException( m_fileName, ss.str(), 
                __LINE__, Apg::ErrorType_Critical );
        }

        std::vector<uint8_t>::iterator copyStart = result.begin()+offset;
        std::copy( strDb.begin(), strDb.end(), copyStart );

        NumBytesExpected -= SizeToRead;
        offset += SizeToRead; 
    }
   
   return CamStrDb::UnpackStrings( result );
}

//////////////////////////// 
//      READ       STR     DATABASE
bool AspenEthernetIo::ReadStrDatabase( const uint32_t offset, const uint32_t size2Read, 
                                      std::vector<uint8_t> & out )
{
    std::stringstream offsetSs;
    offsetSs << "offset=" << offset;

    std::stringstream bytesSs;
    bytesSs << "bytes=" << size2Read;

    std::string fullUrl = m_url + "/camcmd.cgi?req=get_str_db&" + offsetSs.str() + "&" + bytesSs.str() + m_sessionKeyUrlStr;
    
    m_libcurl->HttpGet( fullUrl, out );

    if( 0 == out.size() )
    {
        apgHelper::LogErrorMsg( __FILE__,"string db cmd return a zero size response", __LINE__ );
        return false;
    }

    //check if ERR 2 is in the response
    std::string testStr( reinterpret_cast<const char*>( &out.at(0) ), out.size() );
    if( std::string::npos != testStr.find("ERR 2") )
    {
        std::string errMsg( "Error returned with str db cmd: ");
        errMsg.append( testStr );
        apgHelper::LogErrorMsg( __FILE__, errMsg, __LINE__ );
        return false;
    }

    return true;
}



