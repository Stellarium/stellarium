/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \brief namespace helper for the apg library 
* 
*/ 

#include "apgHelper.h" 
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
 #include <cmath> 
#include <stdio.h>
#include "ApgLogger.h" 
#include "helpers.h"

//CROSS-PLATFORM SLEEP
#ifdef WIN_OS
    #include <windows.h> // needed for Sleep
#else
    #include <unistd.h>
    #define Sleep(x) usleep((x)*1000)
#endif 

namespace
{
    int32_t NumDebugMsgTries = 0;
    const int32_t MAX_NUM_DEBUG_TRIES = 3;
}

using namespace std;

//////////////////////////// 
//      DEBUG       MSG 
void apgHelper::DebugMsg( const char *fmt, ... )
{
    #ifdef DEBUGGING_CAMERA
        // from http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf
        va_list ap;
        va_start( ap, fmt );
        apgHelper::LogDebugMsg( 1024, fmt, ap );
        va_end( ap );
    #endif
}

//----------------------------------------------
//  LOG      DEBUG      MSG
 void apgHelper::LogDebugMsg(const int size, const char *fmt, va_list ap)
 {
#ifdef DEBUGGING_CAMERA
     // from http://stackoverflow.com/questions/69738/c-how-to-get-fprintf-results-as-a-stdstring-w-o-sprintf
    std::vector<char> buf(size);
    
    // TODO - detemerine if I need to save a copy of va_list
    // to use later, ie is vsnprintf destructive???
    char *cbufPtr = &buf[0];
    int needed = vsnprintf(cbufPtr, size, fmt, ap);

    //handle the vsnprintf  -1 return case on microsoft compilers
    if(needed < 0) 
    {
        if( NumDebugMsgTries < MAX_NUM_DEBUG_TRIES)
        {
            //double the buffer size and try again
            ++NumDebugMsgTries;
            apgHelper::LogDebugMsg( size*2, fmt, ap );
        }
        else
        {
             NumDebugMsgTries = 0;

            //bailing don't want inifitely recuring loop
            apgHelper::throwRuntimeException( __FILE__, 
                "in infinite loop trying to log debug message", __LINE__,
                Apg::ErrorType_Critical );
        }
    }

    //handle the case where the number of chars are actually returned
    //by vsnprintf
    if (needed > size ) 
    {
        buf.resize( needed+1 );
        needed = vsnprintf( &buf[0], needed, fmt, ap); 
    }

    //we made it - log this message
    NumDebugMsgTries = 0;
    std::string msg( &buf[0] );
    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG, "debug", msg);
#endif
 }

//----------------------------------------------
//  LOG      VERBOSE       MSG
 void apgHelper::LogVerboseMsg(const std::string & fileName, 
        const std::string & msg, const int32_t lineNum, const std::string & type)
 {
     if( ApgLogger::Instance().IsLevelVerbose() )
     {
        string msg2Log = mkMsg(fileName, msg, lineNum);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_VERBOSE,type, msg2Log);
     }
 }

 //----------------------------------------------
 //  LOG      ERROR       MSG
  void apgHelper::LogErrorMsg(const std::string & fileName,
         const std::string & msg, const int32_t lineNum)
  {
	 string msg2Log = mkMsg(fileName, msg, lineNum);
	 ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error", msg2Log);
  }

 //----------------------------------------------
 //  LOG      WARNING       MSG
  void apgHelper::LogWarningMsg(const std::string & fileName,
         const std::string & msg, const int32_t lineNum)
  {
	 string msg2Log = mkMsg(fileName, msg, lineNum);
	 ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn", msg2Log);
  }
//----------------------------------------------
//  MK      MSG
string apgHelper::mkMsg( const string & fileName, 
                       const string & errStr, 
                       const int32_t lineNum)
{
    stringstream ss;
    ss << "libapogee:" << fileName << "("  << lineNum << "):" << errStr;
    return ss.str();
}

//----------------------------------------------
//  MK      MSG
string apgHelper::mkMsg( const string & fileName, 
                       const string & errStr, 
                       const int32_t lineNum, 
                       const Apg::ErrorType errType )
{
    string base = apgHelper::mkMsg( fileName, errStr, lineNum );
    
    stringstream ss;
    ss << ":" << errType;
    base.append( ss.str() );

    return base;
}

//----------------------------------------------
//  THROW       RUNTIME         EXCEPTION
void apgHelper::throwRuntimeException( const std::string & fileName, 
        const std::string & errStr, 
        const int32_t lineNum,
        const Apg::ErrorType errType )
{
    string whatStr = mkMsg(fileName, errStr, lineNum, errType);

    try
    {
        //log all exceptions
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE, "error", whatStr);
    }
    catch(LoggerError & logException )
    {
        //add the logging failure information to the what that is thrown
        std::string logErrMsg("\nLogging Exception: ");
        logErrMsg.append( logException.what() );
        whatStr.append( logErrMsg );
    }

    runtime_error except( whatStr );
    throw except;
}


//----------------------------------------------
//  APOGEE    SLEEP
void apgHelper::ApogeeSleep( uint32_t TimeInMs )
{
    Sleep( TimeInMs );
}

