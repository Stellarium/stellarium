/*****************************************************************************************
NAME
 CameraID

DESCRIPTION
 Class to specify info to define a specific camera
	Inherited by USBID and EthernetID

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
DRC 03.23.09 Original Version
******************************************************************************************/

#pragma once

#include <string>
#include <netinet/in.h>

class CameraID
{
public:
	enum ConnProto_t
	{
		CP_None,
		CP_All,
		CP_USB,
		CP_TCP,
		CP_CyUSB
	}ConnProto;
public:
	CameraID(void);
	// USB Connection FTDI or Cypress
	CameraID(std::string Serial, std::string SerialToOpen, std::string Desc, int vid, int pid, ConnProto_t proto);
	// Sockets based TCP/IP connection
	CameraID(std::string Serial, in_addr IPv4Addr);
	~CameraID(void);
	CameraID(const CameraID & cid);
	CameraID & operator=( const CameraID & cid); 

	std::string SerialNumber;
	std::string Description;
	std::string SerialToOpen;
	
	// USB
	int VendorID;
	int ProductID;

	// TCP
	in_addr IPv4Addr;

};
