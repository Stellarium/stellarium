/*
 * Stellarium
 * Copyright (C) 2009 Fabien Chereau
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */

#include "StelLogger.hpp"

#include <QDateTime>
#include <QProcess>
#ifdef Q_OS_WIN
 #include <windows.h>
#endif

#ifdef ANDROID
#include <QSystemDisplayInfo>
QTM_USE_NAMESPACE
#endif

// Init statics variables.
QFile StelLogger::logFile;
QString StelLogger::log;

void StelLogger::init(const QString& logFilePath)
{
	logFile.setFileName(logFilePath);

	if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text | QIODevice::Unbuffered))
		qInstallMsgHandler(StelLogger::debugLogHandler);

	// write timestamp
	writeLog(QString("%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));

	// write OS version
#ifdef Q_OS_WIN
	switch(QSysInfo::WindowsVersion)
	{
		case QSysInfo::WV_95:
			writeLog("Windows 95");
			break;
		case QSysInfo::WV_98:
			writeLog("Windows 98");
			break;
		case QSysInfo::WV_Me:
			writeLog("Windows Me");
			break;
		case QSysInfo::WV_NT:
			writeLog("Windows NT");
			break;
		case QSysInfo::WV_2000:
			writeLog("Windows 2000");
			break;
		case QSysInfo::WV_XP:
			writeLog("Windows XP");
			break;
		case QSysInfo::WV_2003:
			writeLog("Windows Server 2003");
			break;
		case QSysInfo::WV_VISTA:
			writeLog("Windows Vista");
			break;
		case QSysInfo::WV_WINDOWS7:
			writeLog("Windows 7");
			break;
		#ifdef WV_WINDOWS8
		case QSysInfo::WV_WINDOWS8:
			writeLog("Windows 8");
			break;
		#endif
		default:
			writeLog("Unsupported Windows version");
			break;
	}

	// somebody writing something useful for Macs would be great here
#elif defined Q_OS_MAC
	switch(QSysInfo::MacintoshVersion)
	{
		case QSysInfo::MV_10_3:
			writeLog("Mac OS X 10.3");
			break;
		case QSysInfo::MV_10_4:
			writeLog("Mac OS X 10.4");
			break;
		case QSysInfo::MV_10_5:
			writeLog("Mac OS X 10.5");
			break;
		case QSysInfo::MV_10_6:
			writeLog("Mac OS X 10.6");
			break;
		#ifdef MV_10_7
		case QSysInfo::MV_10_7:
			writeLog("Mac OS X 10.7");
			break;
		#endif
		#ifdef MV_10_8
		case QSysInfo::MV_10_8:
			writeLog("Mac OS X 10.8");
			break;
		#endif
		default:
			writeLog("Unsupported Mac version");
			break;
	}

#elif defined Q_OS_LINUX || defined ANDROID

#ifdef ANDROID
	writeLog("Android"); //todo: JNI to get Android version #
#endif
	QFile procVersion("/proc/version");
	if(!procVersion.open(QIODevice::ReadOnly | QIODevice::Text))
		writeLog("Unknown Linux version");
	else
	{
		QString version = procVersion.readAll();
		if(version.right(1) == "\n")
			version.chop(1);
		writeLog(version);
		procVersion.close();
	}
#elif defined Q_OS_BSD4
	// Check FreeBSD, NetBSD, OpenBSD and DragonFly BSD
	QProcess uname;
	uname.start("/usr/bin/uname -srm");
	uname.waitForStarted();
	uname.waitForFinished();
	const QString BSDsystem = uname.readAllStandardOutput();
	writeLog(BSDsystem.trimmed());
#else
	writeLog("Unknown operating system");
#endif

	// write GCC version
#if defined __GNUC__ && !defined __clang__
	#ifdef __MINGW32__
		#define COMPILER "MinGW GCC"
	#else
		#define COMPILER "GCC"
	#endif
	writeLog(QString("Compiled using %1 %2.%3.%4").arg(COMPILER).arg(__GNUC__).arg(__GNUC_MINOR__).arg(__GNUC_PATCHLEVEL__));
#elif defined __clang__
	writeLog(QString("Compiled using %1 %2.%3.%4").arg("Clang").arg(__clang_major__).arg(__clang_minor__).arg(__clang_patchlevel__));
#else
	writeLog("Unknown compiler");
#endif

	// write Qt version
	writeLog(QString("Qt runtime version: %1").arg(qVersion()));
	writeLog(QString("Qt compilation version: %1").arg(QT_VERSION_STR));

	// write addressing mode
#if defined(__LP64__) || defined(_WIN64)
	writeLog("Addressing mode: 64-bit");
#else
	writeLog("Addressing mode: 32-bit");
#endif

	// write memory and CPU info
#if defined Q_OS_LINUX || defined ANDROID

#ifndef BUILD_FOR_MAEMO
	QFile infoFile("/proc/meminfo");
	if(!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		writeLog("Could not get memory info.");
	else
	{
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if (line.startsWith("Mem") || line.startsWith("SwapTotal"))
				writeLog(line);
		}
		infoFile.close();
	}

	infoFile.setFileName("/proc/cpuinfo");
	if (!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		writeLog("Could not get CPU info.");
	else
	{
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if(line.startsWith("model name") || line.startsWith("cpu MHz"))
				writeLog(line);
		}
		infoFile.close();
	}

#ifndef ANDROID
	QProcess lspci;
	lspci.start("lspci -v", QIODevice::ReadOnly);
	lspci.waitForFinished(300);
	const QString pciData(lspci.readAll());
	QStringList pciLines = pciData.split('\n', QString::SkipEmptyParts);
	for (int i = 0; i<pciLines.size(); i++)
	{
		if(pciLines.at(i).contains("VGA compatible controller"))
		{
			writeLog(pciLines.at(i));
			i++;
			while(i < pciLines.size() && pciLines.at(i).startsWith('\t'))
			{
				if(pciLines.at(i).contains("Kernel driver in use"))
					writeLog(pciLines.at(i).trimmed());
				else if(pciLines.at(i).contains("Kernel modules"))
					writeLog(pciLines.at(i).trimmed());
				i++;
			}
		}
	}
#else
	//ANDROID
	QSystemDisplayInfo displayInfo;
	writeLog(QString("DPI width x height: %1 x %2").arg(displayInfo.getDPIWidth(0)).arg(displayInfo.getDPIHeight(0)));
	writeLog(QString("Physical (mm) width x height: %1 x %2").arg(displayInfo.physicalWidth(0)).arg(displayInfo.physicalHeight(0)));
#endif
#endif

// Aargh Windows API
#elif defined Q_OS_WIN
	// Hopefully doesn't throw a linker error on earlier systems. Not like
	// I'm gonna test it or anything.
	if (QSysInfo::WindowsVersion >= QSysInfo::WV_2000)
	{
#ifdef __LP64__
		MEMORYSTATUSEX statex;
		GlobalMemoryStatusEx(&statex);
		writeLog(QString("Total physical memory: %1 MB (unreliable)").arg(statex.ullTotalPhys/(1024<<10)));
		writeLog(QString("Total virtual memory: %1 MB (unreliable)").arg(statex.ullTotalVirtual/(1024<<10)));
		writeLog(QString("Physical memory in use: %1%").arg(statex.dwMemoryLoad));
#else
		MEMORYSTATUS statex;
		GlobalMemoryStatus(&statex);
		writeLog(QString("Total memory: %1 MB (unreliable)").arg(statex.dwTotalPhys/(1024<<10)));
		writeLog(QString("Total virtual memory: %1 MB (unreliable)").arg(statex.dwTotalVirtual/(1024<<10)));
		writeLog(QString("Physical memory in use: %1%").arg(statex.dwMemoryLoad));
#endif
	}
	else
		writeLog("Windows version too old to get memory info.");

	HKEY hKey = NULL;
	DWORD dwType = REG_DWORD;
	DWORD numVal = 0;
	DWORD dwSize = sizeof(numVal);

	// iterate over the processors listed in the registry
	QString procKey = "Hardware\\Description\\System\\CentralProcessor";
	LONG lRet = ERROR_SUCCESS;
	int i;
	for(i = 0; lRet == ERROR_SUCCESS; i++)
	{
		lRet = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
					TEXT(qPrintable(QString("%1\\%2").arg(procKey).arg(i))),
					0, KEY_QUERY_VALUE, &hKey);

		if(lRet == ERROR_SUCCESS)
		{
			if(RegQueryValueEx(hKey, "~MHz", NULL, &dwType, (LPBYTE)&numVal, &dwSize) == ERROR_SUCCESS)
				writeLog(QString("Processor speed: %1 MHz").arg(numVal));
			else
				writeLog("Could not get processor speed.");
		}

		// can you believe this trash?
		dwType = REG_SZ;
		char nameStr[512];
		DWORD nameSize = sizeof(nameStr);

		if (lRet == ERROR_SUCCESS)
		{
			if (RegQueryValueEx(hKey, "ProcessorNameString", NULL, &dwType, (LPBYTE)&nameStr, &nameSize) == ERROR_SUCCESS)
				writeLog(QString("Processor name: %1").arg(nameStr));
			else
				writeLog("Could not get processor name.");
		}

		RegCloseKey(hKey);
	}
	if(i == 0)
		writeLog("Could not get processor info.");

#elif defined Q_OS_MAC
	QProcess systemProfiler;
	systemProfiler.start("/usr/sbin/system_profiler -detailLevel mini SPHardwareDataType SPDisplaysDataType");
	systemProfiler.waitForStarted();
	systemProfiler.waitForFinished();
	const QString systemData(systemProfiler.readAllStandardOutput());
	QStringList systemLines = systemData.split('\n', QString::SkipEmptyParts);
	for (int i = 0; i<systemLines.size(); i++)
	{
		if(systemLines.at(i).contains("Model"))
		{
			writeLog(systemLines.at(i).trimmed());
		}

		if(systemLines.at(i).contains("Processor"))
		{
			writeLog(systemLines.at(i).trimmed());
		}

		if(systemLines.at(i).contains("Memory"))
		{
			writeLog(systemLines.at(i).trimmed());
		}

		if(systemLines.at(i).contains("VRAM"))
		{
			writeLog(systemLines.at(i).trimmed());
		}

	}

#elif defined Q_OS_BSD4
	QProcess dmesg;
	dmesg.start("/sbin/dmesg", QIODevice::ReadOnly);
	dmesg.waitForStarted();
	dmesg.waitForFinished();
	const QString dmesgData(dmesg.readAll());
	QStringList dmesgLines = dmesgData.split('\n', QString::SkipEmptyParts);
	for (int i = 0; i<dmesgLines.size(); i++)
	{
		if (dmesgLines.at(i).contains("memory"))
		{
			writeLog(dmesgLines.at(i).trimmed());
		}
		if (dmesgLines.at(i).contains("CPU"))
		{
			writeLog(dmesgLines.at(i).trimmed());
		}
		if (dmesgLines.at(i).contains("VGA"))
		{
			writeLog(dmesgLines.at(i).trimmed());
		}
	}
#endif
}

void StelLogger::deinit()
{
	qInstallMsgHandler(0);
	logFile.close();
}

void StelLogger::debugLogHandler(QtMsgType, const char* msg)
{
	fprintf(stderr, "%s\n", msg);
	writeLog(QString(msg));
}

void StelLogger::writeLog(QString msg)
{
	msg += "\n";
	logFile.write(qPrintable(msg), msg.size());
	log += msg;
}
