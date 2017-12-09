/*
 * QSIError.cpp -- implementation stuff related to QSIException
 * qsilib
 * Copyright (C) QSI (Quantum Scientific Imaging) 2005-2013 <dchallis@qsimaging.com>
 * 
 * 
 */
#include "QSIError.h"

std::ostream&	operator<<(std::ostream& out, const QSIException& qsiexception) 
{
	out << qsiexception.what() << ": ";
	switch (qsiexception.error_code()) 
	{
	case QSI_OK:
		out << "OK";
		break;
	case QSI_NOTSUPPORTED:
		out << "not supported";
		break;
	case QSI_UNRECOVERABLE:
		out << "unrecoverable";
		break;
	case QSI_NOFILTER:
		out << "no filter";
		break;
	case QSI_NOMEMORY:
		out << "no memory";
		break;
	case QSI_BADROWSIZE:
		out << "bad row size";
		break;
	case QSI_BADCOLSIZE:
		out << "bad column size";
		break;
	case QSI_INVALIDBIN:
		out << "invalid binning mode";
		break;
	case QSI_NOASYMBIN:
		out << "asym binning not supported";
		break;
	case QSI_BADEXPOSURE:
		out << "bad exposure";
		break;
	case QSI_BADBINSIZE:
		out << "bad bin size";
		break;
	case QSI_NOEXPOSURE:
		out << "no exposure";
		break;
	case QSI_BADRELAYSTATUS:
		out << "bad relay status";
		break;
	case QSI_BADABORTRELAYS:
		out << "bad abort relays";
		break;
	case QSI_RELAYERROR:
		out << "relay error";
		break;
	case QSI_INVALIDIMAGEPARAMETER:
		out << "invalid image parameter";
		break;
	case QSI_NOIMAGEAVAILABLE:
		out << "no image available";
		break;
	case QSI_NOTCONNECTED:
		out << "not connected";
		break;
	case QSI_INVALIDFILTERNUMBER:
		out << "invalid filter number";
		break;
	case QSI_RECOVERABLE:
		out << "recoverable";
		break;
	case QSI_CONNECTED:
		out << "connected";
		break;
	case QSI_INVALIDTEMP:
		out << "invalid temperature";
		break;
	case QSI_TRIGGERTIMEOUT:
		out << "trigger timeout";
		break;
	case QSI_EEPROMREADERROR:
		out << "EEPROM read error";
		break;
	default:
		out << "unspecified error";
		break;
	}
	return out;
}
