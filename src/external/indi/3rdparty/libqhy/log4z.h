/*
 * Log4z License
 * -----------
 * 
 * Log4z is licensed under the terms of the MIT license reproduced below.
 * This means that Log4z is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */


/*
 * AUTHORS:  YaweiZhang <yawei_zhang@foxmail.com>
 * VERSION:  3.0.0
 * PURPOSE:  A lightweight library for error reporting and logging to file and screen .
 * CREATION: 2010.10.4
 * LCHANGE:  2014.12.19
 * LICENSE:  Expat/MIT License, See Copyright Notice at the begin of this file.
 */


/*
 * contact me:
 * tencent qq group: 19811947
 * mail: yawei_zhang@foxmail.com
 */


/* 
 * UPDATES LOG
 * 
 * VERSION 0.1.0 <DATE: 2010.10.4>
 * 	create the first project.  
 * 	It support put log to screen and files, 
 * 	support log level, support one day one log file.
 * 	support multi-thread, cross-platform.
 * 
 * VERSION .... <DATE: ...>
 * 	...
 * 
 * VERSION 0.9.0 <DATE: 2012.12.24>
 * 	support config files.
 * 	support color text in screen.
 * 	support multiple output to different files.
 * 
 * VERSION 1.0.0 <DATE: 2012.12.29>
 * 	support comments in the config file.
 * 	add a advanced demo in the ./project
 * 	fix some details.
 * 
 * VERSION 1.0.1 <DATE: 2013.01.01>
 * 	change and add some Comments in the log4z
 * 	simplify the 'fast_test' demo projects.
 * 
 * VERSION 1.1.0 <DATE: 2013.01.24>
 * 	the method Start will wait for the logger thread started.
 * 	config and add method change. 
 * 	namespace change.
 * 
 * VERSION 1.1.1 <DATE: 2013.02.23>
 * 	add status info method.
 * 	optimize. 
 *
 * VERSION 1.2.0 <DATE: 2013.04.05>
 * 	add stress test demo
 *  rewrite Stream module,better performance. 
 * 
 * VERSION 1.2.1 <DATE: 2013.04.13>
 * 	fixed type name 'long' stream format on 64/32 operation system.
 *  logger will not loss any log on process normal exit.
 *
 * VERSION 2.0.0 <DATE: 2013.04.25>
 * 	new interface: 
 *      merge some Main interface and Dynamic interface
 *      add Change Logger Attribute method by thread-safe
 * 	new config design.
 * 	log file name append process id.
 *
 * VERSION 2.1 <DATE: 2013.05.22>
 * 	support binary text output 
 *  rewrite write file module, support vs2005 open Chinese characters path
 *
 * VERSION 2.2 <DATE: 2013.07.08>
 *	optimized binary stream output view
 * 	support wchar * string.
 *  
 * VERSION 2.3 <DATE: 2013.08.29>
 *  adjust output file named.
 *  support different month different directory.
 *  adjust some detail.
 * 
 * VERSION 2.4 <DATE: 2013.10.07>
 *  support rolling log file.
 *  support hot update configure
 *  used precision time in log.
 *  micro set default logger attribute
 *  fix tls bug in windows xp dll
 *
 * VERSION 2.5 <DATE: 2014.03.25>
 *  screen output can choice synchronous or not
 *  fix sometimes color will disorder on windows.
 *  eliminate some compiler warning
 *  fix sem_timewait in linux
 *  add format-style method at input log, cannot support vs2003 and VC6.
 *  fix WCHAR String cannot output
 *  optimize std::string, binary log input, and support std::wstring.
 *  clean code, better readability
 *  
 * VERSION 2.6 <DATE: 2014.08.19>
 *  add PrePushLog 
 *  better performance when log is filter out.
 *  interface replace std::string because it's in shared library is unsafe.
 *  add log level 'trace'
 * 
 * VERSION 2.6.1 <DATE: 2014.08.22>
 *  fix bug from defined _MSC_VER 
 *
 * VERSION 2.7 <DATE: 2014.09.21>
 *  compatible mac machine,  now  log4z can working in linux/windows/mac.
 *
 * VERSION 2.8 <DATE: 2014.09.27>
 *  support synchronous written to file and thread-safe
 *  fix compatibility on MinGW. a constant value suffix.
 *  ignore utf-8 file BOM when load configure file
 *  use macro WIN32_LEAN_AND_MEAN replace head file winsock2.h
 *  new naming notations
 * 
 * VERSION 3.0 <DATE: 2014.12.19>
 *  new naming notations
 *  support for reading config from a string.
 *  remove all TLS code, used dispatch_semaphore in apple OS.
 *  support system: windows, linux, mac, iOS
 *  
 */


