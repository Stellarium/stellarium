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

#include "QSI_PacketWrapper.h"
#include "QSI_Registry.h"

//////////////////////////////////////////////////////////////////////////////////////////
QSI_PacketWrapper::QSI_PacketWrapper()
{
	m_log = new QSILog(_T("QSIINTERFACELOG.TXT"), _T("LOGUSBTOFILE"), _T("PACKET"));
	m_iStatus = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
QSI_PacketWrapper::~QSI_PacketWrapper()
{
	delete m_log;
}

//////////////////////////////////////////////////////////////////////////////////////////
int QSI_PacketWrapper::PKT_CheckQueues( IHostIO * con )
{
	// Declare variables
	int AmountInRxQueue = 0, AmountInTxQueue = 0;

	// Get number of characters in Rx and Tx queues
	m_iStatus = con->GetReadWriteQueueStatus(&AmountInRxQueue, &AmountInTxQueue);
	if (m_iStatus != ALL_OK)
		return m_iStatus + ERR_PKT_CheckQueuesFailed;

	// Return error if both or one of the queues is dirty
	if( (AmountInRxQueue != 0) && (AmountInTxQueue != 0) )
		return ERR_PKT_BothQueuesDirty;
	else if (AmountInRxQueue != 0)
	{
		do
		{
			int AmountRead;
			unsigned char * ucReadbuf = new unsigned char[AmountInRxQueue];
			con->Read(ucReadbuf, AmountInRxQueue, &AmountRead);
			m_log->Write(2, _T("*** Dirty Read Queue with %d pending in queue. Dumping data: ***"), AmountInRxQueue);
			m_log->WriteBuffer(2, (void *)ucReadbuf, AmountInRxQueue, AmountRead, 256);
			m_log->Write(2, _T("*** End Dirty Single Read Queue Dump, (there may be more remaining...) ***"));
			delete[] ucReadbuf;
			usleep(1000*100);	// Allow for more pending data to accumulate on host.
			con->GetReadWriteQueueStatus(&AmountInRxQueue, &AmountInTxQueue);
		} while (AmountInRxQueue != 0);
		return ERR_PKT_RxQueueDirty;
	}
	else if (AmountInTxQueue != 0)
	  return ERR_PKT_TxQueueDirty;

	return ALL_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
int QSI_PacketWrapper::PKT_SendPacket( IHostIO * con, unsigned char * pvTxBuffer, unsigned char * pvRxBuffer, 
                                      bool bPostCheckQueues, IOTimeout ioTimeout  )
{
	// Declare variables
	UCHAR *ucTxBuffer = (UCHAR*) pvTxBuffer,  // UCHAR pointer to Tx Buffer
		*ucRxBuffer = (UCHAR*) pvRxBuffer,  // UCHAR pointer to Rx Buffer
		ucTxCommand = 0,      // Holds Tx packet command byte
		ucRxCommand = 0;      // Holds Rx packet command byte

	int dwBytesToWrite = 0,   // Holds # of bytes to write for Tx packet
		dwBytesWritten = 0,   // Holds # of bytes written of Tx packet
		dwBytesInPacket = 0,    // Holds # of bytes to read for Rx packet
		dwBytesReturned = 0;  // Holds # of bytes read of Rx packet

	// Make sure we're starting with clean queues
	m_iStatus = PKT_CheckQueues(con);
	if (m_iStatus != ALL_OK) 
		goto SendPacketExit;
  
	// Get command and length from Tx packet
	ucTxCommand = *(ucTxBuffer+PKT_COMMAND);
	dwBytesToWrite = *(ucTxBuffer+PKT_LENGTH) + PKT_HEAD_LENGTH;
  
	// Make sure Tx packet isn't greater than allowed
	if (dwBytesToWrite + PKT_HEAD_LENGTH > MAX_PKT_LENGTH)
	{
		m_iStatus = ERR_PKT_TxPacketTooLong;
		goto SendPacketExit;
	}

	m_log->Write(2, _T("***Send Request Packet to Camera*** %d bytes total length. Packet Data Follows:"), dwBytesToWrite);
	m_log->WriteBuffer(2, ucTxBuffer, dwBytesToWrite, dwBytesToWrite, 256);
	m_log->Write(2, _T("***Send Request Packet*** Done"));

	if (ioTimeout != IOTimeout_Normal)
		con->SetIOTimeout(ioTimeout);

	// Write Tx packet to buffer
	m_iStatus = con->WritePacket(ucTxBuffer, dwBytesToWrite, &dwBytesWritten);
	if (m_iStatus != ALL_OK)
	{
		m_iStatus += ERR_PKT_TxFailed;
		goto SendPacketExit;
	}
  
	///////////////////////////////////////////////////////////////////////////////////

	// Make sure the entire Tx packet was sent
	if (dwBytesWritten == 0)
	{
		m_iStatus = ERR_PKT_TxNone;
		goto SendPacketExit;
	}
	else if (dwBytesWritten < dwBytesToWrite)
	{
		m_iStatus = ERR_PKT_TxTooLittle;
		goto SendPacketExit;
	}
  
	/////////////////////////////////////////////////////////////////////////////////////
	// Get the reponse packet from camera and validate it
	/////////////////////////////////////////////////////////////////////////////////////
	m_log->Write(2, _T("Read Returned Packet."));
	m_iStatus = con->ReadPacket(ucRxBuffer, 256, &dwBytesReturned);

	if (m_iStatus != ALL_OK)
	{
		m_log->Write(2, _T("***Read Returned Packet Status Failed. Error code %x"), m_iStatus);
		m_iStatus += ERR_PKT_RxHeaderFailed;
		goto SendPacketExit;
	}

	// Get command and length from Rx packet
	ucRxCommand = *(ucRxBuffer+PKT_COMMAND);
	dwBytesInPacket = (DWORD) *(ucRxBuffer+PKT_LENGTH) + 2;
  
	// Make sure Tx and Rx commands match
	if (ucTxCommand != ucRxCommand)
	{
		m_log->Write(2, _T("***Read Returned Packet Header Failed. Tx/Rx Command mismatched. TX: %x, RX: %x"), ucTxCommand, ucRxCommand);
		m_iStatus = ERR_PKT_RxBadHeader;
		goto SendPacketExit;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	// We have the response packet, do final checks and return
	///////////////////////////////////////////////////////////////////////////////////////////////////////////

	// Check Packet Length
	if (dwBytesReturned < dwBytesInPacket)
	{
		m_log->Write(2, _T("***Read Remaining Packeted Data Failed. Too Few Bytes Returned from Read.  BytesToRead %d, BytesReturned %d"), dwBytesInPacket, dwBytesReturned);
		m_iStatus = ERR_PKT_RxTooLittle;
		goto SendPacketExit;
	}

	m_log->Write(2, _T("***Packet Response Read from Camera*** %d bytes total length. Packet Data Follows:"), dwBytesReturned+2);
	m_log->WriteBuffer(2, ucRxBuffer, dwBytesReturned, dwBytesReturned, 256);
	m_log->Write(2, _T("***Packet Response Read Done.***"));

	// Make sure queues are clean
	//
	// if the read image command doesn't happen fast enough, or if the camera is very fast
	// returning from a transfer image or AutoZero request, this could throw an error.
	// This should only be done at points where the camera should be idle.
	// To avoid a race condition with the camera this can only be done on all commands except 
	// TransferImage and AutoZero.
	//
	// This is implemented in those routines in QSI_Interface by setting bPostCheckQueues false 
	//

	if (bPostCheckQueues)
	{
		m_iStatus = PKT_CheckQueues(con);
	}

	// Common Exit routine to insure IOTimeouts get reset on errors.
SendPacketExit:
  	if (ioTimeout != IOTimeout_Normal || m_iStatus != ALL_OK) con->SetIOTimeout(IOTimeout_Normal);
	return m_iStatus;
}


