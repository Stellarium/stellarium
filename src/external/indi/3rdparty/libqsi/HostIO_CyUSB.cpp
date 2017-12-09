/*****************************************************************************************
NAME
 HostIO_CyUSB

DESCRIPTION
 Cypress CyApi USB driver

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging, Inc.) 2014

REVISION HISTORY
 DRC 02.04.14 Original Version
 *****************************************************************************************/
#ifndef NO_CYUSB
#include "HostIO_CyUSB.h"
#include "QSI_Registry.h"
#include "QSI_Global.h"
#include "QSILog.h"
#include <vector>
#include <string>
#include <algorithm>
#include "libusb-1.0/libusb.h"

HostIO_CyUSB::HostIO_CyUSB(void)
{
	m_log = new QSILog(_T("QSIINTERFACELOG.TXT"), _T("LOGUSBTOFILE"), _T("CYUSB"));
	m_log->TestForLogging();
	QSI_Registry reg;
	//
	// Get USB timeouts in ms.
	//
	m_IOTimeouts.ShortRead		= SHORT_READ_TIMEOUT;
	m_IOTimeouts.ShortWrite		= SHORT_WRITE_TIMEOUT;
	m_IOTimeouts.StandardRead	= reg.GetUSBReadTimeout( READ_TIMEOUT );
	m_IOTimeouts.StandardWrite	= reg.GetUSBWriteTimeout( WRITE_TIMEOUT );
	m_IOTimeouts.ExtendedRead	= reg.GetUSBExReadTimeout( LONG_READ_TIMEOUT );
	m_IOTimeouts.ExtendedWrite	= reg.GetUSBExWriteTimeout( LONG_WRITE_TIMEOUT );
}

