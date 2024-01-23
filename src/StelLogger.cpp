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
#include <windows.h>
#include <WbemIdl.h>
#include <comdef.h>
#include <string>
#pragma comment(lib, "wbemuuid.lib")
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

	// write CPU and memory info
	writeLog(QString("Processor architecture: %1").arg(QSysInfo::currentCpuArchitecture()));

#ifdef Q_OS_WIN
	// Use WMI
	// Idea and some code is catching from HWInfo project (MIT license)
	IWbemLocator* locator = nullptr;
	IWbemServices* service = nullptr;
	IEnumWbemClassObject* enumerator = nullptr;

	auto res = CoInitializeSecurity(nullptr, -1, nullptr, nullptr, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE, nullptr);
	res &= CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	res &= CoCreateInstance(CLSID_WbemLocator, nullptr, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&locator);
	if (locator)
	{
		res &= locator->ConnectServer(_bstr_t("ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &service);
		if (service)
		{
			res &= CoSetProxyBlanket(service, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, nullptr, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE);

			VARIANT vt_prop;
			HRESULT hr;
			ULONG u_return = 0;
			IWbemClassObject* obj = nullptr;

			// CPU info
			const std::wstring cpu_query(L"SELECT Name, NumberOfLogicalProcessors, MaxClockSpeed FROM Win32_Processor");
			service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(cpu_query.begin(), cpu_query.end()).c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
			while (enumerator)
			{
				enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
				if (!u_return)
					break;

				hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Processor name: %1").arg(vt_prop.bstrVal));

				hr = obj->Get(L"MaxClockSpeed", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Processor maximum speed: %1 MHz").arg(vt_prop.uintVal));

				hr = obj->Get(L"NumberOfLogicalProcessors", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Processor logical cores: %1").arg(vt_prop.intVal));

				VariantClear(&vt_prop);
				obj->Release();
			}

			// RAM info
			int64_t totalRAM = 0;
			const std::wstring ram_query(L"SELECT Capacity FROM Win32_PhysicalMemory");
			service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(ram_query.begin(), ram_query.end()).c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
			while (enumerator)
			{
				enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
				if (!u_return)
					break;

				hr = obj->Get(L"Capacity", 0, &vt_prop, nullptr, nullptr);
				totalRAM += std::stoll(vt_prop.bstrVal);

				VariantClear(&vt_prop);
				obj->Release();
			}
			writeLog(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));

			// GPU info
			const std::wstring gpu_query(L"SELECT Name FROM Win32_VideoController");
			service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(gpu_query.begin(), gpu_query.end()).c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
			while (enumerator)
			{
				enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
				if (!u_return)
					break;

				hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Video controller name: %1").arg(vt_prop.bstrVal));

				VariantClear(&vt_prop);
				obj->Release();
			}
		}
	}

	if (locator) locator->Release();
	if (service) service->Release();
	CoUninitialize();
#endif

/*

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

	// REMOVED

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
	#if (QT_VERSION>=QT_VERSION_CHECK(5, 14, 0))
	QStringList dmesgLines = dmesgData.split('\n', QString::SkipEmptyParts);
	#else
	QStringList dmesgLines = dmesgData.split('\n', Qt::SkipEmptyParts);
	#endif
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
*/
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
