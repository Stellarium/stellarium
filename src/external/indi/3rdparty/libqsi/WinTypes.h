#pragma once

#ifndef __WINDOWS_TYPES__
#define __WINDOWS_TYPES__

#define WINAPI

#define MAX_NUM_DEVICES 50
#include <sys/time.h>

typedef unsigned int 	DWORD;
typedef unsigned int 	ULONG;
typedef unsigned short 	USHORT;
typedef short 			SHORT;
typedef unsigned char	UCHAR;
typedef unsigned short	WORD;
typedef unsigned char	BYTE;
typedef unsigned char	*LPBYTE;
typedef int 		BOOL;
typedef char 		BOOLEAN;
typedef char 		CHAR;
typedef int 		*LPBOOL;
typedef unsigned char 	*PUCHAR;
typedef const char	*LPCSTR;
typedef char		*PCHAR;
typedef void 		*PVOID;
typedef void 		*HANDLE;
typedef long 		LONG;
typedef int 		INT;
typedef unsigned int 	UINT;
typedef char 		*LPSTR;
typedef char 		*LPTSTR;
typedef const char  *LPCTSTR;
typedef DWORD 		*LPDWORD;
typedef WORD 		*LPWORD;
typedef ULONG		*PULONG;
typedef LONG        *LPLONG;
typedef PVOID		LPVOID;
typedef void		VOID;
typedef unsigned long long int ULONGLONG;

typedef struct _OVERLAPPED {
	DWORD Internal;
	DWORD InternalHigh;
	DWORD Offset;
	DWORD OffsetHigh;
	HANDLE hEvent;
} OVERLAPPED, *LPOVERLAPPED;

typedef struct _SECURITY_ATTRIBUTES {
	DWORD nLength;
	LPVOID lpSecurityDescriptor;
	BOOL bInheritHandle;
} SECURITY_ATTRIBUTES , *LPSECURITY_ATTRIBUTES;

typedef struct timeval SYSTEMTIME;
typedef struct timeval FILETIME;
#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

//
// Modem Status Flags
//
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

//
// Error Flags
//

#define CE_RXOVER           0x0001  // Receive Queue overflow
#define CE_OVERRUN          0x0002  // Receive Overrun Error
#define CE_RXPARITY         0x0004  // Receive Parity Error
#define CE_FRAME            0x0008  // Receive Framing error
#define CE_BREAK            0x0010  // Break Detected
#define CE_TXFULL           0x0100  // TX Queue is full
#define CE_PTO              0x0200  // LPTx Timeout
#define CE_IOE              0x0400  // LPTx I/O Error
#define CE_DNS              0x0800  // LPTx Device not selected
#define CE_OOP              0x1000  // LPTx Out-Of-Paper
#define CE_MODE             0x8000  // Requested mode unsupported

#ifndef INVALID_HANDLE_VALUE
#define INVALID_HANDLE_VALUE 0xFFFFFFFF
#endif

#endif
