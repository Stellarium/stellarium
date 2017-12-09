/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
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
#pragma once
#ifndef NO_CYUSB
#include "IHostIO.h"
#include "QSILog.h"
#include "QSI_Global.h"
#include "cyusb.h"

const USHORT QSICyVID = 0x0403;
const USHORT QSICyPID = 0xeb48;

class HostIO_CyUSB : public IHostIO
{
public:
	HostIO_CyUSB(void);
	virtual ~HostIO_CyUSB(void);
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

private:
	int GetSerialNumber(std::string & pSerial);
	int GetDesc(std::string & pDesc);
	
protected:
	QSI_IOTimeouts	m_IOTimeouts;
	QSILog			*m_log;				// Log USB transactions

private:
    cyusb_handle *h;
	unsigned int CyReadTimeout;
	unsigned int CyWriteTimeout;
};
#endif
