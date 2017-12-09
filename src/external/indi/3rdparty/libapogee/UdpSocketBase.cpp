/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class UdpSocketBase 
* \brief Base class for a upd socket that finds apogee devices on a subnet 
* 
*/ 

#include "UdpSocketBase.h" 
#include "apgHelper.h" 
#include <sstream>
#include <cstring> //for memset

#if defined (WIN_OS)
    #include "winsock2.h"
#else
	#include <netinet/in.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <stdexcept>
	const int32_t SOCKET_ERROR = -1;
    const int32_t INVALID_SOCKET = -1;
#endif


namespace
{
    const int32_t DISCOVERY_TIMEOUT_SECS = 10;
    const int32_t DISCOVERY_MAXBUFFER = (16 * 1024);
}

//////////////////////////// 
// CTOR 
UdpSocketBase::UdpSocketBase() : m_SocketDescriptor(INVALID_SOCKET),
                                 m_udpPacket(""),
                                 m_fileName(__FILE__),
                                 m_timeout(DISCOVERY_TIMEOUT_SECS),
                                 m_elapsedSec(0)

{ 
    
} 

//////////////////////////// 
// DTOR 
UdpSocketBase::~UdpSocketBase() 
{ 
} 

//////////////////////////// 
// SEARCH   4   APOGEE      DEVICES
std::vector<std::string> UdpSocketBase::Search4ApogeeDevices(const std::string & subnet,
                                           const uint16_t portNum)
{

    CreateSocket( portNum );

    SetSocketOptions();

    CreateUpdPacket();

    //blast a UPD message looking for all
    //Apogee devices on the subnet
    BroadcastMsg(subnet, portNum);

    //collect the return messages
    std::vector<std::string> msgs = GetReturnedMsgs();
    
    //use the platform specific close
    CloseSocket();

    return msgs;

}

//////////////////////////// 
//  CREATE  SOCKET
void UdpSocketBase::CreateSocket(uint16_t portNum)
{
    m_SocketDescriptor = socket( AF_INET, SOCK_DGRAM, 0 );

    if( INVALID_SOCKET == m_SocketDescriptor )
    {
        std::string errMsg("Failed to create a socket");
        apgHelper::throwRuntimeException(m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection);
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
	addr.sin_family	= AF_INET;
	addr.sin_port	 = htons( portNum );
    //INADDR_ANY puts your IP address automatically 
	addr.sin_addr.s_addr = INADDR_ANY;  

    //bind the socket to the input port number
    const int32_t result = bind(m_SocketDescriptor,reinterpret_cast<sockaddr *>(&addr),
        sizeof(struct sockaddr) );

    if(-1 == result )
    {
        apgHelper::throwRuntimeException(m_fileName, "Binding socket failed", 
            __LINE__, Apg::ErrorType_Connection);
    }     

}

//////////////////////////// 
//  SET     SOCKET      OPTIONS
void UdpSocketBase::SetSocketOptions()
{
	int32_t OptVal = 1;  
    const int32_t OptLen = sizeof(int32_t);

    int32_t result = setsockopt( m_SocketDescriptor, 
        SOL_SOCKET, SO_BROADCAST, 
        reinterpret_cast<char*>(&OptVal), OptLen);

    if( result )
	{
        std::stringstream ss;
        ss << result;
		
        std::string errMsg = "setsockopt failed with error " + ss.str();
        apgHelper::throwRuntimeException(m_fileName, errMsg, __LINE__,
            Apg::ErrorType_Connection );
	}
}


//////////////////////////// 
//  CREATE      UPD     PACKET
void UdpSocketBase::CreateUpdPacket()
{
    std::stringstream lineStream;

    lineStream << "Discovery::Request-Except: \"Apogee\"; ";
    lineStream << std::hex << std::showbase;
    lineStream << 0x12345678 << "; ";
    lineStream << std::dec << std::noshowbase;
    lineStream << 0 << "; ";
    lineStream << DISCOVERY_TIMEOUT_SECS / 2 << "; ";
    lineStream << 0 << "; ";
    lineStream << 0 << "\r\n\r\n";

    std::string lineStr = lineStream.str();

    std::stringstream ss;
    
    ss << "MIME-Version: 1.0\r\n";
    ss << "Content-Type: application/octet-stream\r\n";
    ss << "Content-Transfer-Encoding: binary\r\n";
    ss << std::hex << std::showbase;
    ss << "Content-Length: " << lineStr.size() << "\r\n";
    ss << "X-Project: Apogee\r\n"; 
    ss << "X-Project-Version: 0.1\r\n\r\n";
   
 
    m_udpPacket = ss.str() + lineStr;
}


//////////////////////////// 
//  BROADCAST   MSG
void UdpSocketBase::BroadcastMsg(const std::string & subnet,
            uint16_t portNum)
{

    struct hostent *he = gethostbyname( subnet.c_str() );

    if( !he )
    {
        std::string errMsg = "Failed to create hostent structure";
        apgHelper::throwRuntimeException(m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection);
    }

    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family	= AF_INET;
	servaddr.sin_port	= htons( portNum );
	servaddr.sin_addr	= *(reinterpret_cast<in_addr *>(he->h_addr) );

    int32_t result = sendto( m_SocketDescriptor, m_udpPacket.c_str(), 
        apgHelper::SizeT2Int32( m_udpPacket.size() ), 
        0, 
        reinterpret_cast<sockaddr *>(&servaddr), 
        sizeof(servaddr) );

    if( SOCKET_ERROR == result ) 
    {
        std::stringstream ss;
        ss << result;

        std::string errMsg = "sendto failed with error " + ss.str();
        apgHelper::throwRuntimeException(m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection );
    }
}

//////////////////////////// 
//  RECEIVED        MSG
std::vector<std::string> UdpSocketBase::GetReturnedMsgs()
{

    struct timeval tv;
	fd_set rset;

    std::vector<std::string> returnedStrs;

    //wait for responses
    for(m_elapsedSec = 0; m_elapsedSec < m_timeout; ++m_elapsedSec)
   {
        FD_ZERO( &rset );
		FD_SET( m_SocketDescriptor, &rset );

		//LT: putting this here, because it appears that
		//the structure is getting set to zero after the first
		//read on linux
		tv.tv_sec = 1;
		tv.tv_usec = 0;

        int32_t result = select( m_SocketDescriptor + 1, &rset, 0, 0, &tv );
        
    	if( SOCKET_ERROR == result ) 
		{
           std::stringstream ss;
           ss << result;
		
            std::string errMsg = "select failed with error " + ss.str();
            apgHelper::throwRuntimeException(m_fileName, errMsg, 
                __LINE__, Apg::ErrorType_Connection);
        }

        // return of 0 = time limit expired
        // return of > 0 total number of socket handles 
        // that are ready and contained in fd_set
        // there is data so lets go fectch it
        if( result > 0 )
        {
            returnedStrs.push_back( FetchMsgFromSocket() );
        }

    }

    return returnedStrs;
}


//////////////////////////// 
//  FETCH   MSG     FROM        SOCKET
std::string UdpSocketBase::FetchMsgFromSocket()
{
    std::vector<char> returnedMsg(DISCOVERY_MAXBUFFER);

    int32_t value = recvfrom(m_SocketDescriptor, 
        &(*returnedMsg.begin()), 
        apgHelper::SizeT2Int32( returnedMsg.size() ), 
        0, 
        0,
        0);

    if( SOCKET_ERROR == value ) 
    {
        std::string errMsg = "recvfrom socket operation failed";
        apgHelper::throwRuntimeException(m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection );
    }

    std::string msg( &(*returnedMsg.begin()) );
    return msg;
}
