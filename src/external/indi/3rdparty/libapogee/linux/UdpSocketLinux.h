/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class UdpSocketLinux 
* \brief upd socket class for linux 
* 
*/ 


#ifndef UDPSOCKETLINUX_INCLUDE_H__ 
#define UDPSOCKETLINUX_INCLUDE_H__ 

#include "../UdpSocketBase.h"

class UdpSocketLinux : public UdpSocketBase
{ 
    public: 
        UdpSocketLinux();
        virtual ~UdpSocketLinux(); 

    protected:
        void CloseSocket();
}; 

#endif
