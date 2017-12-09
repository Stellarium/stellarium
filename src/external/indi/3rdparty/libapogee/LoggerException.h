/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class LoggerException 
* \brief specialized exception for dealing errors in the logging sub-system 
* 
*/ 


#ifndef LOGGEREXCEPTION_INCLUDE_H__ 
#define LOGGEREXCEPTION_INCLUDE_H__ 

#include <stdexcept>
#include <string>

class LoggerException : public std::runtime_error
{ 
    public: 
        LoggerException(const std::string & msg);

}; 

#endif
