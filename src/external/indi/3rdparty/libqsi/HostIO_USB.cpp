/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI (Quantum Scientific Imaging) 2005-2013 <dchallis@qsimaging.com>
 * 
 */

#include "HostIO_USB.h"
#include "QSI_Registry.h"
#include "QSI_Global.h"
#include "QSILog.h"

#if defined(USELIBFTD2XX) && defined(USELIBFTDIZERO)
	#error "Multiple FTDI stacks defined.  Use only one of libftdi and libftd2xx"
#endif

#if defined(USELIBFTDIZERO)
	#pragma message "libftdi-0.1 selected."
#elif defined(USELIBFTDIONE)
	#pragma message "libftdi-1.0 selected."
#elif defined (USELIBFTD2XX)
	#pragma message "libftd2xx selected."
#else
	#error "No ftdi library selected."
#endif

const ULONG QSI_FSVIDPID = 0x0403eb48;
const ULONG QSI_HSVIDPID = 0x0403eb49;

//****************************************************************************************
// CLASS FUNCTION DEFINITIONS
//****************************************************************************************

//////////////////////////////////////////////////////////////////////////////////////////
HostIO_USB::HostIO_USB(void)
{
	m_DLLHandle = NULL;
	m_bLoaded = false;
	m_iLoadStatus = 0;
	m_iUSBStatus = 0;
	QSI_Registry reg;
	//
	// Get USB timeouts in ms.
	//
	m_IOTimeouts.ShortRead = SHORT_READ_TIMEOUT;
	m_IOTimeouts.ShortWrite = SHORT_WRITE_TIMEOUT;
	m_IOTimeouts.StandardRead = reg.GetUSBReadTimeout( READ_TIMEOUT );
	m_IOTimeouts.StandardWrite = reg.GetUSBWriteTimeout( WRITE_TIMEOUT );
	m_IOTimeouts.ExtendedRead = reg.GetUSBExReadTimeout( LONG_READ_TIMEOUT );
	m_IOTimeouts.ExtendedWrite = reg.GetUSBExWriteTimeout( LONG_WRITE_TIMEOUT );

	this->m_log = new QSILog("QSIINTERFACELOG.TXT", "LOGUSBTOFILE", "USB");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_init(&m_ftdi);
	m_ftdiIsOpen = false;
#elif defined(USELIBFTD2XX)
	m_DeviceHandle = NULL;
#endif
	m_vidpids.clear();
	m_vidpids.push_back(VidPid(0x0403, 0xeb48));
	m_vidpids.push_back(VidPid(0x0403, 0xeb49));	
}

HostIO_USB::~HostIO_USB()
{
#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	ftdi_deinit(&m_ftdi);
#elif defined(USELIBFTD2XX)

#endif
	delete this->m_log;
}