HostIO_CyUSB::~HostIO_CyUSB(void)
{
	m_log->Close();				// flush to log file
	m_log->TestForLogging();	// turn off logging is appropriate
	delete m_log;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::ListDevices(std::vector<CameraID> & vID )
{
	int		numDevs;
	USHORT	VID;
	USHORT	PID;

	m_log->Write(2, _T("ListDevices started."));
	numDevs = cyusb_open();

	for (int i = 0; i < (int)numDevs; i++) 
	{
		h = cyusb_gethandle(i);
		VID = cyusb_getvendor(h);
		PID = cyusb_getproduct(h);

		std::string SerialNum = std::string("None");
		GetSerialNumber(SerialNum);
		std::string SerialToOpen = SerialNum;
		
		std::string Desc = std::string("None");
		GetDesc(Desc);
		
		m_log->Write(2, _T("Dev %d:"), i);
		m_log->Write(2, _T(" SerialNumber=%s"), SerialNum.c_str());
		m_log->Write(2, _T(" Description=%s"), Desc.c_str());

		if (VID == QSICyVID && PID == QSICyPID && !SerialNum.empty() )
		{
			m_log->Write(2, 
			             _T("USB ListDevices found QSI Cy device at index: %d, Serial: %s, Description: %s"), 
						 i, 
			             SerialNum.c_str(), 
			             Desc.c_str()
			             );
			CameraID id(SerialNum, SerialToOpen, Desc, VID, PID, CameraID::CP_CyUSB);
			vID.push_back(id);
		}
	}
	
	cyusb_close();
	
	m_log->Write(2, _T("USB ListDevices Done. Number of devices found: %d"), numDevs);
	return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::OpenEx(CameraID cID)
{
	int		numDevs;
	USHORT	VID;
	USHORT	PID;
	bool	bFoundDevice = false;

	m_log->Write(2, _T("OpenEx name: %s"), cID.SerialToOpen.c_str());
	
	numDevs = cyusb_open();

	for (int i = 0; i < (int)numDevs; i++) 
	{
		h = cyusb_gethandle(i);
		VID = cyusb_getvendor(h);
		PID = cyusb_getproduct(h);

		std::string SerialNum = std::string("None");
		GetSerialNumber(SerialNum);
		std::string SerialToOpen = SerialNum;
		
		std::string Desc = std::string("None");
		GetDesc(Desc);

		m_log->Write(2, _T("Dev %d:"), i);
		m_log->Write(2, _T(" SerialNumber=%s"), SerialNum.c_str());
		m_log->Write(2, _T(" Description=%s"), Desc.c_str());

		if (VID == QSICyVID && PID == QSICyPID && SerialNum == cID.SerialToOpen )
		{
			m_log->Write(2, _T("USB Open found QSI Cy device at index: %d, Serial: %s, Description: %s"), 
							i, SerialNum.c_str(), Desc.c_str());
			bFoundDevice = true;
			break;
		}
	}
	
	if (bFoundDevice && cyusb_kernel_driver_active(h, 0) == 0 && cyusb_claim_interface(h, 0) == 0)
	{
		SetTimeouts(READ_TIMEOUT, WRITE_TIMEOUT);		
	}
	else
	{
		m_log->Write(2, "No devices matched");
	}

	m_log->Write(2, _T("OpenEx Done."));

	return bFoundDevice ? ALL_OK : ERR_USB_OpenFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::SetTimeouts(int dwReadTimeout, int dwWriteTimeout)
{
	if (dwReadTimeout  < MINIMUM_READ_TIMEOUT) dwReadTimeout = MINIMUM_READ_TIMEOUT;
	if (dwWriteTimeout < MINIMUM_WRITE_TIMEOUT) dwWriteTimeout = MINIMUM_WRITE_TIMEOUT;

	m_log->Write(2, _T("SetTimeouts %d ReadTimeout %d WriteTimeout"), dwReadTimeout, dwWriteTimeout);
	CyWriteTimeout = (ULONG) dwWriteTimeout;
	CyReadTimeout  = (ULONG) dwReadTimeout;
	m_log->Write(2,  _T("SetTimeouts Done."));

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::SetStandardReadTimeout ( int ulTimeout)
{
	m_IOTimeouts.StandardRead = ulTimeout;
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::SetStandardWriteTimeout( int ulTimeout)
{
	m_IOTimeouts.StandardWrite = ulTimeout;
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::Close()
{
	m_log->Write(2, _T("Close"));

	cyusb_release_interface(h, 0);
	cyusb_close();

	m_log->Write(2, _T("Close Done.") );
	m_log->Close();				// flush to log file
	m_log->TestForLogging();	// turn off logging if appropriate

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::Write(unsigned char * lpvBuffer, int dwBuffSize, int * lpdwBytes)
{
	int iStatus;
	
	m_log->Write(2, _T("Write %d bytes, Data:"), dwBuffSize);
	m_log->WriteBuffer(2, lpvBuffer, dwBuffSize, dwBuffSize, 256);
	iStatus = cyusb_bulk_transfer(h, 0x02, lpvBuffer, dwBuffSize, lpdwBytes, CyWriteTimeout);
	m_log->Write(2, _T("Write Done %d bytes written."), *lpdwBytes);

	return iStatus==0 ? ALL_OK : ERR_USB_WriteFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::Read(unsigned char * lpvBuffer, int dwBuffSize, int * lpdwBytesRead)
{
	int iStatus;

	m_log->Write(2, _T("Read buffer size: %d bytes"), dwBuffSize);
	iStatus = cyusb_bulk_transfer(h, 0x83, lpvBuffer, dwBuffSize, lpdwBytesRead, CyReadTimeout);
	m_log->Write(2, _T("Read Done %d bytes read, data: "), *lpdwBytesRead);
	m_log->WriteBuffer(2, lpvBuffer, dwBuffSize, *lpdwBytesRead, 256);
	return iStatus==0 ? ALL_OK : ERR_USB_ReadFailed;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::ResetDevice()
{
	m_log->Write(2, _T("ResetDevice"));

	m_log->Write(2, _T("ResetDevice Done.") );

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::Purge()
{

	m_log->Write(2, _T("Purge/Abort Started."));

	m_log->Write(2, _T("Purge/Abort Done.") );

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::GetReadWriteQueueStatus(int * lpdwAmountInRxQueue, int * lpdwAmountInTxQueue)
{
	m_log->Write(2, _T("Get RW Queue Depth"));
	*lpdwAmountInRxQueue=0;
	*lpdwAmountInTxQueue=0;
	m_log->Write(2, _T("Get RW Queue Depth Done. %d bytes read queue, %d bytes write queue."),
			*lpdwAmountInRxQueue, *lpdwAmountInTxQueue);

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::GetReadQueueStatus(int *lpdwAmountInRxQueue)
{
	m_log->Write(2, _T("GetReadQueueStatus"));
	*lpdwAmountInRxQueue=0;
	m_log->Write(2, _T("GetReadQueueStatus Done %d in Rx queue."), *lpdwAmountInRxQueue);

	return ALL_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
int HostIO_CyUSB::SetIOTimeout (IOTimeout ioTimeout)
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
	return SetTimeouts (iReadTO, iWriteTO)	;	
}

int HostIO_CyUSB::MaxBytesPerReadBlock()
{
	return 1024;
}

int HostIO_CyUSB::WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten)
{
	return Write(pBuff, iBuffLen, iBytesWritten);
}

int HostIO_CyUSB::ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead)
{
	int result;

	result = Read(pBuff, iBuffLen, iBytesRead);
	// Adjust bytes returned to fit logical packet size
	if (result == 0 && *iBytesRead > PKT_HEAD_LENGTH)
	{
		// Total packet length from host is 2 byte header and the remaining length
		// as specified in pBuff[1] (LEN field)
		*iBytesRead = PKT_HEAD_LENGTH + pBuff[1];
	}
	else
		*iBytesRead = 0;

	return result;
}

IOType HostIO_CyUSB::GetTransferType()
{
	return IOType_SingleRow;
}


int HostIO_CyUSB::GetSerialNumber(std::string & pSerial) 
{ 
	unsigned char buff[256]; 
	int err; 
	struct libusb_device_descriptor desc;
	
	err = cyusb_get_device_descriptor(h, &desc); 
	if (err < 0) 
		return err; 
	
	err = libusb_get_string_descriptor_ascii(h, desc.iSerialNumber, buff, sizeof(buff)); 
	if (err < 0) 
		return err; 

	pSerial = std::string((const char *)buff);
	return ALL_OK;
} 

int HostIO_CyUSB::GetDesc(std::string & pDesc) 
{ 
	unsigned char buff[256]; 
	int err; 
	struct libusb_device_descriptor desc;
	
	err = cyusb_get_device_descriptor(h, &desc); 
	if (err < 0) 
		return err; 
	
	err = libusb_get_string_descriptor_ascii(h, desc.iProduct, buff, sizeof(buff)); 
	if (err < 0) 
		return err; 

	pDesc = std::string((const char *)buff);
	return ALL_OK;
} 
#endif
