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

#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/sysinfo.h>
#endif

// all BSD systems
#if defined Q_OS_BSD4 || defined Q_OS_MACOS
#include <sys/sysctl.h>
// specific for NetBSD
#ifdef Q_OS_NETBSD
#include <sys/param.h>
#endif
// specific for OpenBSD
#ifdef Q_OS_OPENBSD
#include <sys/types.h>
#endif
#endif

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <string>
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

	// write ABI
	writeLog(QString("Build ABI: %1").arg(QSysInfo::buildAbi()));

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
			const std::wstring gpu_query(L"SELECT Name, AdapterRAM, CurrentHorizontalResolution, CurrentVerticalResolution FROM Win32_VideoController");
			service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(gpu_query.begin(), gpu_query.end()).c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
			while (enumerator)
			{
				enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
				if (!u_return)
					break;

				hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Video controller name: %1").arg(vt_prop.bstrVal));

				hr = obj->Get(L"AdapterRAM", 0, &vt_prop, nullptr, nullptr);
				writeLog(QString("Video controller RAM: %1 MB").arg(vt_prop.ullVal/(1024<<10)));

				hr = obj->Get(L"CurrentHorizontalResolution", 0, &vt_prop, nullptr, nullptr);
				int currHRes = vt_prop.intVal;
				hr = obj->Get(L"CurrentVerticalResolution", 0, &vt_prop, nullptr, nullptr);
				int currVRes = vt_prop.intVal;
				writeLog(QString("Current resolution: %1x%2").arg(currHRes).arg(currVRes));

				VariantClear(&vt_prop);
				obj->Release();
			}
		}
	}

	if (locator) locator->Release();
	if (service) service->Release();
	CoUninitialize();
#endif

#ifdef Q_OS_MACOS
	size_t size = 1024;
	std::string cpuname;
	cpuname.resize(size);
	if (sysctlbyname("machdep.cpu.brand_string", cpuname.data(), &size, nullptr, 0) < 0)
		cpuname = "unknown";
	else
		cpuname.resize(size);
	writeLog(QString("Processor name: %1").arg(cpuname.data()));

	int64_t maxFreq = 0;
	size = sizeof(maxFreq);
	if (sysctlbyname("hw.cpufrequency_max", &maxFreq, &size, nullptr, 0) != -1)
		writeLog(QString("Processor maximum speed: %1 MHz").arg(maxFreq/1000000));

	int ncpu = 0;
	size = sizeof(ncpu);
	sysctlbyname("hw.logicalcpu", &ncpu, &size, nullptr, 0);
	writeLog(QString("Processor logical cores: %1").arg(ncpu));

	int64_t totalRAM = 0;
	size = sizeof(totalRAM);
	sysctlbyname("hw.memsize", &totalRAM, &size, nullptr, 0);
	writeLog(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));

	// extra info
	size_t len = 0;
	sysctlbyname("hw.model", nullptr, &len, nullptr, 0);
	std::string model(len, '\0');
	sysctlbyname("hw.model", const_cast<char *>(model.data()), &len, nullptr, 0);
	writeLog(QString("Model identifier: %1").arg(model.data()));
#endif

#ifdef Q_OS_LINUX
	// CPU info
	QString cpumodel = "unknown", hardware = "", model = "";
	int ncpu = 0;
	bool cpuOK = false;
	QFile infoFile("/proc/cpuinfo");
	if (!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		writeLog("Could not get CPU info.");
	else
	{
		cpuOK = true;
		bool readModel = true;
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if (line.startsWith("processor", Qt::CaseInsensitive))
				ncpu++;

			if (line.startsWith("model name", Qt::CaseInsensitive) && readModel)
			{
				cpumodel = line.split(":").last().trimmed();
				readModel = false;
			}

			// for ARM-devices, such Raspberry Pi
			if (line.startsWith("hardware", Qt::CaseInsensitive))
				hardware = line.split(":").last().trimmed();
			if (line.startsWith("model", Qt::CaseInsensitive) && !hardware.isEmpty())
				model = line.split(":").last().trimmed();
		}
		infoFile.close();
	}

	QString freq = "unknown";
	infoFile.setFileName("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	if (infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// frequency in kHz: https://www.kernel.org/doc/Documentation/cpu-freq/user-guide.txt
		freq = QString::number( infoFile.readAll().toInt()/1000);
		infoFile.close();
	}

	if (cpuOK)
	{
		writeLog(QString("Processor name: %1").arg(cpumodel));
		if (!hardware.isEmpty())
			writeLog(QString("Processor hardware: %1").arg(hardware));
		if (!model.isEmpty())
			writeLog(QString("Device model: %1").arg(model));
		writeLog(QString("Processor maximum speed: %1 MHz").arg(freq));
		writeLog(QString("Processor logical cores: %1").arg(ncpu));
	}

	// memory info
	struct sysinfo memInfo;
	sysinfo (&memInfo);
	long long totalRAM = memInfo.totalram;
	long long totalVRAM = totalRAM +memInfo.totalswap;
	//Multiply in next statements to avoid int overflow on right hand side...
	totalRAM *= memInfo.mem_unit;
	totalVRAM *= memInfo.mem_unit;

	writeLog(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));
	writeLog(QString("Total virtual memory: %1 MB").arg(totalVRAM/(1024<<10)));

#endif

#if defined Q_OS_BSD4 && !defined Q_OS_MACOS
	const char* _model = "hw.model";
	const char* _freq = "machdep.tsc_freq";
	const char* _ncpu = "hw.ncpu";
	const char* _physmem = "hw.physmem64";

	#ifdef Q_OS_FREEBSD
	_physmem  = "hw.physmem";
	#endif

	#ifdef Q_OS_NETBSD
	_model       = "machdep.cpu_brand";
	#endif

	#ifdef Q_OS_OPENBSD
	_freq          = "hw.cpuspeed";
	#endif

	// CPU info
	size_t len = 0;
	sysctlbyname(_model, nullptr, &len, nullptr, 0);
	std::string model(len, '\0');
	sysctlbyname(_model, const_cast<char *>(model.data()), &len, nullptr, 0);
	writeLog(QString("Processor name: %1").arg(model.data()));

	#ifdef Q_OS_OPENBSD
	int freq = -1;
	len = sizeof(freq);
	sysctlbyname(_freq, &freq, &len, nullptr, 0);
	writeLog(QString("Processor speed: %1 MHz").arg(freq));
	#else
	int64_t freq = 0;
	len = sizeof(freq);
	sysctlbyname(_freq, &freq, &len, nullptr, 0);
	writeLog(QString("Processor speed: %1 MHz").arg(freq/1000000));
	#endif

	int ncpu = 0;
	len = sizeof(ncpu);
	sysctlbyname(_ncpu, &ncpu, &len, nullptr, 0);
	writeLog(QString("Processor logical cores: %1").arg(ncpu));

	// memory info
	int64_t totalRAM = 0;
	len = sizeof(totalRAM);
	sysctlbyname(_physmem, &totalRAM, &len, nullptr, 0);
	writeLog(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));
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
