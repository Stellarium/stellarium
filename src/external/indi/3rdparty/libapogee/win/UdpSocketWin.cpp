/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class UdpSocketWin 
* \brief Windows platform implemenation of the UdpSocketBase class for a upd socket that finds apogee devices on a subnet 
* 
*/ 

#include "UdpSocketWin.h" 
#include "apgHelper.h" 
#include <sstream>

//////////////////////////// 
// CTOR 
UdpSocketWin::UdpSocketWin() : m_fileName(__FILE__)
{ 
	// Try to start the Windows sockets library
    int32_t result = WSAStartup(MAKEWORD(1, 0), &m_WinSockData);
	if( result )
	{
        std::stringstream ss;
        ss << result;
		
        std::string errMsg = "WSAStartup failed with error " + ss.str();
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__,Apg::ErrorType_Connection );
	}

}

//////////////////////////// 
// DTOR 
UdpSocketWin::~UdpSocketWin() 
{ 
    int result = WSACleanup();

    if( result )
    {
        std::stringstream ss;
        ss << result;
		
        std::string errMsg = "WSACleanup failed with error " + ss.str();
        apgHelper::LogErrorMsg( __FILE__, errMsg, __LINE__);
    }
} 


//////////////////////////// 
// CLOSE    SOCKET 
void UdpSocketWin::CloseSocket()
{
    closesocket( m_SocketDescriptor );
}