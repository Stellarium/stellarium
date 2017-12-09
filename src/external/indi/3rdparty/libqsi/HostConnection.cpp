#include "HostConnection.h"


HostConnection::HostConnection(void)
{
	m_HostIO = NULL;
	m_iStatus = 0;
}


HostConnection::~HostConnection(void)
{
}

int HostConnection::ListDevices(std::vector<CameraID> & vID, CameraID::ConnProto_t protocol)
{
	std::vector<CameraID> USBcams;
	std::vector<CameraID> TCPcams;

	if (ImplementsProtocol(CameraID::CP_USB) && (protocol == CameraID::CP_All || protocol == CameraID::CP_USB))
	{
		m_HostIO_USB.ListDevices( USBcams );
		vID.insert(vID.end(), USBcams.begin(), USBcams.end());
	}

	if (ImplementsProtocol(CameraID::CP_TCP) && (protocol == CameraID::CP_All || protocol == CameraID::CP_TCP))
	{
		m_HostIO_TCP.ListDevices( TCPcams );
		vID.insert(vID.end(), TCPcams.begin(), TCPcams.end());		
	}
	
#ifndef NO_CYUSB
	if (ImplementsProtocol(CameraID::CP_CyUSB) && (protocol == CameraID::CP_All || protocol == CameraID::CP_CyUSB))
	{
		m_HostIO_CyUSB.ListDevices( USBcams );
		vID.insert(vID.end(), USBcams.begin(), USBcams.end());
	}
#endif
	
	return S_OK;
}

IHostIO* HostConnection::GetConnection(CameraID id)
{
	return GetConnection(id.ConnProto);
}

IHostIO* HostConnection::GetConnection(CameraID::ConnProto_t protocol)
{
	if (ImplementsProtocol(protocol))
	{
		switch (protocol)
		{
			case CameraID::CP_USB:
				return &m_HostIO_USB;
			case CameraID::CP_TCP:
				return &m_HostIO_TCP;
#ifndef NO_CYUSB
			case CameraID::CP_CyUSB:
				return &m_HostIO_CyUSB;
#endif
			default:
				return NULL;
		}		
	}
	else
		return NULL;
}

bool HostConnection::ImplementsProtocol(CameraID::ConnProto_t protocol)
{
	switch (protocol)
	{
		case CameraID::CP_USB:
			#ifdef ENABLEUSBCONNECTION
				return true;
			#else
				return false;
			#endif
		case CameraID::CP_TCP:
			#ifdef ENABLETCPCONNECTION
				return true;
			#else
				return false;
			#endif
		case CameraID::CP_CyUSB:
			#ifdef ENABLECYUSBCONNECTION
				return true;
			#else
				return false;
			#endif
		default:
			return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostConnection::Open( CameraID cID )
{	
	// Get connection protocol
	m_HostIO = GetConnection(cID);
	if (m_HostIO == NULL)
		return ERR_PKT_NoConnection;
	// Open device by serial number
	m_iStatus = m_HostIO->OpenEx( cID );
	if (m_iStatus != ALL_OK) 
	  return m_iStatus + ERR_PKT_OpenFailed;

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostConnection::Close( void )
{
  // Close current device
	if (m_HostIO != NULL)
		m_HostIO->Close();
  return ALL_OK;
}