#pragma once
#ifndef _ZSUMMER_LOG4Z_H_
#define _ZSUMMER_LOG4Z_H_

#include <string>
#include <sstream>
#include <errno.h>
#include <stdio.h>
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

//! logger ID type. DO NOT TOUCH
typedef int LoggerId;

//! the invalid logger id. DO NOT TOUCH
const int LOG4Z_INVALID_LOGGER_ID = -1;

//! the main logger id. DO NOT TOUCH
//! can use this id to set the main logger's attribute.
//! example:
//! ILog4zManager::getPtr()->setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_WARN);
//! ILog4zManager::getPtr()->setLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);
const int LOG4Z_MAIN_LOGGER_ID = 0;

//! the main logger name. DO NOT TOUCH
const char*const LOG4Z_MAIN_LOGGER_KEY = "Main";

//! check VC VERSION. DO NOT TOUCH
//! format micro cannot support VC6 or VS2003, please use stream input log, like LOGI, LOGD, LOG_DEBUG, LOG_STREAM ...
#if _MSC_VER >= 1400 //MSVC >= VS2005
#define LOG4Z_FORMAT_INPUT_ENABLE
#endif

#ifndef WIN32
#define LOG4Z_FORMAT_INPUT_ENABLE
#endif

//! LOG Level
enum ENUM_LOG_LEVEL
{
	LOG_LEVEL_TRACE = 0,
	LOG_LEVEL_DEBUG,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARN,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_ALARM,
	LOG_LEVEL_FATAL,
};

//////////////////////////////////////////////////////////////////////////
//! -----------------default logger config, can change on this.-----------
//////////////////////////////////////////////////////////////////////////
//! the max logger count.
const int LOG4Z_LOGGER_MAX = 10;
//! the max log content length.
const int LOG4Z_LOG_BUF_SIZE = 2048;

//! all logger synchronous output or not
const bool LOG4Z_ALL_SYNCHRONOUS_OUTPUT = false;
//! all logger synchronous display to the windows debug output
const bool LOG4Z_ALL_DEBUGOUTPUT_DISPLAY = false;

//! default logger output file.
const char* const LOG4Z_DEFAULT_PATH = "./log/";
//! default log filter level
const int LOG4Z_DEFAULT_LEVEL = LOG_LEVEL_DEBUG;
//! default logger display
const bool LOG4Z_DEFAULT_DISPLAY = true;
//! default logger output to file
const bool LOG4Z_DEFAULT_OUTFILE = true;
//! default logger month dir used status
const bool LOG4Z_DEFAULT_MONTHDIR = false;
//! default logger output file limit size, unit M byte.
const int LOG4Z_DEFAULT_LIMITSIZE = 100;
//! default logger show suffix (file name and line number) 
const bool LOG4Z_DEFAULT_SHOWSUFFIX = true;

///////////////////////////////////////////////////////////////////////////
//! -----------------------------------------------------------------------
//////////////////////////////////////////////////////////////////////////





#ifndef _ZSUMMER_BEGIN
#define _ZSUMMER_BEGIN namespace zsummer {
#endif  
#ifndef _ZSUMMER_LOG4Z_BEGIN
#define _ZSUMMER_LOG4Z_BEGIN namespace log4z {
#endif
_ZSUMMER_BEGIN
_ZSUMMER_LOG4Z_BEGIN




//! log4z class
class ILog4zManager
{
public:
	ILog4zManager(){};
	virtual ~ILog4zManager(){};

	//! Log4z Singleton
	
	static ILog4zManager * getInstance();
	inline static ILog4zManager & getRef(){return *getInstance();}
	inline static ILog4zManager * getPtr(){return getInstance();}

	//! Config or overwrite configure
	//! Needs to be called before ILog4zManager::Start,, OR Do not call.
	virtual bool config(const char * configPath) = 0;
	virtual bool configFromString(const char * configContent) = 0;

	//! Create or overwrite logger.
	//! Needs to be called before ILog4zManager::Start, OR Do not call.
	virtual LoggerId createLogger(const char* key) = 0;

