/*****************************************************************************************
NAME
 QSIError.h 

DESCRIPTION
 QSI API Error codes

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2007

REVISION HISTORY
 DRC 12.19.06 Original Version
*****************************************************************************************/
//
// QSIError.h
// QSI Camera API error codes
//
//
#pragma once

#include <stdexcept>
#include <iostream>

#define QSI_OK				0
#define QSI_NOTSUPPORTED	0x80040400
#define QSI_UNRECOVERABLE	0x80040401
#define QSI_NOFILTER		0x80040402
#define QSI_NOMEMORY		0x80040403
#define QSI_BADROWSIZE		0x80040404
#define QSI_BADCOLSIZE		0x80040405
#define QSI_INVALIDBIN		0x80040406
#define QSI_NOASYMBIN		0x80040407
#define QSI_BADEXPOSURE		0x80040408
#define QSI_BADBINSIZE		0x80040409
#define QSI_NOEXPOSURE		0x8004040A
#define QSI_BADRELAYSTATUS	0x8004040B
#define QSI_BADABORTRELAYS	0x8004040C
#define QSI_RELAYERROR		0x8004040D
#define QSI_INVALIDIMAGEPARAMETER 0x8004040E
#define QSI_NOIMAGEAVAILABLE 0x8004040F
#define QSI_NOTCONNECTED	0x80040410
#define QSI_INVALIDFILTERNUMBER 0x80040411
#define QSI_RECOVERABLE		0x80040412
#define QSI_CONNECTED		0x80040413
#define QSI_INVALIDTEMP		0x80040414
#define QSI_TRIGGERTIMEOUT	0x80040415
#define QSI_EEPROMREADERROR	0x80040416

class QSIException : public std::runtime_error 
{
	unsigned long	_error_code;
public:
	QSIException(const std::string& cause, unsigned long code) : std::runtime_error(cause), _error_code(code) { }
	unsigned long	error_code() const { return _error_code; }
};


std::ostream&	operator<<(std::ostream& out, const QSIException& qsiexception);

