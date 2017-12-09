/*****************************************************************************************
NAME
 PacketWrapper

DESCRIPTION
 Packet interface

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 MWB 04.28.06 Original Version
 *****************************************************************************************/

#pragma once

#include "QSI_Global.h"
#include "IHostIO.h"
#include "HostConnection.h"
#include "QSILog.h"

class QSI_PacketWrapper
{
public:
	QSI_PacketWrapper();
	~QSI_PacketWrapper();
    int PKT_SendPacket(IHostIO *connection, unsigned char * pTBuffer , unsigned char * pRxBuffer, 
                       bool bCheckQueues, IOTimeout ioTimout = IOTimeout_Normal);
    int PKT_CheckQueues(IHostIO *connection);  // Returns number of characters in Rx & Tx queues

private:
    int m_iStatus;          // Stores last status received
	QSILog * m_log;
};
