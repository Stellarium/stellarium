/*****************************************************************************************
NAME
 wincompat

DESCRIPTION
 Windows types defined for linux

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 MWB 04.28.06 Original Version
 *****************************************************************************************/
#pragma once

#ifdef __APPLE__
#include <limits.h>
#define CString std::string
typedef char TCHAR;
#else
#include <linux/limits.h>
#endif

#include <stdint.h>
#include <cstdio>
#include <string>
#include <sstream>

#define MAX_PATH PATH_MAX
#define MAKE_HRESULT(x, y, z) z
#define IID_ICamera 1
#define IID_IFilterWheel 2
#define Error(x,y,z) 							\
	strncpy(m_szLastErrorText, x, LASTERRORTEXTSIZE),		\
	m_iLastErrorValue = z,						\
	sprintf(m_ErrorText, "0x%x:", z),				\
	(m_bStructuredExceptions)					\
		? throw std::runtime_error(std::string(m_ErrorText)	\
			+ std::string(m_szLastErrorText))		\
		: z
// 
#define S_OK 0
#define EnterCriticalSection( x ) pthread_mutex_lock( x )
#define LeaveCriticalSection( x ) pthread_mutex_unlock( x )
#define _T( x ) x
#define DOUBLE double

template <class T>
std::string StringOf(T object)
{
	std::ostringstream os;
	os << object;
	return (os.str());
}