int HostIO_USB::ListDevices( std::vector<CameraID> & vID )
{
	m_log->Write(2, "List All Devices Started");
	vID.clear();
#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)

	ftdi_device_list* devlist=NULL;
	ftdi_device_list* curdev=NULL;
	char  szMan[256];
	int count;
	char pDesc[USB_DESCRIPTION_LENGTH] = "";
	char pSerial[USB_SERIAL_LENGTH] = "";

	for (int j = 0; j < (int)m_vidpids.size(); j++)
	{
		m_iUSBStatus = ftdi_usb_find_all(&m_ftdi, &devlist, m_vidpids[j].VID, m_vidpids[j].PID);
		curdev = devlist;
		if (m_iUSBStatus > 0 )
		{
			count = m_iUSBStatus < USB_MAX_DEVICES ? m_iUSBStatus : USB_MAX_DEVICES;
			for (int i = 0; i < count; i++)
			{ 
				ftdi_usb_get_strings(&m_ftdi, curdev->dev, szMan, 256, pDesc, USB_DESCRIPTION_LENGTH, pSerial, USB_SERIAL_LENGTH);
				curdev = curdev->next;
				
				std::string SerialNum		= std::string(pSerial);
				// All upper case on Serial number, to find optional trailing "A" or "B"
				if (!SerialNum.empty()) std::transform(SerialNum.begin(), SerialNum.end(), SerialNum.begin(), toupper); 
				std::string SerialToOpen	= SerialNum;
				std::string Desc			= std::string(pDesc);

				if (!SerialNum.empty() && *SerialNum.rbegin() != 'B' )
				{
					m_log->Write(2, _T("USB ListDevices GetDeviceInfoList found QSI device at index: %d, Serial Number: %s, Description: %s"), 
										i, SerialToOpen.c_str(), Desc.c_str());
					
					// Must have "0403" and "eb48" or "eb49". Ignore B channels of dual devices.
					// Good QSI Camera
					// trim off "A" from dual channel devices description and serial number for display
					if (!SerialNum.empty() && *SerialNum.rbegin()  == 'A')
					{
						SerialNum = SerialNum.erase(SerialNum.find_last_not_of("A")+1);	// Remove trailing "A"
					}
					if (!Desc.empty() && *Desc.rbegin()  == 'A')
					{
						Desc = Desc.erase(Desc.find_last_not_of("A")+1);	// Remove trailing "A"
						Desc = Desc.erase(Desc.find_last_not_of(" ")+1);	// Trim right spaces
					}

					CameraID id(SerialNum, SerialToOpen, Desc, m_vidpids[j].VID, m_vidpids[j].PID, CameraID::CP_USB);
					vID.push_back(id);
				}
			}
			m_iUSBStatus = 0;
		}

		if (devlist != NULL)
			ftdi_list_free(&devlist);
	}
	
	m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are neg, we expect pos
	
#elif defined(USELIBFTD2XX)

	FT_STATUS ftStatus = 0;
	FT_DEVICE_LIST_INFO_NODE *devinfo;

	for (int j = 0; j < (int)m_vidpids.size(); j++)
	{
		DWORD numDevs = 0;
		ftStatus = FT_SetVIDPID(m_vidpids[j].VID, m_vidpids[j].PID);		
		ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
		ftStatus = FT_CreateDeviceInfoList( &numDevs );

		if (ftStatus !=FT_OK)
			numDevs = 0;

		if (numDevs > 0)
		{
			devinfo = (FT_DEVICE_LIST_INFO_NODE*)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE)*(numDevs+1));
			ftStatus = FT_GetDeviceInfoList(devinfo, &numDevs);
			if (ftStatus == FT_OK)
			{
				for (int i = 0; i < (int)numDevs; i++)
				{
					if (devinfo[i].ID != 0)
					{
						std::string SerialNum		= std::string(devinfo[i].SerialNumber);
						// All upper case on Serial number, to find optional trailing "A" or "B"
						if (!SerialNum.empty()) std::transform(SerialNum.begin(), SerialNum.end(), SerialNum.begin(), toupper); 
						std::string SerialToOpen	= SerialNum;
						std::string Desc			= std::string(devinfo[i].Description);

						if ((devinfo[i].ID == QSI_FSVIDPID || devinfo[i].ID == QSI_HSVIDPID) && !SerialNum.empty() && *SerialNum.rbegin() != 'B' )
						{
							m_log->Write(2, _T("USB ListDevices GetDeviceInfoList found QSI device at index: %d, Serial Number: %s, Description: %s"), 
												i, devinfo[i].SerialNumber, devinfo[i].Description);
					
							// Must have "0403" and "eb48" or "eb49". Ignore B channels of dual devices.
							// Good QSI Camera
							// trim off "A" from dual channel devices description and serial number for display
							if (!SerialNum.empty() && *SerialNum.rbegin()  == 'A')
							{
								SerialNum = SerialNum.erase(SerialNum.find_last_not_of("A")+1);	// Remove trailing "A"
							}
							if (!Desc.empty() && *Desc.rbegin()  == 'A')
							{
								Desc = Desc.erase(Desc.find_last_not_of("A")+1);	// Remove trailing "A"
								Desc = Desc.erase(Desc.find_last_not_of(" ")+1);	// Trim right spaces
							}

							CameraID id(SerialNum, SerialToOpen, Desc, (devinfo[i].ID >> 16) & 0x0000FFFF, devinfo[i].ID & 0x0000FFFF, CameraID::CP_USB);
							vID.push_back(id);
						}
					}
				}
			}
			free (devinfo);
		}
		m_iUSBStatus = ftStatus;
	}
