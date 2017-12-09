#include "HostIO_TCP.h"
#include "QSI_Registry.h"
#include "QSI_Global.h"

HostIO_TCP::HostIO_TCP(void)
{
	m_log = new QSILog(_T("QSIINTERFACELOG.TXT"), _T("LOGTCPTOFILE"), _T("TCP"));
	m_log->TestForLogging();
	QSI_Registry reg;
	//
	// Get timeouts in ms.
	//
	m_IOTimeouts.ShortRead = SHORT_READ_TIMEOUT;
	m_IOTimeouts.ShortWrite = SHORT_WRITE_TIMEOUT;
	m_IOTimeouts.StandardRead = reg.GetUSBReadTimeout( READ_TIMEOUT );
	m_IOTimeouts.StandardWrite = reg.GetUSBWriteTimeout( WRITE_TIMEOUT );
	m_IOTimeouts.ExtendedRead= reg.GetUSBExReadTimeout( LONG_READ_TIMEOUT );
	m_IOTimeouts.ExtendedWrite = reg.GetUSBExWriteTimeout( LONG_WRITE_TIMEOUT );

	m_ReadTimeout = READ_TIMEOUT;
	m_WriteTimeout = WRITE_TIMEOUT;

	m_TCP_Stack_OK = false;

#ifdef WIN32
	int iResult = WSAStartup(MAKEWORD(2,2), &m_wsaData);
	switch (iResult)
	{
		case NO_ERROR:
			m_log->Write(2, _T("TCP/IP WSAStartup OK"));
			m_TCP_Stack_OK = true;
			break;
		case WSASYSNOTREADY:		// The underlying network subsystem is not ready for network communication.
			m_log->Write(2, _T("TCP/IP The underlying network subsystem is not ready for network communication."));
			break;
		case WSAVERNOTSUPPORTED:	// The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation.
			m_log->Write(2, _T("TCP/IP The version of Windows Sockets support requested is not provided by this particular Windows Sockets implementation."));
			break; 
		case WSAEINPROGRESS:		// A blocking Windows Sockets 1.1 operation is in progress.
			m_log->Write(2, _T("TCP/IP A blocking Windows Sockets 1.1 operation is in progress."));
			break; 
		case WSAEPROCLIM :			// A limit on the number of tasks supported by the Windows Sockets implementation has been reached.
			m_log->Write(2, _T("TCP/IP A limit on the number of tasks supported by the Windows Sockets implementation has been reached."));
			break; 
		case WSAEFAULT:				//	The lpWSAData parameter is not a valid pointer.
			m_log->Write(2, _T("TCP/IP The lpWSAData parameter is not a valid pointer."));
			break;
		default:
			m_log->Write(2, _T("TCP/IP WSAStartup Unknown error code returned."));
			break;
	}
#else
	m_TCP_Stack_OK = true;
#endif 
	m_sock = 0;
	m_log->Write(2, _T("TCP/IP Constructor Done."));
}


HostIO_TCP::~HostIO_TCP(void)
{
#ifdef WIN32
	WSACleanup();
#endif
	m_log->Write(2, _T("TCP/IP Destructor."));
	m_log->Close();				// flush to log file
	m_log->TestForLogging();	// turn off logging is appropriate
	delete m_log;
}

int HostIO_TCP::ListDevices(std::vector<CameraID> &vID)
{
	QSI_Registry reg;
	vID.clear();
//
// TODO
// No TCP devices for now
//
#ifdef HASTCPIP
	in_addr ipAddr;	
	ipAddr.S_un.S_addr = reg.GetIPv4Addresss(true, MAKEIPADDRESS(0,0,0,0));
	CameraID cID("", ipAddr);
	vID.push_back(cID);

	ipAddr.S_un.S_addr = reg.GetIPv4Addresss(false, MAKEIPADDRESS(0,0,0,0));
	CameraID cgID("", ipAddr);
	vID.push_back(cgID);
#endif
	m_log->Write(2, _T("TCP/IP ListDevices Done."));
	return 0;
}

