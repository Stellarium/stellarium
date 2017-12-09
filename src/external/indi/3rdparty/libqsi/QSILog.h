/*****************************************************************************************
NAME
 QSILog

DESCRIPTION
 Log camera i/o to logfile

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2006

REVISION HISTORY
 DRC 12.19.06 Original Version
*****************************************************************************************/

#pragma once
#include <cstdio>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "wincompat.h"
#include <string.h>
#include <grp.h>
#include <pwd.h>
#include <cstdarg>
#define MSGSIZE 256

class QSILog
{
public:
	QSILog(const char* filename, const char * regkey, const char * prefixName);
	~QSILog(void);

	bool Open(void);
	void TestForLogging(void);
	int LogLevel(void);
	bool LoggingEnabled(void);
	bool LoggingEnabled(int iLevel);
	bool IsLogFileOpen(void);
	void Write(int iReqLevel);
	// Display a character buffer, 16 bytes at a time, up to character limit, with an overriding maxium allowed
	void WriteBuffer(int iReqLevel, void * buff, unsigned int bufsize, unsigned int len, unsigned int maxshown);
	void Write(int iReqLevel, const char * msg, ...);
	void Close(void);
	char m_Message[MSGSIZE];

private:
	char m_tszFilename[MAX_PATH+1];
	char m_tszValueName[MSGSIZE];
	char m_tszPreFixName[MSGSIZE];
	FILE* m_pfLogFile;
	bool m_bLogging;
	int m_logLevel;
	timeval m_tvLastTick;

	char szPath[MAX_PATH+1];
	char* pTmp;
	uid_t me;
	struct passwd *my_passwd;
};
