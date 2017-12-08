/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
*
* \class ApgLogger 
* \brief Singleton logging class for the apg library. 
* 
*/ 


#ifndef APGLOGGER_INCLUDE_H__ 
#define APGLOGGER_INCLUDE_H__ 

#include <string>
#include <stdexcept>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

class ILog;

class ApgLogger 
{ 
    public: 
        enum Level
        {
            LEVEL_RELEASE,
            LEVEL_DEBUG,
            LEVEL_VERBOSE
        };

        static ApgLogger & Instance() 
        {
            static ApgLogger theApgLogger;
            return theApgLogger;
        }

        void Write(ApgLogger::Level level, 
            const std::string & type, const std::string & msg);

        ApgLogger::Level GetLogLevel() { return m_level; }

        void SetLogLevel( ApgLogger::Level newLevel ) { m_level = newLevel; }

        bool IsLevelVerbose();

    private:
        ApgLogger(); 
        ApgLogger(ApgLogger  const&); 
        ApgLogger & operator=(ApgLogger  const&); 
        ~ApgLogger(); 

        std::shared_ptr<ILog> m_theLogger;
        ApgLogger::Level m_level;
}; 

///////////////
//  CLASS   ERROR     LOGGER 
// Specialized class that only thrown by the logger sub system

 class LoggerError : public std::runtime_error 
 {
     public:
         LoggerError(const std::string & whatMsg) : std::runtime_error(whatMsg) { }
 };

#endif