int HostIO_TCP::OpenEx(CameraID cID)
{
	int iError;
	u_long IO_NONBLOCK = 1;
	u_long IO_BLOCK = 0;
	struct timeval tv;
	// 5 second timeout
	tv.tv_sec = 5;	
    tv.tv_usec = 0;
	fd_set wfdset;
	fd_set rfdset;

	if (cID.IPv4Addr.s_addr == 0)
	{
		m_log->Write(2, _T("TCP/IP address is zero. Open failed."));
		return ERR_PKT_OpenFailed;
	}

	if (!m_TCP_Stack_OK)
	{
		m_log->Write(2, _T("TCP/IP WSAStartup failed. No stack available. Open failed."));
		return ERR_PKT_OpenFailed;
	}

	// Create a SOCKET for connecting to server
	m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock < 0)
	{
		TCPIP_ErrorDecode();		
		m_log->Write(2, _T("TCP/IP: Error at socket(): %d."), m_sock) ;
		return ERR_PKT_OpenFailed;
	}

	iError = ioctl(m_sock, FIONBIO, &IO_NONBLOCK);
	if (iError < 0)
	{
		TCPIP_ErrorDecode();		
		m_log->Write(2, _T("TCP/IP: Error at ioctl(FIONBIO): %d."), m_sock) ;
		return ERR_PKT_OpenFailed;
	}
	
	m_log->Write(2, _T("TCP/IP: socket() is OK.") );
 
	// The sockaddr_in structure specifies the address family,
	// IP address, and port of the server to be connected to.
	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_addr.s_addr = htonl(cID.IPv4Addr.s_addr);
	clientService.sin_port = htons(27727);
	// Connect to server.
	if ( connect(m_sock, (sockaddr*)&clientService, sizeof(clientService)) < 0)
	{
		m_log->Write(2, _T("TCP/IP: Failed to connect."));
		return ERR_PKT_OpenFailed;
	}

    FD_ZERO(&rfdset);
    FD_SET(m_sock, &rfdset);

	FD_ZERO(&wfdset);
    FD_SET(m_sock, &wfdset);

	iError = select(m_sock + 1, &rfdset, &wfdset, NULL, &tv);

	if (iError == 0)
	{
		// Select timout with no connection.
		close(m_sock);
		m_log->Write(2, _T("TCP/IP: Failed to connect after select timeout."));
		return ERR_PKT_OpenFailed;
	}

	if (iError == -1 )
	{
        TCPIP_ErrorDecode();
		m_log->Write(2, _T("TCP/IP: Failed to select."));
		close(m_sock);
		return ERR_PKT_OpenFailed;
	}

	ioctl(m_sock, FIONBIO, &IO_BLOCK);
	SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
	m_log->Write(2, _T("TCP/IP: connect() is OK.") );
	
	return 0;
}

int HostIO_TCP::SetTimeouts(int RxTimeout, int TxTimeout)
{
	m_log->Write(2, _T("TCP/IP SetTimeouts %d ReadTimeout %d WriteTimeout"), RxTimeout, TxTimeout);

	if (RxTimeout  < MINIMUM_READ_TIMEOUT) RxTimeout = MINIMUM_READ_TIMEOUT;
	if (TxTimeout < MINIMUM_WRITE_TIMEOUT) TxTimeout = MINIMUM_WRITE_TIMEOUT;

    if (setsockopt (m_sock, SOL_SOCKET, SO_RCVTIMEO, (char *)&RxTimeout, sizeof(RxTimeout)    )     < 0)
	{
		TCPIP_ErrorDecode();
        m_log->Write(2, _T("setsockopt SO_RCVTIMEO failed"));
		return ERR_PKT_SetTimeOutFailed;
	}

    if (setsockopt (m_sock, SOL_SOCKET, SO_SNDTIMEO, (char *)&TxTimeout, sizeof(TxTimeout)    )     < 0)
	{
		TCPIP_ErrorDecode();
        m_log->Write(2, _T("setsockopt SO_SNDTIMEO failed"));
		return ERR_PKT_SetTimeOutFailed;
	}

	m_log->Write(2,  _T("TCP/IP SetTimeouts Done."));
	return 0;
}

int HostIO_TCP::Close()
{
	close(m_sock);
	m_log->Write(2,  _T("TCP/IP Close Done."));
	return 0;
}

int HostIO_TCP::Write(unsigned char * sendBuf, int bytesToSend, int *bytesSent )
{
	// Send and receive data.
	*bytesSent = send(m_sock, reinterpret_cast<const char *>(sendBuf), bytesToSend, 0);
	if (*bytesSent == -1)
	{
		TCPIP_ErrorDecode();
		m_log->Write(2, _T("TCP/IP: write failed."));
		return ERR_PKT_RxFailed;
	}
	m_log->Write(2, _T("TCP/IP: Bytes sent: %ld"), *bytesSent);
	return 0;
}

