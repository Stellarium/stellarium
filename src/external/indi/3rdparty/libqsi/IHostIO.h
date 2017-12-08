/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * qsilib
 * Copyright (C) Quantum Scientific Imaging, Inc. 2013 <dchallis@qsimaging.com>
 * 
 */
#pragma once

#include "CameraID.h"
#include <vector>
#include "WinTypes.h"

enum IOTimeout
{
	IOTimeout_Normal = 0,
	IOTimeout_Short	 = 1,
	IOTimeout_Long	 = 2
};

enum IOType
{
	IOType_Stream = 0,
	IOType_MultiRow = 1,
	IOType_SingleRow = 2,
	IOType_Fixed = 3,
	IOType_Packet = 4
};

class IHostIO
{
public:
	IHostIO(void);
	virtual ~IHostIO(void);
	virtual int ListDevices(std::vector<CameraID> &) = 0;
	virtual int OpenEx(CameraID) = 0;
	virtual int SetTimeouts(int, int) = 0;
	virtual int Close() = 0;
	virtual int Write(unsigned char *, int, int *) = 0;
	virtual int Read(unsigned char *, int, int *) = 0;
	virtual int GetReadWriteQueueStatus(int *, int *) = 0;
	virtual int ResetDevice() = 0;
	virtual int Purge() = 0;
	virtual int GetReadQueueStatus(int *) = 0;
	virtual int SetStandardReadTimeout ( int ulTimeout) = 0;
	virtual int SetStandardWriteTimeout( int ulTimeout) = 0;
	virtual int SetIOTimeout(IOTimeout ioTimeout) = 0;
	virtual int MaxBytesPerReadBlock() = 0;
	virtual int WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten) = 0;
	virtual int ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead) = 0;
	virtual IOType GetTransferType() = 0;
};

