/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class LoggerWin 
* \brief Implementation class for logging with platform specific resources, Windows Event log for example. 
*  This class is why we are using managed code.  The interface to the event log is MUCH better and
*  easier to use.  I am concerned that using manage code will create issues with the COM object.
*/ 
#include <windows.h>
#include "LoggerWin.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include "CameraInfo.h" // for Apg::ErrorType
#include "windozeHelpers.h"

//////////////////////////// 
// CTOR 
LoggerWin::LoggerWin() : m_hEventLog( NULL )
{ 
    m_hEventLog = RegisterEventSource( NULL, _T("Apogee") );

    if( NULL == m_hEventLog )
    {
        apgHelper::throwRuntimeException( __FILE__,
            "RegisterEventSource failed", __LINE__,
            Apg::ErrorType_Configuration );
    }
} 

//////////////////////////// 
// DTOR 
LoggerWin::~LoggerWin() 
{ 
} 


//////////////////////////// 
//  WRITE
void LoggerWin::Write(const std::string & type,
            const std::string & msg)
{

    WORD eventLogType = EVENTLOG_ERROR_TYPE;

    if( std::string::npos != type.find("error") )
    {
        eventLogType = EVENTLOG_ERROR_TYPE;
    }

    if( std::string::npos != type.find("warn") )
    {
        eventLogType = EVENTLOG_WARNING_TYPE;
    }

    if( std::string::npos != type.find("info") )
    {
       eventLogType = EVENTLOG_INFORMATION_TYPE;
    }

    
#ifdef DEBUGGING_CAMERA
    std::string msgWithNewline = msg + "\n";
    OutputDebugStringA( msgWithNewline.c_str() );
#else
    USES_CONVERSION;
    CString CStrMsg( A2T( msg.c_str() ) );
    LPCTSTR  lpMsg = CStrMsg;

    BOOL result = ReportEvent( m_hEventLog,
            eventLogType,
            0,
            0,
            NULL,
            1,
            0,
            &lpMsg,
            NULL );

    if( !result )
    {
        std::string lastError = windozeHelpers::GetLastWinError();
        std::string text = "Error ( " + lastError + " ) occurred writing to windows event log.";
        std::string errMsg = apgHelper::mkMsg(__FILE__, text ,__LINE__);
        LoggerError theException( errMsg );
        throw theException;
    }
#endif
}