int HostIO_TCP::Read(unsigned char * recvBuf, int bytesRequested, int * bytesReceived)
{
	*bytesReceived = recv(m_sock, reinterpret_cast<char *>(recvBuf), bytesRequested, 0);
	if (*bytesReceived == -1)
	{
		TCPIP_ErrorDecode();
		m_log->Write(2,  _T("TCP/IP Read Failed. %d Status Returned."), *bytesReceived);
		return ERR_PKT_RxFailed;
	}

	m_log->Write(2,  _T("TCP/IP Read Done. %d Bytes Returned."), *bytesReceived);
	return 0;
}

int HostIO_TCP::GetReadWriteQueueStatus(int * RxBytes, int * TxBytes)
{
	int iStatus = GetReadQueueStatus(RxBytes);
	// TODO
	TxBytes = 0;
	m_log->Write(2,  _T("TCP/IP GetReadWriteQueueStatus Done."));
	return iStatus;
}

int HostIO_TCP::ResetDevice()
{
	m_log->Write(2,  _T("TCP/IP ResetDevice Done."));
	return 0;
}

int HostIO_TCP::Purge()
{
	const int MAX_FRAME_SIZE = 9000;
	unsigned char buff[MAX_FRAME_SIZE];
	int iBytesAvailable;
	int iBytesReceived;

	while (GetReadQueueStatus(&iBytesAvailable) == 0 && iBytesAvailable > 0)
	{
		Read(buff, iBytesAvailable < MAX_FRAME_SIZE ? iBytesAvailable:MAX_FRAME_SIZE, &iBytesReceived);
	}

	m_log->Write(2,  _T("TCP/IP Purge Done."));
	return 0;
}

int HostIO_TCP::GetReadQueueStatus(int * bytesAvailable)
{
	int iError;
	*bytesAvailable = 0;

	iError = ioctl(m_sock, FIONREAD, (ULONG *)bytesAvailable);
	if (iError == -1)
	{
        TCPIP_ErrorDecode();
		m_log->Write(2, _T("TCP/IP: Failed to FIONREAD."));
		close(m_sock);
		return ERR_PKT_OpenFailed;
	}

	m_log->Write(2,  _T("TCP/IP ReadQueueStatus Done."));
	return 0;
}

int HostIO_TCP::SetStandardReadTimeout ( int ulTimeout)
{
	m_IOTimeouts.StandardRead = ulTimeout;
	m_log->Write(2,  _T("TCP/IP SetStandardReadTimeouts Done."));
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

int HostIO_TCP::SetStandardWriteTimeout( int ulTimeout)
{
	m_IOTimeouts.StandardWrite = ulTimeout;
	m_log->Write(2,  _T("TCP/IP SetStandardWriteTimeouts Done."));
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

int HostIO_TCP::SetIOTimeout(IOTimeout ioTimeout)
{
	int iReadTO;
	int	iWriteTO;
	switch (ioTimeout)
	{
	case IOTimeout_Normal:
			iReadTO  = m_IOTimeouts.StandardRead;
			iWriteTO = m_IOTimeouts.StandardWrite;
			break;
	case IOTimeout_Short:
			iReadTO  = m_IOTimeouts.ShortRead;
			iWriteTO = m_IOTimeouts.ShortWrite;
			break;
	case IOTimeout_Long:
			iReadTO  = m_IOTimeouts.ExtendedRead;
			iWriteTO = m_IOTimeouts.ExtendedWrite;
			break;
		default:
			iReadTO  = m_IOTimeouts.StandardRead;
			iWriteTO = m_IOTimeouts.StandardWrite;
			break;
	}
	m_log->Write(2,  _T("TCP/IP SetIOTimeouts Done."));
	return SetTimeouts (iReadTO, iWriteTO)	;	
}

int HostIO_TCP::MaxBytesPerReadBlock()
{
	return 65536;
}

int HostIO_TCP::WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten)
{
	return 0;
}

int HostIO_TCP::ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead)
{
	return 0;
}

IOType HostIO_TCP::GetTransferType()
{
	return IOType_MultiRow;
}

int HostIO_TCP::TCPIP_ErrorDecode()
{
	return 0;
}

