/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
*
* \brief namespace helper for the apg library 
* 
*/ 


#ifndef APGHELPER_INCLUDE_H__ 
#define APGHELPER_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdarg.h>
#include <stdint.h>

#include "CameraInfo.h" 

//#define DEBUGGING_CAMERA
//#define DEBUGGING_CAMERA_STATUS

//----------------------------------------------------------------

//trying to make a cross platform way to get rid of
//vc compiler warning C4100
//http://msdn.microsoft.com/en-us/magazine/cc163805.aspx
#define NO_OP_PARAMETER(x) { (x); }

//----------------------------------------------------------------
namespace apgHelper 
{ 

    void DebugMsg( const char *fmt, ... );
    void LogDebugMsg(const int size, const char *fmt, va_list ap);
	void LogVerboseMsg(const std::string & fileName,
	const std::string & msg, const int32_t lineNum, const std::string & type);

	void LogErrorMsg(const std::string & fileName,
			const std::string & msg, const int32_t lineNum);

    void LogWarningMsg(const std::string & fileName,
			const std::string & msg, const int32_t lineNum);
   /*!
    *  Creates a string with the following format filename(lineNumber):msgStr
    * \param [in] fileName 
    * \param [in] msgStr
    * \param [in] lineNum
    * \return std::string message.
    */
    std::string mkMsg(const std::string & fileName, 
        const std::string & msgStr, 
        const int32_t lineNum);

    /*!
    *  Creates a string with the following format filename(lineNumber):errStr:errorType
    * \param [in] fileName 
    * \param [in] errStr
    * \param [in] lineNum
    * \param [in] errType
    * \return std::string message.
    */
    std::string mkMsg(const std::string & fileName, 
        const std::string & errStr, 
        const int32_t lineNum,
        const Apg::ErrorType errType);


   /*!
    *  Takes the input, forms an error message, creates a std::runtime error, assigns
    *  the error message to the what string and finally throws the error.
    * \param [in] fileName 
    * \param [in] errStr
    * \param [in] lineNum
    * \param [in] errType
    */
    void throwRuntimeException(const std::string & fileName, 
        const std::string & errStr, 
        const int32_t lineNum,
        const Apg::ErrorType errType);

    void ApogeeSleep( uint32_t TimeInMs );

     Apg::LedState ConvertUShort2ApnLedState( const uint16_t value );

     bool IsEqual(double x, double y);

     uint32_t SizeT2Uint32( size_t value );
     int32_t SizeT2Int32( size_t value );
     uint16_t SizeT2Uint16( size_t value );
     uint8_t SizeT2Uint8( size_t value );

     int32_t OsInt2Int32( int value );

     std::string GetCfgFileName();
     std::string GetCamCfgDir();
     std::string GetCfgDir();
}

#endif
