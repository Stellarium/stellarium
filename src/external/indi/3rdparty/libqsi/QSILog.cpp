/*****************************************************************************************
NAME          : QSILog
DESCRIPTION   : Provides registry controlled USB event logging
COPYRIGHT (C) : QSI (Quantum Scientific Imaging) 2006-2007
*****************************************************************************************/
#include "QSILog.h"

//****************************************************************************************
// CLASS FUNCTION DEFINITIONS
//****************************************************************************************


QSILog::QSILog(const char * filename, const char * szValueName, const char * szPreFixName)
{
	//
	// szValueName is the name of the ini value to check if logging is enabled
	// for this object
	//
	me = getuid();
	my_passwd = getpwuid(me);
	pTmp = my_passwd->pw_dir;

	if (pTmp != NULL)
	{
		strncpy(szPath, pTmp, MAX_PATH);
		//Add target file name
		strncat(szPath, "/", MAX_PATH);
		strncat(szPath, filename, MAX_PATH);
		strncpy(m_tszFilename, szPath, MAX_PATH);
	}
	strncpy(m_tszValueName, szValueName, MSGSIZE);
	strncpy(m_tszPreFixName, szPreFixName, MSGSIZE);
	m_pfLogFile = NULL;
	m_bLogging = false;
	m_logLevel = 0;
	gettimeofday( &m_tvLastTick, NULL );
	return;
}

QSILog::~QSILog()
{
	this->Close();
	return;
}

bool QSILog::Open()
{
	if (!IsLogFileOpen())
	{
		m_pfLogFile = fopen(m_tszFilename, "a+t");
	}
	return m_pfLogFile!=NULL?true:false;
}

void QSILog::TestForLogging()
{
	FILE *pFile;

	me = getuid();
	my_passwd = getpwuid(me);
	pTmp = my_passwd->pw_dir;

	if (pTmp == NULL)
	{
		m_bLogging = false;
		return;
	}
	strncpy(szPath, pTmp, MAX_PATH);
	strncat(szPath, "/.", MAX_PATH);
	strncat(szPath, m_tszValueName, MAX_PATH);

	pFile = fopen(szPath, "r");

	if (pFile == NULL) 
	{
		m_bLogging = false;
		return;
	}
	
	m_logLevel = 0;
	fscanf(pFile, "%d", &m_logLevel);

	if (m_logLevel != 0)
	{
		m_bLogging = Open();
	}
	else
	{
		m_bLogging = false;
		if (IsLogFileOpen())
			this->Close();
	}
	return;
}

bool QSILog::LoggingEnabled()
{
	return m_bLogging;
}

bool QSILog::LoggingEnabled(int iLevel)
{
	return m_bLogging && m_logLevel >= iLevel;
}

int QSILog::LogLevel ()
{
	return m_logLevel;
}

bool QSILog::IsLogFileOpen()
{
	return (m_pfLogFile != NULL);
}

void QSILog::Write(int iReqLevel)
{
	this->Write(iReqLevel, m_Message);
	return;
}

void QSILog::WriteBuffer(int iReqLevel, void * lpvBuff, unsigned int bufsize, unsigned int len, unsigned int maxshown)
{
		if (!LoggingEnabled(iReqLevel))
			return;
		len = len > bufsize ? bufsize : len; // limit len to buffsize for no overrun
		len = len > maxshown ? maxshown : len; // Limit len to the max bytes the caller wants
		int iLines = (len / 16);		// 16 bytes per row
		if (len % 16 > 0)			// Partial row
			iLines++;

		for (int i = 0; i < iLines; i++)
		{
			int col = i == iLines-1 ? len - (i * 16) : 16;
			for (int j = 0; j < col; j++)
			{
				snprintf(m_Message+(j*3), 4, _T("%02x "), ((unsigned char *)lpvBuff)[(i*16)+j]);
			}
			Write(iReqLevel);
		}
}

void QSILog::Write(int iReqLevel, const char * msg, ...)
{
	char pMessage[1024];
	std::va_list args;
	char tcsBuf[MSGSIZE];
	time_t stTime;
	tm *tmGMT;
	pid_t PID;
	timeval tvCurrentTick;
	long long llNetTick;

	if (!LoggingEnabled(iReqLevel))
			return;

	va_start(args, msg);
	vsnprintf(pMessage, 1024, msg, args);
	va_end(args);

	time(&stTime);
	tmGMT = gmtime(&stTime);

	gettimeofday( &tvCurrentTick, NULL );
	llNetTick = (tvCurrentTick.tv_sec * 1000000) + tvCurrentTick.tv_usec;
	llNetTick = llNetTick - ((m_tvLastTick.tv_sec * 1000000) + m_tvLastTick.tv_usec);

	snprintf(tcsBuf, MSGSIZE, "%04d-%02d-%02d,%02d:%02d:%02d.%03d,delta_usec:%012lld,", 
				tmGMT->tm_year + 1900, 
				tmGMT->tm_mon + 1, 
				tmGMT->tm_mday, 
				tmGMT->tm_hour, 
				tmGMT->tm_min, 
				tmGMT->tm_sec, 
				0,
				llNetTick);

	m_tvLastTick.tv_sec  = tvCurrentTick.tv_sec;
	m_tvLastTick.tv_usec = tvCurrentTick.tv_usec;

	fputs(tcsBuf, m_pfLogFile);
	PID = getpid();
	snprintf(tcsBuf, MSGSIZE, "Thread:%08u,", PID);
	fputs(tcsBuf, m_pfLogFile);
	fputs(m_tszPreFixName, m_pfLogFile);
	fputs(":", m_pfLogFile);
	fputs(pMessage, m_pfLogFile);
	fputs("\n", m_pfLogFile);
	fflush(m_pfLogFile);

	return;
}

void QSILog::Close()
{
	if (m_pfLogFile != NULL)
	{
		fclose(m_pfLogFile);
	}
	m_pfLogFile = NULL;
	return;
}
