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
#include "StelUtils.hpp"

#include <QDateTime>
#include <QProcess>
#ifdef Q_OS_WIN
 #include <Windows.h>
#endif

// Init statics variables.
QFile StelLogger::logFile;
QString StelLogger::log;
QMutex StelLogger::fileMutex;

void StelLogger::init(const QString& logFilePath)
{
	logFile.setFileName(logFilePath);

	if (logFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text | QIODevice::Unbuffered))
		qInstallMessageHandler(StelLogger::debugLogHandler);

	// write timestamp
	writeLog(QString("%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
	// write info about operating system
	writeLog(QString("Operating System: %1").arg(StelUtils::getOperatingSystemInfo()));	

	// write compiler version
	QString compiler = StelUtils::getCompilerInfo();
	writeLog(compiler.isEmpty() ? "Unknown compiler" : QString("Compiled using %1").arg(compiler));

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
#ifdef Q_OS_LINUX

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

	QProcess lspci;
	lspci.start("lspci", { "-v" }, QIODevice::ReadOnly);
	lspci.waitForFinished(300);
	const QString pciData(lspci.readAll());
	#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
	QStringList pciLines = pciData.split('\n', Qt::SkipEmptyParts);
	#else
	QStringList pciLines = pciData.split('\n', QString::SkipEmptyParts);
	#endif
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
#endif

// Aargh Windows API
#elif defined Q_OS_WIN
	MEMORYSTATUSEX statex;
	statex.dwLength = sizeof (statex);
	GlobalMemoryStatusEx(&statex);
	writeLog(QString("Total physical memory: %1 MB").arg(statex.ullTotalPhys/(1024<<10)));
	writeLog(QString("Available physical memory: %1 MB").arg(statex.ullAvailPhys/(1024<<10)));
	writeLog(QString("Physical memory in use: %1%").arg(statex.dwMemoryLoad));
	#ifndef _WIN64
	// This always reports about 8TB on Win64, not really useful to show.
	writeLog(QString("Total virtual memory: %1 MB").arg(statex.ullTotalVirtual/(1024<<10)));
	writeLog(QString("Available virtual memory: %1 MB").arg(statex.ullAvailVirtual/(1024<<10)));
	#endif

	HKEY hKey = Q_NULLPTR;
	DWORD dwType = REG_DWORD;
	DWORD numVal = 0;
	DWORD dwSize = sizeof(numVal);

	// iterate over the processors listed in the registry
	QString procKey = "Hardware\\Description\\System\\CentralProcessor";
	LONG lRet = ERROR_SUCCESS;
	int i;
	for(i = 0; lRet == ERROR_SUCCESS; i++)
	{
		lRet = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
					qPrintable(QString("%1\\%2").arg(procKey).arg(i)),
					0, KEY_QUERY_VALUE, &hKey);

		if(lRet == ERROR_SUCCESS)
		{
			if(RegQueryValueExA(hKey, "~MHz", Q_NULLPTR, &dwType, reinterpret_cast<LPBYTE>(&numVal), &dwSize) == ERROR_SUCCESS)
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
			if (RegQueryValueExA(hKey, "ProcessorNameString", Q_NULLPTR, &dwType, reinterpret_cast<LPBYTE>(&nameStr), &nameSize) == ERROR_SUCCESS)
				writeLog(QString("Processor name: %1").arg(nameStr));
			else
				writeLog("Could not get processor name.");
		}

		RegCloseKey(hKey);
	}
	if(i == 0)
		writeLog("Could not get processor info.");

#elif defined Q_OS_MACOS
	QProcess systemProfiler;
	systemProfiler.start("/usr/sbin/system_profiler", {"-detailLevel mini SPHardwareDataType SPDisplaysDataType"});
	systemProfiler.waitForStarted();
	systemProfiler.waitForFinished();
	const QString systemData(systemProfiler.readAllStandardOutput());
	#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
	QStringList systemLines = systemData.split('\n', Qt::SkipEmptyParts);
	#else
	QStringList systemLines = systemData.split('\n', QString::SkipEmptyParts);
	#endif
	for (int i = 0; i<systemLines.size(); i++)
	{
		// hardware overview
		if(systemLines.at(i).contains("Model", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed());

		if(systemLines.at(i).contains("Chip:", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed());

		if(systemLines.at(i).contains("Processor", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed().replace("Unknown", "Apple M1"));

		if(systemLines.at(i).contains("Memory", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed());

		// graphics/display overview
		if(systemLines.at(i).contains("Resolution", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed());

		if(systemLines.at(i).contains("VRAM", Qt::CaseInsensitive))
			writeLog(systemLines.at(i).trimmed());
	}

#elif defined Q_OS_BSD4
	QProcess dmesg;
	dmesg.start("/sbin/dmesg", {}, QIODevice::ReadOnly);
	dmesg.waitForStarted();
	dmesg.waitForFinished();
	const QString dmesgData(dmesg.readAll());
	QStringList dmesgLines = dmesgData.split('\n', Qt::SkipEmptyParts);
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
	qInstallMessageHandler(Q_NULLPTR);
	logFile.close();
}

void StelLogger::debugLogHandler(QtMsgType type, const QMessageLogContext& ctx, const QString& msg)
{
	// *** NOTE: see original Qt source in qlogging.cpp (qDefaultMessageHandler) for sensible default code

	//use Qt to format the log message, if possible
	//this uses the format set by qSetMessagePattern
	//or QT_MESSAGE_PATTERN environment var
	QString fmt = qFormatLogMessage(type,ctx,msg);
	//do nothing for null messages
	if(fmt.isNull())
		return;

	//always append newline
	fmt.append(QLatin1Char('\n'));

#ifdef Q_OS_WIN
	//Send debug messages to Debugger, if one is attached, instead of stderr
	//This seems to avoid output delays in Qt Creator, allowing for easier debugging
	//Seems to work fine with MSVC and MinGW
	if (IsDebuggerPresent())
	{
		OutputDebugStringW(reinterpret_cast<const wchar_t *>(fmt.utf16()));
	}
	else
#endif
	{
		//this does the same as the default handler in qlogging.cpp
		fprintf(stderr, "%s", qPrintable(fmt));
		fflush(stderr);
	}
	writeLog(fmt);
}

void StelLogger::writeLog(QString msg)
{
	if (!msg.endsWith('\n'))
		msg.append(QLatin1Char('\n'));

	fileMutex.lock();
	const auto utf8 = msg.toUtf8();
	logFile.write(utf8.constData(), utf8.size());
	log += msg;
	fileMutex.unlock();
}
