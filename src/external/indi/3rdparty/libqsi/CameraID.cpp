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

#include "CameraID.h"

CameraID::CameraID()
{
	SerialNumber = "";
	Description = "";
	VendorID = 0;
	ProductID = 0;
	ConnProto = CP_None;
	SerialToOpen = "";
	IPv4Addr.s_addr = 0;
}

CameraID::CameraID(std::string Serial, std::string SerialNumToOpen, std::string Desc, int vid, int pid, ConnProto_t proto)
{
	this->ConnProto = proto;
	this->SerialNumber = Serial;
	this->Description = Desc;
	this->VendorID = vid;
	this->ProductID = pid;
	this->SerialToOpen = SerialNumToOpen;
	this->IPv4Addr.s_addr = 0;
}

CameraID::CameraID(std::string Serial, in_addr Addr)
{
	this->ConnProto = CP_TCP;
	this->SerialNumber = Serial;
	this->SerialToOpen = Serial;
	this->Description = "";
	this->IPv4Addr = Addr;
	this->VendorID = 0;
	this->ProductID = 0;
}

CameraID::~CameraID()
{

}


// Copy constructor
CameraID::CameraID(const CameraID & cid)
{
	this->SerialNumber = cid.SerialNumber;
	this->Description  = cid.Description;
	this->VendorID     = cid.VendorID;
	this->ProductID    = cid.ProductID;
	this->ConnProto = cid.ConnProto;
	this->SerialToOpen = cid.SerialToOpen;
	this->IPv4Addr	   = cid.IPv4Addr;
}

//Operator = constructor
CameraID & CameraID::operator=( const CameraID & cid)
{
	this->SerialNumber = cid.SerialNumber;
	this->Description  = cid.Description;
	this->VendorID     = cid.VendorID;
	this->ProductID    = cid.ProductID;
	this->ConnProto = cid.ConnProto;
	this->SerialToOpen = cid.SerialToOpen;
	this->IPv4Addr	   = cid.IPv4Addr;
	return *this;
}
