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


#ifndef LOGGERSYSLOG_INCLUDE_H__ 
#define LOGGERSYSLOG_INCLUDE_H__ 

#include "../ILog.h"
#include <string>

class LoggerSyslog : public ILog
{ 
    public: 
        LoggerSyslog();
        virtual ~LoggerSyslog(); 

        void Write(const std::string & type,
            const std::string & msg);
}; 

#endif