	//! Start Log Thread. This method can only be called once by one process.
	virtual bool start() = 0;

	//! Default the method will be calling at process exit auto.
	//! Default no need to call and no recommended.
	virtual bool stop() = 0;

	//! Find logger. thread safe.
	virtual LoggerId findLogger(const char* key) =0;

	//pre-check the log filter. if filter out return false. 
	virtual bool prePushLog(LoggerId id, int level) = 0;
	//! Push log, thread safe.
	virtual bool pushLog(LoggerId id, int level, const char * log, const char * file = NULL, int line = 0) = 0;

	//! set logger's attribute, thread safe.
	virtual bool enableLogger(LoggerId id, bool enable) = 0;
	virtual bool setLoggerName(LoggerId id, const char * name) = 0;
	virtual bool setLoggerPath(LoggerId id, const char * path) = 0;
	virtual bool setLoggerLevel(LoggerId id, int nLevel) = 0;
	virtual bool setLoggerFileLine(LoggerId id, bool enable) = 0;
	virtual bool setLoggerDisplay(LoggerId id, bool enable) = 0;
	virtual bool setLoggerOutFile(LoggerId id, bool enable) = 0;
	virtual bool setLoggerLimitsize(LoggerId id, unsigned int limitsize) = 0;
	virtual bool setLoggerMonthdir(LoggerId id, bool enable) = 0;
	

	//! Update logger's attribute from config file, thread safe.
	virtual bool setAutoUpdate(int interval/*per second, 0 is disable auto update*/) = 0;
	virtual bool updateConfig() = 0;

	//! Log4z status statistics, thread safe.
	virtual bool isLoggerEnable(LoggerId id) = 0;
	virtual unsigned long long getStatusTotalWriteCount() = 0;
	virtual unsigned long long getStatusTotalWriteBytes() = 0;
	virtual unsigned long long getStatusWaitingCount() = 0;
	virtual unsigned int getStatusActiveLoggers() = 0;
};

class Log4zStream;
class Log4zBinary;

#ifndef _ZSUMMER_END
#define _ZSUMMER_END }
#endif  
#ifndef _ZSUMMER_LOG4Z_END
#define _ZSUMMER_LOG4Z_END }
#endif

_ZSUMMER_LOG4Z_END
_ZSUMMER_END



//! base micro.
#define LOG_STREAM(id, level, log)\
{\
	if (zsummer::log4z::ILog4zManager::getPtr()->prePushLog(id,level)) \
	{\
		char logBuf[LOG4Z_LOG_BUF_SIZE];\
		zsummer::log4z::Log4zStream ss(logBuf, LOG4Z_LOG_BUF_SIZE);\
		ss << log;\
		zsummer::log4z::ILog4zManager::getPtr()->pushLog(id, level, logBuf, __FILE__, __LINE__);\
	}\
}


//! fast micro
#define LOG_TRACE(id, log) LOG_STREAM(id, LOG_LEVEL_TRACE, log)
#define LOG_DEBUG(id, log) LOG_STREAM(id, LOG_LEVEL_DEBUG, log)
#define LOG_INFO(id, log)  LOG_STREAM(id, LOG_LEVEL_INFO, log)
#define LOG_WARN(id, log)  LOG_STREAM(id, LOG_LEVEL_WARN, log)
#define LOG_ERROR(id, log) LOG_STREAM(id, LOG_LEVEL_ERROR, log)
#define LOG_ALARM(id, log) LOG_STREAM(id, LOG_LEVEL_ALARM, log)
#define LOG_FATAL(id, log) LOG_STREAM(id, LOG_LEVEL_FATAL, log)

//! super micro.
#define LOGT( log ) LOG_TRACE(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGD( log ) LOG_DEBUG(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGI( log ) LOG_INFO(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGW( log ) LOG_WARN(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGE( log ) LOG_ERROR(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGA( log ) LOG_ALARM(LOG4Z_MAIN_LOGGER_ID, log )
#define LOGF( log ) LOG_FATAL(LOG4Z_MAIN_LOGGER_ID, log )


