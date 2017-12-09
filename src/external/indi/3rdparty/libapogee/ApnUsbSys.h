// ApnUsbSys.h    
//
// Copyright (c) 2003, 2004 Apogee Instruments, Inc.
//
// Defines common data structure(s) for sharing between application
// layer and the AltaUSB.sys device driver.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.
//////////////////////////////////////////////////////////////////////

#if !defined(_APNUSBSYS_H__INCLUDED_)
#define _APNUSBSYS_H__INCLUDED_

#include <stdint.h>


#define VND_ANCHOR_LOAD_INTERNAL		0xA0

#define VND_APOGEE_CMD_BASE				0xC0
#define VND_APOGEE_STATUS				( VND_APOGEE_CMD_BASE + 0 )
#define VND_APOGEE_CAMCON_REG			( VND_APOGEE_CMD_BASE + 2 )
#define VND_APOGEE_BUFCON_REG			( VND_APOGEE_CMD_BASE + 3 )
#define VND_APOGEE_SET_SERIAL			( VND_APOGEE_CMD_BASE + 4 )
#define VND_APOGEE_SERIAL				( VND_APOGEE_CMD_BASE + 5 )
#define VND_APOGEE_EEPROM				( VND_APOGEE_CMD_BASE + 6 )
#define VND_APOGEE_SOFT_RESET			( VND_APOGEE_CMD_BASE + 8 )
#define VND_APOGEE_GET_IMAGE			( VND_APOGEE_CMD_BASE + 9 )
#define VND_APOGEE_STOP_IMAGE			( VND_APOGEE_CMD_BASE + 10 )
#define VND_APOGEE_I2C					( VND_APOGEE_CMD_BASE + 11 )
#define VND_APOGEE_MEMRW				( VND_APOGEE_CMD_BASE + 12 )
#define VND_APOGEE_DATA_PORT			( VND_APOGEE_CMD_BASE + 13 )
#define VND_APOGEE_CONTROL_PORT			( VND_APOGEE_CMD_BASE + 14 )
#define VND_APOGEE_VID					( VND_APOGEE_CMD_BASE + 15 )
#define VND_APOGEE_PID					( VND_APOGEE_CMD_BASE + 16 )
#define VND_APOGEE_DID					( VND_APOGEE_CMD_BASE + 17 )
#define VND_APOGEE_ROM_CRC				( VND_APOGEE_CMD_BASE + 18 )	// Not used.
#define VND_APOGEE_DFRW					( VND_APOGEE_CMD_BASE + 19 )	// Read DF 
#define VND_APOGEE_FLASHID				( VND_APOGEE_CMD_BASE + 20 )	// Read IF code (4 bytes) from dataflash.
#define VND_APOGEE_DFERA				( VND_APOGEE_CMD_BASE + 21 )	// Erase blocks (not pages -- 4K blocks).
#define VND_APOGEE_PROGMODE				( VND_APOGEE_CMD_BASE + 22 )
#define VND_APOGEE_MEMRW2				( VND_APOGEE_CMD_BASE + 23 )
#define VND_APOGEE_DFERASE				( VND_APOGEE_CMD_BASE + 24 )
#define VND_APOGEE_CMD_TOP				( 24 )  


#define	REQUEST_IN	0x1
#define REQUEST_OUT	0x0


typedef struct _APN_USB_REQUEST
{
	uint8_t	Request;
	uint8_t	Direction;
	uint16_t	Value;
	uint16_t	Index;
} APN_USB_REQUEST, *PAPN_USB_REQUEST;



#endif  // !defined(_APNUSBSYS_H__INCLUDED_)