#endif

	m_log->Write(2, "List All Devices done %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::OpenEx(CameraID  cID )
{
	m_log->Write(2, "OpenEx name: %s", cID.Description.c_str());

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_set_interface( &m_ftdi, INTERFACE_A ); 
	m_iUSBStatus |= ftdi_usb_open_desc(&m_ftdi, cID.VendorID, cID.ProductID, cID.Description.c_str(), cID.SerialNumber.c_str());
	if (m_iUSBStatus == 0)
		m_ftdiIsOpen = true;
	else
	{
		m_ftdiIsOpen = false;
		m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are neg, we expect pos
	}
	if (m_ftdiIsOpen)
	{
		if (cID.ProductID == 0xeb49 ) // high speed ftdi
		{
			m_iUSBStatus |= ftdi_set_bitmode(&m_ftdi, 0xff, BITMODE_RESET);
			usleep(10000);	// Delay 10ms as per ftdi example		
			m_iUSBStatus |= ftdi_set_bitmode(&m_ftdi, 0xff, BITMODE_SYNCFF);
			if (m_log->LoggingEnabled()) m_log->Write(2, _T("SetBitMode (HS) Done status: %x"), m_iUSBStatus);
		}

		m_iUSBStatus |= ftdi_setflowctrl(&m_ftdi, SIO_RTS_CTS_HS);

		if (m_iUSBStatus == 0)
			m_ftdiIsOpen = true;
		else
		{
			m_ftdiIsOpen = false;
			m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are neg, we expect pos
		}
	}

#elif defined(USELIBFTD2XX)
	m_iUSBStatus = FT_SetVIDPID(cID.VendorID, cID.ProductID);
	m_iUSBStatus |= FT_OpenEx((void*)cID.SerialToOpen.c_str(), FT_OPEN_BY_SERIAL_NUMBER, &m_DeviceHandle);

	if (cID.ProductID == 0xeb49 ) // high speed ftdi
	{
		m_iUSBStatus |= FT_SetBitMode(m_DeviceHandle, 0xff, FT_BITMODE_RESET);
		usleep(20000);	// Delay 10ms as per ftdi example
		m_iUSBStatus |= FT_SetBitMode(m_DeviceHandle, 0xff, FT_BITMODE_SYNC_FIFO);
		m_log->Write(2, _T("SetBitMode (HS) Done status: %x"), m_iUSBStatus);
	}
	// Startup Settings
	m_iUSBStatus |= FT_SetFlowControl(m_DeviceHandle, FT_FLOW_RTS_CTS, 0, 0);
	m_iUSBStatus |= FT_SetChars(m_DeviceHandle, false, 0, false, 0);

#endif

	if (m_iUSBStatus != ALL_OK)
	{
		m_log->Write(2, "OpenEx Failed status: %x", m_iUSBStatus);
		return m_iUSBStatus + ERR_PKT_OpenFailed;
	}
	//Now purge IO as per ftdi AN_130
	m_iUSBStatus = Purge();	

	// USB buffer size parameters
	QSI_Registry rReg;
	DWORD dwUSBInSize = rReg.GetUSBInSize( USB_IN_TRANSFER_SIZE );
	if (dwUSBInSize < 1000) dwUSBInSize = 0;		//Zero means leave at default value
	DWORD dwUSBOutSize = rReg.GetUSBOutSize( USB_OUT_TRANSFER_SIZE );
	if (dwUSBOutSize < 1000) dwUSBOutSize = 0;		//Zero means leave at default value
	
	// Set USB Driver Buffer Sizes
	m_iUSBStatus |= SetUSBParameters(dwUSBInSize, dwUSBOutSize);
#if defined(USELIBFTD2XX)
	m_iUSBStatus |= ResetDevice();
#endif
	// Latency Timer
	m_iUSBStatus |= SetLatencyTimer(LATENCY_TIMER_MS);
	// Set timeouts
	m_iUSBStatus |= SetTimeouts(READ_TIMEOUT, WRITE_TIMEOUT);
	
	if (m_iUSBStatus != ALL_OK)
	{
		m_log->Write(2, "OpenEx Failed status: %x", m_iUSBStatus);
		return m_iUSBStatus + ERR_PKT_OpenFailed;
	}
	
	m_log->Write(2, "OpenEx Done status: %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetTimeouts(int iReadTimeout, int iWriteTimeout)
{
	m_log->Write(2, "SetTimeouts %d ReadTimeout %d WriteTimeout", iReadTimeout, iWriteTimeout);
	if (iReadTimeout  < MINIMUM_READ_TIMEOUT) iReadTimeout = MINIMUM_READ_TIMEOUT;
	if (iWriteTimeout < MINIMUM_WRITE_TIMEOUT) iWriteTimeout = MINIMUM_WRITE_TIMEOUT;
	m_log->Write(2, "SetTimeouts set to %d ReadTimeout %d WriteTimeout", iReadTimeout, iWriteTimeout);
	
#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_ftdi.usb_read_timeout = iReadTimeout;
	m_ftdi.usb_write_timeout = iWriteTimeout;
	m_iUSBStatus = 0;
#elif defined(USELIBFTD2XX)
	m_iUSBStatus =  FT_SetTimeouts(m_DeviceHandle, iReadTimeout, iWriteTimeout);
#endif

	m_log->Write(2, "SetTimeouts Done %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetStandardReadTimeout ( int ulTimeout)
{
	m_IOTimeouts.StandardRead = ulTimeout;
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetStandardWriteTimeout( int ulTimeout)
{
	m_IOTimeouts.StandardWrite = ulTimeout;
	return SetTimeouts(m_IOTimeouts.StandardRead, m_IOTimeouts.StandardWrite);
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::Close()
{

	m_log->Write(2, "Close");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	if (m_ftdiIsOpen)
	{
		m_iUSBStatus = ftdi_usb_close(&m_ftdi);
		m_ftdiIsOpen = false;
	}
	ftdi_deinit(&m_ftdi);
	m_iUSBStatus = ftdi_init(&m_ftdi);
	m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are neg, we expect pos
#elif defined(USELIBFTD2XX)
	if (m_DeviceHandle != 0)
		m_iUSBStatus = FT_Close(m_DeviceHandle);
	else
		m_iUSBStatus = 0;
#endif

	m_log->Write(2, "Close Done status: %x", m_iUSBStatus);
	m_log->Close();				// flush to log file
	m_log->TestForLogging();	// turn off logging is appropriate

	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::Write(unsigned char * lpvBuffer, int dwBuffSize, int * lpdwBytes)
{
	m_log->Write(2, _T("Write %d bytes, Data:"), dwBuffSize);
	m_log->WriteBuffer(2, lpvBuffer, dwBuffSize, dwBuffSize, 256);

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_write_data(&m_ftdi, lpvBuffer, dwBuffSize);
	if (m_iUSBStatus >= 0)
	{
		*lpdwBytes = m_iUSBStatus;
		m_iUSBStatus = 0;
	}
	else
	{
		*lpdwBytes = 0;
		m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are negative, we expect positive
	}
#elif defined(USELIBFTD2XX)
	m_iUSBStatus =  FT_Write(m_DeviceHandle, lpvBuffer, dwBuffSize, (LPDWORD)lpdwBytes);
#endif

	m_log->Write(2, "Write Done %d bytes written, status: %x", *lpdwBytes, m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::Read(unsigned char * lpvBuffer, int dwBuffSize, int * lpdwBytesRead)
{
	m_log->Write(2, "Read buffer size: %d bytes", dwBuffSize);

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = my_ftdi_read_data(&m_ftdi, lpvBuffer, dwBuffSize);
	if (m_iUSBStatus > 0)
	{
		*lpdwBytesRead = m_iUSBStatus;
		m_iUSBStatus = 0;
	}
	else
	{
		*lpdwBytesRead = 0;
		m_iUSBStatus = -m_iUSBStatus; // Error codes from ftdi are negative, we expect positive
		if (m_iUSBStatus == 0) m_iUSBStatus = 4; // read returned with zero bytes.
		if (m_iUSBStatus == 4) m_log->Write(2, "***USB_Read Timeout***");
	}
#elif defined(USELIBFTD2XX)
	m_iUSBStatus = FT_Read(m_DeviceHandle, lpvBuffer, dwBuffSize, (LPDWORD)lpdwBytesRead);
#endif
	m_log->Write(2, _T("Read Done %d bytes read, status: %x, data: "), *lpdwBytesRead, m_iUSBStatus);
	m_log->WriteBuffer(2, lpvBuffer, dwBuffSize, *lpdwBytesRead, 256);

	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::GetReadQueueStatus(int * lpdwAmountInRxQueue)
{
	m_log->Write(2, "GetQueueStatus");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = 0;
	*lpdwAmountInRxQueue = m_ftdi.readbuffer_remaining;	
#elif defined(USELIBFTD2XX)
	m_iUSBStatus = FT_GetQueueStatus(m_DeviceHandle, (DWORD*)lpdwAmountInRxQueue);
#endif

	m_log->Write(2, "GetQueueStatus Done %d in Rx queue, status: %x", *lpdwAmountInRxQueue, m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::GetReadWriteQueueStatus(int * lpdwAmountInRxQueue, int * lpdwAmountInTxQueue)
{
	m_log->Write(2, "GetStatus of RX TX queues");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = 0;
	*lpdwAmountInRxQueue = m_ftdi.readbuffer_remaining;
	*lpdwAmountInTxQueue = 0;	
#elif defined(USELIBFTD2XX)
	DWORD dwDummy = 0;  				// Used in place of lpdwEventStatus
	m_iUSBStatus = FT_GetStatus(m_DeviceHandle, (DWORD*)lpdwAmountInRxQueue, (DWORD*)lpdwAmountInTxQueue, &dwDummy);
#endif

	m_log->Write(2, "GetStatus of RX TX queues done %d bytes read queue, %d bytes write queue, status: %x",
	             	*lpdwAmountInRxQueue, *lpdwAmountInTxQueue, m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetLatencyTimer(UCHAR ucTimer)
{
	m_log->Write(2, "SetLatencyTimer %0hx", ucTimer);

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_set_latency_timer(&m_ftdi, ucTimer);
	m_iUSBStatus = -m_iUSBStatus;
#elif defined(USELIBFTD2XX)
	m_iUSBStatus = FT_SetLatencyTimer(m_DeviceHandle, ucTimer);
#endif

	m_log->Write(2, "SetLatencyTimer Done status: %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::ResetDevice()
{
	m_log->Write(2, "ResetDevice");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_usb_reset(&m_ftdi);
	m_iUSBStatus = -m_iUSBStatus;
#elif defined(USELIBFTD2XX)
	m_iUSBStatus = FT_ResetDevice(m_DeviceHandle);
#endif

	m_log->Write(2, "ResetDevice Done status: %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::Purge()
{
	m_log->Write(2, "Purge() started.");

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = ftdi_usb_purge_buffers(&m_ftdi);
	m_iUSBStatus = -m_iUSBStatus;
#elif defined(USELIBFTD2XX)
	int iMask = FT_PURGE_RX | FT_PURGE_TX;
	m_iUSBStatus = FT_Purge(m_DeviceHandle, iMask);
#endif

	m_log->Write(2, "Purge Done status: %x", m_iUSBStatus);
	return m_iUSBStatus;
}

//////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetUSBParameters(DWORD dwInSize, DWORD dwOutSize)
{
	m_log->Write(2, "SetUSBParamters %d In Size, %d Out Size", dwInSize, dwOutSize);

#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	m_iUSBStatus = 0;
	if (dwInSize  != 0) 
		// m_iUSBStatus = ftdi_read_data_set_chunksize(&m_ftdi, dwInSize);
		ftdi_read_data_set_chunksize(&m_ftdi, 1<<14);
	if (dwOutSize != 0) 
		m_iUSBStatus += ftdi_write_data_set_chunksize(&m_ftdi, dwOutSize);
	m_iUSBStatus = -m_iUSBStatus;
#elif defined(USELIBFTD2XX)
	// SetUSBParameters corrupts memory on Fedora 17.  Disable this call
	// and use default setttings.
	//TODO current release of ftdi dll fails with this call. 
	//TODO m_iUSBStatus = FT_SetUSBParameters(m_DeviceHandle, dwInSize, dwOutSize);
#endif

	m_log->Write(2, "SetUSBParamters Done status: %x", m_iUSBStatus);
	return m_iUSBStatus;
}

/////////////////////////////////////////////////////////////////////////////////////////
int HostIO_USB::SetIOTimeout (IOTimeout ioTimeout)
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

int HostIO_USB::MaxBytesPerReadBlock()
{
	// Maximum number of pixels (not bytes) to read per block
	// Limited by ftdi constraints. 62 bytes of real data per packet, 510 for ft2232H
	// Max is 65536 BYTES total
	return 510 * 128 / 2;
}


int HostIO_USB::WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten)
{
	return Write(pBuff, iBuffLen, iBytesWritten);
}

int HostIO_USB::ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead)
{
	int iStatus;
	int dwBytesToRead;
	int dwBytesReturned;

	// Read command and length of Rx packet
	m_log->Write(2, _T("Read Returned Packet Header, 2 bytes to read."));
	iStatus = Read(pBuff, PKT_HEAD_LENGTH, &dwBytesReturned);
	if (iStatus != ALL_OK)
	{
		m_log->Write(2, _T("***Read Returned Packet Header Failed. Error code %x"), iStatus);
		iStatus += ERR_PKT_RxHeaderFailed;
		goto SendPacketExit;
	}
	// Make sure the entire Rx packet header was read
	if (dwBytesReturned != PKT_HEAD_LENGTH)
	{
		m_log->Write(2, _T("***Read Returned Packet Header Failed. Wrong number Bytes returned.  Returned %d Bytes"), dwBytesReturned);
		iStatus = ERR_PKT_RxHeaderFailed;
		goto SendPacketExit;
	}
	
	dwBytesToRead = (int)*(pBuff + PKT_LENGTH);

	// Make sure Rx packet isn't greater than allowed
	if (dwBytesToRead + PKT_HEAD_LENGTH > MAX_PKT_LENGTH)
	{
		m_log->Write(2, _T("***Read Returned Packet Header Failed. Packet Too Long, %d, Bytes"), dwBytesToRead + PKT_HEAD_LENGTH);
		iStatus = ERR_PKT_RxPacketTooLong;
		goto SendPacketExit;
	}
	// Get remaining data of Rx packet
	m_log->Write(2, _T("Read Remaining Packet Data, %d bytes to read."), dwBytesToRead);
	iStatus = Read(pBuff + PKT_HEAD_LENGTH, dwBytesToRead, &dwBytesReturned);
	if (iStatus != ALL_OK)
	{
		m_log->Write(2, _T("***Read Remaining Packeted Data Failed. Error Code %x"), iStatus);
		iStatus += ERR_PKT_RxFailed;
		goto SendPacketExit;
	}
	// Make sure the entire Rx packet was read
	if (dwBytesReturned == 0)
	{
		m_log->Write(2, _T("***Read Remaining Packeted Data Failed. Zero bytes returned."));
		iStatus = ERR_PKT_RxNone;
		goto SendPacketExit;
	}

	*iBytesRead = dwBytesReturned + PKT_HEAD_LENGTH;

SendPacketExit:
	return iStatus;
}

IOType HostIO_USB::GetTransferType()
{
	return IOType_MultiRow;
}


#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)

int HostIO_USB::my_ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size)
{
	const int uSECPERSEC = 1000000;
	const int uSECPERMILLISEC = 1000;

	int offset;
	int result;
	// Sleep interval, 1 microsecond
	timespec tm;
	tm.tv_sec = 0;
	tm.tv_nsec = 1000L;
	// Read timeout
	timeval startTime;
	timeval timeout;
	timeval now;

	gettimeofday(&startTime, NULL);
	// usb_read_timeout in milliseconds
	// Calculate read timeout time of day
	timeout.tv_sec = startTime.tv_sec + ftdi->usb_read_timeout / uSECPERMILLISEC;
	timeout.tv_usec = startTime.tv_usec + ((ftdi->usb_read_timeout % uSECPERMILLISEC)*uSECPERMILLISEC);
	if (timeout.tv_usec >= uSECPERSEC)
	{
		timeout.tv_sec++;
		timeout.tv_usec -= uSECPERSEC;
	}

	offset = 0;
	result = 0;

	while (size > 0)
	{
		result = ftdi_read_data(ftdi, buf+offset, size);
		if (result < 0) 
			break;
		if (result == 0)
		{
			gettimeofday(&now, NULL);
			if (now.tv_sec > timeout.tv_sec || (now.tv_sec == timeout.tv_sec && now.tv_usec > timeout.tv_usec))
				break;
			nanosleep(&tm, NULL); //sleep for 1 microsecond
			continue;
		}
		size -= result;
		offset += result;
	}
	return offset;
}

#elif defined(USELIBFTD2XX)

#endif
