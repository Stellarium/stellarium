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

#include "UdpSocketLinux.h" 

#include <sys/socket.h>
#include <unistd.h>

const int INVALID_SOCKET = -1;


//////////////////////////// 
// CTOR 
UdpSocketLinux::UdpSocketLinux()
{ 


}

//////////////////////////// 
// DTOR 
UdpSocketLinux::~UdpSocketLinux() 
{ 
    
} 


//////////////////////////// 
// CLOSE    SOCKET 
void UdpSocketLinux::CloseSocket()
{
    close( m_SocketDescriptor );
    m_SocketDescriptor = INVALID_SOCKET;
}
