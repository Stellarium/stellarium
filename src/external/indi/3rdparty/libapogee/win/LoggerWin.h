/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class LoggerWin 
* \brief Implementation class for logging with platform specific resources, Windows Event log for example.
*  This class wraps the Event Logging API for XP and eariler OS's http://msdn.microsoft.com/en-us/library/aa363652%28VS.85%29.aspx
*/ 


#ifndef LOGGERWIN_INCLUDE_H__ 
#define LOGGERWIN_INCLUDE_H__ 

#include "ILog.h" 
#include <string>

class LoggerWin : public ILog
{ 
    public: 
        LoggerWin();
        virtual ~LoggerWin(); 

        void Write(const std::string & type,
            const std::string & msg);

    private:
        HANDLE m_hEventLog;
}; 

#endif