//! format input log.
#ifdef LOG4Z_FORMAT_INPUT_ENABLE
#ifdef WIN32
#define LOG_FORMAT(id, level, logformat, ...) \
{ \
	if (zsummer::log4z::ILog4zManager::getPtr()->prePushLog(id,level)) \
	{\
		char logbuf[LOG4Z_LOG_BUF_SIZE]; \
		_snprintf_s(logbuf, LOG4Z_LOG_BUF_SIZE, _TRUNCATE, logformat, ##__VA_ARGS__); \
		zsummer::log4z::ILog4zManager::getPtr()->pushLog(id, level, logbuf, __FILE__, __LINE__); \
	}\
 }
#else
#define LOG_FORMAT(id, level, logformat, ...) \
{ \
	if (zsummer::log4z::ILog4zManager::getPtr()->prePushLog(id,level)) \
	{\
		char logbuf[LOG4Z_LOG_BUF_SIZE]; \
		snprintf(logbuf, LOG4Z_LOG_BUF_SIZE,logformat, ##__VA_ARGS__); \
		zsummer::log4z::ILog4zManager::getPtr()->pushLog(id, level, logbuf, __FILE__, __LINE__); \
	} \
}
#endif
//!format string
#define LOGFMT_TRACE(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#define LOGFMT_DEBUG(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOGFMT_INFO(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOGFMT_WARN(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define LOGFMT_ERROR(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define LOGFMT_ALARM(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_ALARM, fmt, ##__VA_ARGS__)
#define LOGFMT_FATAL(id, fmt, ...)  LOG_FORMAT(id, LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#define LOGFMTT( fmt, ...) LOGFMT_TRACE(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTD( fmt, ...) LOGFMT_DEBUG(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTI( fmt, ...) LOGFMT_INFO(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTW( fmt, ...) LOGFMT_WARN(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTE( fmt, ...) LOGFMT_ERROR(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTA( fmt, ...) LOGFMT_ALARM(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#define LOGFMTF( fmt, ...) LOGFMT_FATAL(LOG4Z_MAIN_LOGGER_ID, fmt,  ##__VA_ARGS__)
#else
inline void empty_log_format_function1(LoggerId id, const char*, ...){}
inline void empty_log_format_function2(const char*, ...){}
#define LOGFMT_TRACE empty_log_format_function1
#define LOGFMT_DEBUG LOGFMT_TRACE
#define LOGFMT_INFO LOGFMT_TRACE
#define LOGFMT_WARN LOGFMT_TRACE
#define LOGFMT_ERROR LOGFMT_TRACE
#define LOGFMT_ALARM LOGFMT_TRACE
#define LOGFMT_FATAL LOGFMT_TRACE
#define LOGFMTT empty_log_format_function2
#define LOGFMTD LOGFMTT
#define LOGFMTI LOGFMTT
#define LOGFMTW LOGFMTT
#define LOGFMTE LOGFMTT
#define LOGFMTA LOGFMTT
#define LOGFMTF LOGFMTT
#endif


_ZSUMMER_BEGIN
_ZSUMMER_LOG4Z_BEGIN

//! optimze from std::stringstream to Log4zStream
#ifdef WIN32
#pragma warning(push)
#pragma warning(disable:4996)
#endif
class Log4zBinary
{
public:
	Log4zBinary(const char * buf, int len)
	{
		_buf = buf;
		_len = len;
	}
	const char * _buf;
	int  _len;
};
class Log4zStream
{
public:
	inline Log4zStream(char * buf, int len);
	inline int getCurrentLen(){return (int)(_cur - _begin);}
private:
	template<class T>
	inline Log4zStream & writeData(const char * ft, T t);
	inline Log4zStream & writeLongLong(long long t);
	inline Log4zStream & writeULongLong(unsigned long long t);
	inline Log4zStream & writePointer(const void * t);
	inline Log4zStream & writeString(const wchar_t* t){ return writeData("%s", t); }
	inline Log4zStream & writeWString(const wchar_t* t);
	inline Log4zStream & writeBinary(const Log4zBinary & t);
public:
	inline Log4zStream & operator <<(const void * t){ return  writePointer(t); }

	inline Log4zStream & operator <<(const char * t){return writeData("%s", t);}
#ifdef WIN32
	inline Log4zStream & operator <<(const wchar_t * t){ return writeWString(t);}
#endif
	inline Log4zStream & operator <<(bool t){ return (t ? writeData("%s", "true") : writeData("%s", "false"));}

	inline Log4zStream & operator <<(char t){return writeData("%c", t);}

	inline Log4zStream & operator <<(unsigned char t){return writeData("%u",(unsigned int)t);}

	inline Log4zStream & operator <<(short t){ return writeData("%d", (int)t); }

	inline Log4zStream & operator <<(unsigned short t){ return writeData("%u", (unsigned int)t); }

	inline Log4zStream & operator <<(int t){return writeData("%d", t);}

	inline Log4zStream & operator <<(unsigned int t){return writeData("%u", t);}

	inline Log4zStream & operator <<(long t) { return writeLongLong(t); }

	inline Log4zStream & operator <<(unsigned long t){ return writeULongLong(t); }

	inline Log4zStream & operator <<(long long t) { return writeLongLong(t); }

	inline Log4zStream & operator <<(unsigned long long t){ return writeULongLong(t); }

	inline Log4zStream & operator <<(float t){return writeData("%.4f", t);}

	inline Log4zStream & operator <<(double t){return writeData("%.4lf", t);}

	template<class _Elem,class _Traits,class _Alloc> //support std::string, std::wstring
	inline Log4zStream & operator <<(const std::basic_string<_Elem, _Traits, _Alloc> & t){ return *this << t.c_str(); }

	inline Log4zStream & operator << (const zsummer::log4z::Log4zBinary & binary){ return writeBinary(binary); }

private:
	Log4zStream(){}
	Log4zStream(Log4zStream &){}
	char *  _begin;
	char *  _end;
	char *  _cur;
};

inline Log4zStream::Log4zStream(char * buf, int len)
{
	_begin = buf;
	_end = buf + len;
	_cur = _begin;
}

template<class T>
inline Log4zStream& Log4zStream::writeData(const char * ft, T t)
{
	if (_cur < _end)
	{
		int len = 0;
		int count = (int)(_end - _cur);
#ifdef WIN32
		len = _snprintf(_cur, count, ft, t);
		if (len == count || (len == -1 && errno == ERANGE))
		{
			len = count;
			*(_end - 1) = '\0';
		}
		else if (len < 0)
		{
			*_cur = '\0';
			len = 0;
		}
#else
		len = snprintf(_cur, count, ft, t);
		if (len < 0)
		{
			*_cur = '\0';
			len = 0;
		}
		else if (len >= count)
		{
			len = count;
			*(_end - 1) = '\0';
		}
#endif
		_cur += len;
	}
	return *this;
}

inline Log4zStream & Log4zStream::writeLongLong(long long t)
{
#ifdef WIN32  
	writeData("%I64d", t);
#else
	writeData("%lld", t);
#endif
	return *this;
}

inline Log4zStream & Log4zStream::writeULongLong(unsigned long long t)
{
#ifdef WIN32  
	writeData("%I64u", t);
#else
	writeData("%llu", t);
#endif
	return *this;
}

inline Log4zStream & Log4zStream::writePointer(const void * t)
{
#ifdef WIN32
	sizeof(t) == 8 ? writeData("%016I64x", (unsigned long long)t) : writeData("%08I64x", (unsigned long long)t);
#else
	sizeof(t) == 8 ? writeData("%016llx", (unsigned long long)t) : writeData("%08llx", (unsigned long long)t);
#endif
	return *this;
}

inline Log4zStream & Log4zStream::writeBinary(const Log4zBinary & t)
{
	writeData("%s", "\r\n\t[");
	for (int i = 0; i < t._len; i++)
	{
		if (i % 16 == 0)
		{
			writeData("%s", "\r\n\t");
			*this << (void*)(t._buf + i);
			writeData("%s", ": ");
		}
		writeData("%02x ", (unsigned char)t._buf[i]);
	}
	writeData("%s", "\r\n\t]\r\n\t");
	return *this;
}

inline zsummer::log4z::Log4zStream & zsummer::log4z::Log4zStream::writeWString(const wchar_t* t)
{
#ifdef WIN32
	DWORD dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, NULL, 0, NULL, NULL);
	if (dwLen < LOG4Z_LOG_BUF_SIZE)
	{
		std::string str;
		str.resize(dwLen, '\0');
		dwLen = WideCharToMultiByte(CP_ACP, 0, t, -1, &str[0], dwLen, NULL, NULL);
		if (dwLen > 0)
		{
			writeData("%s", str.c_str());
		}
	}
#else
	//not support
#endif
	return *this;
}


#ifdef WIN32
#pragma warning(pop)
#endif

_ZSUMMER_LOG4Z_END
_ZSUMMER_END

#endif




