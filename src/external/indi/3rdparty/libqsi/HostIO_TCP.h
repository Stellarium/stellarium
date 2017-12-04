#pragma once
#include "IHostIO.h"
#include "QSILog.h"
#include <stdio.h>
#include "QSI_Global.h"

#ifdef WIN32
    #include <winsock2.h>
#else
    #include <sys/socket.h>
	#include <sys/ioctl.h>
#endif

class HostIO_TCP : public IHostIO
{
public:
	HostIO_TCP(void);
	virtual ~HostIO_TCP(void);

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
	virtual int SetIOTimeout(IOTimeout ioTimeout);
	virtual int MaxBytesPerReadBlock();
	virtual int WritePacket(UCHAR * pBuff, int iBuffLen, int * iBytesWritten);
	virtual int ReadPacket(UCHAR * pBuff, int iBuffLen, int * iBytesRead);
	virtual IOType GetTransferType();

protected:
	QSI_IOTimeouts	m_IOTimeouts;
	QSILog			*m_log;
private:
	int TCPIP_ErrorDecode(void);
#ifdef WIN32
	WSADATA			m_wsaData;
	SOCKET			m_sock;
#else
	int				m_sock;
#endif
	bool			m_TCP_Stack_OK;
	int				m_ReadTimeout;
	int				m_WriteTimeout;
};

