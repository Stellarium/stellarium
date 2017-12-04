/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) QSI (Quantum Scientific Imaging) 2005-2013 <dchallis@qsimaging.com>
 * 
 */
#pragma once
//
// Select the approriate ftdi library. 
// use "./configure --with-ftd=ftd2xx" to switch to libftd2xx.
// and "./configure --with-ftd=ftdi" for the open source stack.
//
#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	#include <ftdi.h>
	#define FT_PURGE_TX 1
	#define FT_PURGE_RX 2
#elif defined (USELIBFTD2XX)
	#include <ftd2xx.h>
#else

#endif
//

#include "IHostIO.h"
#include "QSILog.h"
#include "QSI_Global.h"
#include "VidPid.h"

// FTDI buffering size, Zero means leave as default
// Transfer size in bytes.
const int USB_IN_TRANSFER_SIZE = 64 * 1024; // Max allowed by fdti
const int USB_OUT_TRANSFER_SIZE = 64 * 1024;
const int LATENCY_TIMER_MS = 16;

const int USB_SERIAL_LENGTH = 32; // Length of character array to hold device's USB serial number
const int USB_DESCRIPTION_LENGTH = 32;
const int USB_MAX_DEVICES = 128;

class HostIO_USB : public IHostIO
{

public:
	HostIO_USB(void);
	virtual ~HostIO_USB(void);
	virtual int ListDevices(std::vector<CameraID> &);
	virtual int OpenEx(CameraID);
	virtual int SetTimeouts(int, int);
	virtual int Close();
	virtual int Write(unsigned char *, int, int *);
	virtual int Read(unsigned char *, int, int *);
	virtual int GetReadWriteQueueStatus(int *, int *);
	virtual int ResetDevice();
	virtual int Purge();
	virtual int GetReadQueueStatus(int *);
	virtual int SetStandardReadTimeout ( int ulTimeout);
	virtual int SetStandardWriteTimeout( int ulTimeout);
	virtual int SetIOTimeout (IOTimeout ioTimeout);
	virtual int MaxBytesPerReadBlock();
	virtual int WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten);
	virtual int ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead);
	virtual IOType GetTransferType();
	// USB Specific calls
	int SetLatencyTimer(UCHAR);

protected:
	int				SetUSBParameters(DWORD, DWORD);
	QSI_IOTimeouts	m_IOTimeouts;
	QSILog			*m_log;				// Log USB transactions

private:
	int				m_iUSBStatus;	    // Generic return status
	void* 			m_DLLHandle;        // Holds pointer to FTDI DLL in memory
	bool 			m_bLoaded;          // True if the FTDI USB DLL is loaded
	int 			m_iLoadStatus;      //
	std::vector<VidPid> 	m_vidpids;	// Table of Vendor and Product IDs to try
	
#if defined(USELIBFTDIZERO) || defined(USELIBFTDIONE)
	int my_ftdi_read_data(struct ftdi_context *ftdi, unsigned char *buf, int size);
	ftdi_context m_ftdi;
	bool m_ftdiIsOpen;
#elif defined(USELIBFTD2XX)
	FT_HANDLE 				m_DeviceHandle; // Holds handle to usb device when connected
#endif

	
};

