/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class ApgLogger 
* \brief Singleton logging class for the apg library. 
* 
*/ 

#include "ApgLogger.h" 

#ifdef WIN_OS
    #include "windozeHelpers.h" 
    #include "LoggerWin.h" 
#else
    #include "linux/LoggerSyslog.h" 
#endif


//////////////////////////// 
// CTOR 
ApgLogger::ApgLogger() : m_level( ApgLogger::LEVEL_RELEASE )
{ 

    
#ifdef WIN_OS
        m_theLogger = std::shared_ptr<ILog>( new LoggerWin );
#else
    	m_theLogger = std::shared_ptr<ILog>( new LoggerSyslog );
#endif
   
    //TODO figure out how to read ini or another file
    //to set the logging level
} 

//////////////////////////// 
// DTOR 
ApgLogger::~ApgLogger() 
{ 
    
} 


//////////////////////////// 
//      WRITE 
void ApgLogger::Write(ApgLogger::Level level, 
            const std::string & type, const std::string & msg)
{
    if( level <= m_level )
    {
        std::string libapgStr ( "libapogee:" );
        if( 0 == msg.compare( 0, libapgStr.size(), libapgStr ) )
        {
            m_theLogger->Write(type,msg);
        }
        else
        {
            libapgStr.append( msg );
            m_theLogger->Write(type,libapgStr);
        }
    }
}

//////////////////////////// 
//      IsLevelVerbose
bool ApgLogger::IsLevelVerbose()
{
    return ( ApgLogger::LEVEL_VERBOSE == m_level ? true : false) ;
}
