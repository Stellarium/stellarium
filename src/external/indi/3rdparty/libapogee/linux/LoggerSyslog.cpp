/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class LoggerSyslog 
* \brief class for writing *inx based operating system's syslog file 
* 
*/ 

#include "LoggerSyslog.h" 
#include <syslog.h>
#include <stdarg.h>


//////////////////////////// 
// CTOR 
LoggerSyslog::LoggerSyslog() 
{ 
   

} 

//////////////////////////// 
// DTOR 
LoggerSyslog::~LoggerSyslog() 
{ 
} 


//////////////////////////// 
//  WRITE
void LoggerSyslog::Write(const std::string & type,
            const std::string & msg)
{

    int priority = LOG_ERR;

    if( std::string::npos != type.find("error") )
    {
        priority = LOG_ERR;
    }

    if( std::string::npos != type.find("warn") )
    {
        priority = LOG_WARNING;
    }

    if( std::string::npos != type.find("info") )
    {
        priority = LOG_INFO;
    }

    //TODO - should we handle the fall through?
    syslog( priority, "%s", msg.c_str() );
   
}