//----------------------------------------------
//  CONVERT      USHORT    2   APNLEDSTATE
Apg::LedState apgHelper::ConvertUShort2ApnLedState( const uint16_t value )
{
    Apg::LedState result = Apg::LedState_Expose;
    switch( value )
    {
        case 0:
            result = Apg::LedState_Expose;
        break;

        case 1:
            result = Apg::LedState_ImageActive;
        break;

        case 2:
            result = Apg::LedState_Flushing;
        break;
        
        case 3:
            result = Apg::LedState_ExtTriggerWaiting;
        break;

        case 4:
            result = Apg::LedState_ExtTriggerReceived;
        break;

        case 5:
            result = Apg::LedState_ExtShutterInput;
        break;

        case 6:
            result = Apg::LedState_ExtStartReadout;
        break;

        case 7:
            result = Apg::LedState_AtTemp;
        break;

        default:
         {
            std::stringstream msg;
            msg << "Undefine led state: " << value;
            apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
                Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    return result;
}

//----------------------------------------------
//  APOGEE    SLEEP
bool apgHelper::IsEqual(const double x, const double y)
 {
    //from http://www.parashift.com/c++-faq-lite/newbie.html
    const double epsilon = 0.000001;
    return std::abs(x - y) <= epsilon * std::abs(x);
    // see Knuth section 4.2.2 pages 217-218
 }

//----------------------------------------------
//      SIZET      2      UINT32
uint32_t apgHelper::SizeT2Uint32( const size_t value )
{
    const uint32_t MAX_VALUE = 4294967295;

    if( value > MAX_VALUE  )
    {
        std::stringstream msg;
        msg << "SizeT2Uint32 conversion failed size = " << value;
        apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    return static_cast<uint32_t>( value );
}

//----------------------------------------------
//      SIZET      2      INT32
int32_t apgHelper::SizeT2Int32( const size_t value )
{
    const int32_t MAX_VALUE =  2147483647;

    if( value > MAX_VALUE  )
    {
        std::stringstream msg;
        msg << "SizeT2Int32 conversion failed size = " << value;
        apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    return static_cast<int32_t>( value );
}
 
//----------------------------------------------
//      SIZET      2      UINT16
uint16_t apgHelper::SizeT2Uint16( size_t value )
{
    const uint16_t MAX_VALUE =  65535;

    if( value > MAX_VALUE  )
    {
        std::stringstream msg;
        msg << "SizeT2Uint16 conversion failed size = " << value;
        apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    return static_cast<uint16_t>( value );
}

//----------------------------------------------
//      SIZET      2      UINT8
uint8_t apgHelper::SizeT2Uint8( const size_t value )
{
    const uint8_t MAX_VALUE =  255;

    if( value > MAX_VALUE  )
    {
        std::stringstream msg;
        msg << "SizeT2Uint8 conversion failed size = " << value;
        apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    return static_cast<uint8_t>( value );
}

//----------------------------------------------
//      OS   INT      2      INT32
int32_t apgHelper::OsInt2Int32( const int value )
{
    const int32_t MAX_VALUE =  2147483647;

    if( value > MAX_VALUE  )
    {
        std::stringstream msg;
        msg << "OsInt2Int32 conversion failed size = " << value;
        apgHelper::throwRuntimeException(__FILE__, msg.str(), __LINE__,
            Apg::ErrorType_InvalidUsage );
    }

    return static_cast<int32_t>( value );
}

//----------------------------------------------
//  GET    CFG     FILENAME
std::string apgHelper::GetCfgFileName()
{
    return std::string ( "apnmatrix.txt" );
}

//----------------------------------------------
//  GET    CAM        CFG    PATH
std::string apgHelper::GetCamCfgDir()
{
    std::string result = help::FixPath( apgHelper::GetCfgDir() );
    result.append( "camera/" );
    return result;

}
//CROSS-PLATFORM CFG DIR
#ifdef WIN32
#include <ShlObj.h>
#include <atlstr.h>
//----------------------------------------------
//  GET    CFG         DIR
std::string apgHelper::GetCfgDir()
{
    // http://msdn.microsoft.com/en-us/library/windows/desktop/bb762181(v=vs.85).aspx
    // MS has marked the SHGetFolderPath function depreciated, but there is 
    // no other option for XP.  Decide to go with the simpliest function here, instead doing
    // delayed dll load and such to get different behavoir for different Windoze OS's
    // need to switch this to SHGetKnownFolderPath when we drop windows XP support
    // or the function is removed from shell32.dll, which ever happens first....
    TCHAR szPath[MAX_PATH];
    if( FAILED( SHGetFolderPath(  NULL, 
                        CSIDL_COMMON_APPDATA,
                        NULL, 
                        0,
                        szPath ) ) )
    {
        apgHelper::throwRuntimeException(__FILE__, 
        "Failed fetching the configuration file directory", 
        __LINE__,
        Apg::ErrorType_Configuration );
    }

    USES_CONVERSION;
    std::string path;
    path.append( help::FixPath( T2A(szPath) ) );
    path.append( "Apogee/");
    return path;
}
#else

// for linx and mac
#ifndef SYSCONFDIR
# define SYSCONFDIR "/usr/local/etc/"
#endif

// SYSCONFDIR set by autotools on configuraiton
const char * sysconfdir = SYSCONFDIR;

//----------------------------------------------
//  GET    CFG         DIR
std::string apgHelper::GetCfgDir()
{
    std::string path( help::FixPath( sysconfdir ) );
    path.append( "Apogee/");
    return path;
}

#endif
