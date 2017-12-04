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

#include "LoggerException.h" 

//////////////////////////// 
// CTOR 
LoggerException::LoggerException(const std::string & msg) : std::runtime_error(msg)
{
}
