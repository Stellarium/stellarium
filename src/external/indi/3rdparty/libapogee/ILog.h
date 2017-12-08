/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class ILog 
* \brief Interface class for accessing platform specific logging resources, Windows Event log for example. 
* 
*/ 


#ifndef ILOG_INCLUDE_H__ 
#define ILOG_INCLUDE_H__ 

#include <string>

class ILog 
{ 
    public: 
        virtual ~ILog(); 
        virtual void Write(const std::string & type,
            const std::string & msg) = 0;
}; 

#endif
