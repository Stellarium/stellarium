/*
 * Stellarium
 * Copyright (C) 2024 Alexander Wolf
 * Copyright (C) 2025 Ruslan Kabatsayev
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

#include "StelSystemInfo.hpp"
#include "StelUtils.hpp"

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QFile>

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
#include <cstdint>
#endif

// all BSD systems
#if defined Q_OS_BSD4 || defined Q_OS_MACOS
#include <unistd.h>
#include <sys/sysctl.h>
#include <sys/param.h>
#include <sys/types.h>
#endif

#ifdef Q_OS_MACOS
#include <mach/mach.h>
#include <string>
#endif

#ifdef Q_OS_SOLARIS
#include <sys/types.h>
#include <sys/processor.h>
#include <unistd.h>
#endif

#ifdef Q_OS_HAIKU
#include <os/kernel/OS.h>
#include <private/shared/cpu_type.h>
#endif

void printSystemInfo()
{
	const auto log = [](const QString& s){ qInfo().noquote() << s; };

	// write timestamp
	log(QString("%1").arg(QDateTime::currentDateTime().toString(Qt::ISODate)));
	// write info about operating system
	log(QString("Operating System: %1").arg(StelUtils::getOperatingSystemInfo()));
	log(QString("Platform: %1").arg(qApp->platformName()));

	// write compiler version
	log(QString("Compiled using %1").arg(STELLARIUM_COMPILER));

	// write Qt version
	log(QString("Qt runtime version: %1").arg(qVersion()));
	log(QString("Qt compilation version: %1").arg(QT_VERSION_STR));

	// write ABI
	log(QString("Build ABI: %1").arg(QSysInfo::buildAbi()));

	// write addressing mode
        log(QString("Addressing mode: %1").arg(StelUtils::getAddressingMode()));

	// write CPU and memory info
        log(QString("CPU architecture: %1").arg(QSysInfo::currentCpuArchitecture()));

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
		res &= locator->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), nullptr, nullptr, nullptr, 0, nullptr, nullptr, &service);
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
                                log(QString("CPU name: %1").arg(vt_prop.bstrVal));

				hr = obj->Get(L"MaxClockSpeed", 0, &vt_prop, nullptr, nullptr);
                                log(QString("CPU maximum speed: %1 MHz").arg(vt_prop.uintVal));

				hr = obj->Get(L"NumberOfLogicalProcessors", 0, &vt_prop, nullptr, nullptr);
                                log(QString("CPU logical cores: %1").arg(vt_prop.intVal));

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
			log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));

			// GPU info (Enabled only)
			const std::wstring gpu_query(L"SELECT Name, AdapterRAM, CurrentHorizontalResolution, CurrentVerticalResolution FROM Win32_VideoController WHERE Status='OK'");
			service->ExecQuery(bstr_t(L"WQL"), bstr_t(std::wstring(gpu_query.begin(), gpu_query.end()).c_str()), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, nullptr, &enumerator);
			while (enumerator)
			{
				enumerator->Next(WBEM_INFINITE, 1, &obj, &u_return);
				if (!u_return)
					break;

				hr = obj->Get(L"Name", 0, &vt_prop, nullptr, nullptr);
                                log(QString("GPU name: %1").arg(vt_prop.bstrVal));

				hr = obj->Get(L"AdapterRAM", 0, &vt_prop, nullptr, nullptr);
                                log(QString("GPU RAM: %1 MB").arg(vt_prop.ullVal/(1024<<10)));

				hr = obj->Get(L"CurrentHorizontalResolution", 0, &vt_prop, nullptr, nullptr);
				int currHRes = vt_prop.intVal;
				hr = obj->Get(L"CurrentVerticalResolution", 0, &vt_prop, nullptr, nullptr);
				int currVRes = vt_prop.intVal;
				log(QString("Current resolution: %1x%2").arg(currHRes).arg(currVRes));

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
	// CPU info
	size_t size = 0;
	sysctlbyname("machdep.cpu.brand_string", nullptr, &size, nullptr, 0);
	std::string cpuname(size, '\0');
	sysctlbyname("machdep.cpu.brand_string", const_cast<char *>(cpuname.data()), &size, nullptr, 0);
        log(QString("CPU name: %1").arg(cpuname.data()));

	int64_t maxFreq = 0;
	size = sizeof(maxFreq);
	if (sysctlbyname("hw.cpufrequency_max", &maxFreq, &size, nullptr, 0) != -1)
                log(QString("CPU maximum speed: %1 MHz").arg(maxFreq/1000000));
	else
	{
		// Apple Silicon case
		int64_t tbFreq = 0;
		size = sizeof(tbFreq);
		sysctlbyname("hw.tbfrequency", &tbFreq, &size, nullptr, 0);

		struct clockinfo clockinfo;
		size = sizeof(clockinfo);
		sysctlbyname("kern.clockrate", &clockinfo, &size, nullptr, 0);

		log(QString("CPU maximum speed: %1 MHz").arg((tbFreq*clockinfo.hz)/1000000));
	}

	int ncpu = 0;
	size = sizeof(ncpu);
	sysctlbyname("hw.ncpu", &ncpu, &size, nullptr, 0);
        log(QString("CPU logical cores: %1").arg(ncpu));

	// RAM info
	uint64_t totalRAM = 0;
	size = sizeof(totalRAM);
	sysctlbyname("hw.memsize", &totalRAM, &size, nullptr, 0);
	log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));

	// extra info
	sysctlbyname("hw.model", nullptr, &size, nullptr, 0);
	std::string model(size, '\0');
	sysctlbyname("hw.model", const_cast<char *>(model.data()), &size, nullptr, 0);
	log(QString("Model identifier: %1").arg(model.data()));
#endif

#ifdef Q_OS_LINUX
	// CPU info
	QString cpumodel = "unknown", freq = "", hardware = "", model = "", platform = "", machine = "", vendor = "", systype = "";
	int ncpu = 0;
	bool cpuOK = false;
	bool readVendorId = false;
	QFile infoFile("/proc/cpuinfo");
	if (!infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
		log("Could not get CPU info.");
	else
	{
		cpuOK = true;
		bool readCpuModel = true;
                #if defined(__powerpc__) || defined(__powerpc64__)
		bool readClock = true;
                #endif
		while(!infoFile.peek(1).isEmpty())
		{
			QString line = infoFile.readLine();
			line.chop(1);
			if (line.startsWith("processor", Qt::CaseInsensitive))
				ncpu++;

			if (line.startsWith("model name", Qt::CaseInsensitive) && readCpuModel)
			{
				cpumodel = line.split(":").last().trimmed();
				readCpuModel = false;
			}
			#if defined(__powerpc__) || defined(__powerpc64__)
			if (line.startsWith("cpu", Qt::CaseInsensitive) && readCpuModel)
			{
				cpumodel = line.split(":").last().trimmed();
				readCpuModel = false;
			}
			if (line.startsWith("clock", Qt::CaseInsensitive) && readClock)
			{
				double frequency = line.split(":").last().trimmed().replace("MHz", "").toDouble();
				freq = QString("%1 MHz").arg(QString::number(qRound(frequency)));
				readClock = false;
			}
			#endif
                        #if defined(__e2k__) || defined(__s390__) || defined(__s390x__)
			if (line.startsWith("vendor_id", Qt::CaseInsensitive) && !readVendorId)
			{
				vendor = line.split(":").last().trimmed();
				readVendorId = true;
			}
                        #endif
                        #if defined(__aarch64__) || defined(__arm__)
			if (line.startsWith("Processor", Qt::CaseSensitive) && readCpuModel)
			{
				cpumodel = line.split(":").last().trimmed();
				readCpuModel = false;
			}
                        #endif
                        #if defined(__mips__)
			if (line.startsWith("cpu model", Qt::CaseSensitive) && readCpuModel)
			{
				cpumodel = line.split(":").last().trimmed();
				readCpuModel = false;
			}
                        #endif

			// for PowerPC/MIPS computers
			if (line.startsWith("platform", Qt::CaseInsensitive))
				platform = line.split(":").last().trimmed();
			if (line.startsWith("machine", Qt::CaseInsensitive))
				machine = line.split(":").last().trimmed();

			// for ARM-devices, such Raspberry Pi
			if (line.startsWith("hardware", Qt::CaseInsensitive))
				hardware = line.split(":").last().trimmed();
			if (line.startsWith("model", Qt::CaseInsensitive))
				model = line.split(":").last().trimmed();

			// for MIPS computers
			if (line.startsWith("system type", Qt::CaseInsensitive))
				systype = line.split(":").last().trimmed();
		}
		infoFile.close();
	}

	infoFile.setFileName("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq");
	if (infoFile.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		// frequency in kHz: https://www.kernel.org/doc/Documentation/cpu-freq/user-guide.txt
		freq = QString("%1 MHz").arg(QString::number( infoFile.readAll().toInt()/1000));
		infoFile.close();
	}

	if (cpuOK)
	{
		if (readVendorId)
			log(QString("CPU name: %1 %2").arg(vendor, cpumodel));
		else
			log(QString("CPU name: %1").arg(cpumodel));
		if (!freq.isEmpty())
			log(QString("CPU maximum speed: %1").arg(freq));

		log(QString("CPU logical cores: %1").arg(ncpu));

		if (!hardware.isEmpty())
			log(QString("CPU hardware: %1").arg(hardware));
		if (!systype.isEmpty())
			log(QString("System type: %1").arg(systype));
		if (!platform.isEmpty())
			log(QString("Platform: %1").arg(platform));
		if (!model.isEmpty() && (!hardware.isEmpty() || !platform.isEmpty()))
			log(QString("Model: %1").arg(model));
		if (!machine.isEmpty())
			log(QString("Machine: %1").arg(machine));
	}

	// memory info
	struct sysinfo memInfo;
	sysinfo (&memInfo);
	uint64_t totalRAM = memInfo.totalram;
	uint64_t totalVRAM = totalRAM +memInfo.totalswap;
	//Multiply in next statements to avoid int overflow on right hand side...
	totalRAM *= memInfo.mem_unit;
	totalVRAM *= memInfo.mem_unit;

	log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));
	log(QString("Total virtual memory: %1 MB").arg(totalVRAM/(1024<<10)));

#endif

#if defined Q_OS_FREEBSD || defined Q_OS_NETBSD
	const char* _model   = "hw.model";
	const char* _physmem = "hw.physmem";

	#ifdef Q_OS_NETBSD
	_model   = "machdep.cpu_brand";
	_physmem = "hw.physmem64";
	#endif

	// CPU info
	size_t len = 0;
	sysctlbyname(_model, nullptr, &len, nullptr, 0);
	std::string model(len, '\0');
	sysctlbyname(_model, const_cast<char *>(model.data()), &len, nullptr, 0);
	log(QString("CPU name: %1").arg(model.data()));

	int64_t freq = 0;
	len = sizeof(freq);
	if (sysctlbyname("machdep.tsc_freq", &freq, &len, nullptr, 0) != -1)
		log(QString("CPU speed: %1 MHz").arg(freq/1000000)); // FreeBSD and NetBSD (i386/amd64 by default)
	else if (sysctlbyname("hw.clockrate", &freq, &len, nullptr, 0) != -1)
		log(QString("CPU speed: %1 MHz").arg(freq)); // FreeBSD/amd64
	else if (sysctlbyname("hw.freq.cpu", &freq, &len, nullptr, 0) != -1)
		log(QString("CPU speed: %1 MHz").arg(freq)); // FreeBSD/sparc64
	else if (sysctlbyname("dev.cpu.0.freq", &freq, &len, nullptr, 0) != -1)
		log(QString("CPU speed: %1 MHz").arg(freq)); // FreeBSD/powerpc64
	else if (sysctlbyname("hw.cpu0.clock_frequency", &freq, &len, nullptr, 0) != -1)
		log(QString("CPU speed: %1 MHz").arg(freq/1000000)); // NetBSD/sparc64

	int ncpu = 0;
	len = sizeof(ncpu);
	sysctlbyname("hw.ncpu", &ncpu, &len, nullptr, 0);
	log(QString("CPU logical cores: %1").arg(ncpu));

	// memory info
	uint64_t totalRAM = 0;
	len = sizeof(totalRAM);
	sysctlbyname(_physmem, &totalRAM, &len, nullptr, 0);
	log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));
#endif

#ifdef Q_OS_OPENBSD
	int mib[2], freq, ncpu;
	size_t len = 1024;
	std::string model, vendor, machine;
	model.resize(len);
	vendor.resize(len);
	machine.resize(len);

	// CPU info
	mib[0] = CTL_HW;
	mib[1] = HW_MODEL;
	sysctl(mib, 2, model.data(), &len, NULL, 0);
	model.resize(len);
	log(QString("CPU name: %1").arg(model.data()));

	mib[0] = CTL_HW;
	mib[1] = HW_CPUSPEED;
	len = sizeof(freq);
	sysctl(mib, 2, &freq, &len, NULL, 0);
	log(QString("CPU speed: %1 MHz").arg(freq));

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;
	len = sizeof(ncpu);
	sysctl(mib, 2, &ncpu, &len, NULL, 0);
	log(QString("CPU logical cores: %1").arg(ncpu));

	// memory info
	mib[0] = CTL_HW;
	#if defined(HW_PHYSMEM64)
	mib[1] = HW_PHYSMEM64;
	#else
	mib[1] = HW_PHYSMEM;
	#endif
	uint64_t totalRAM = 0;
	len = sizeof(totalRAM);
	sysctl(mib, 2, &totalRAM, &len, NULL, 0);
	log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));

	// extra info
	mib[0] = CTL_HW;
	mib[1] = HW_VENDOR;
	len = 1024;
	sysctl(mib, 2, vendor.data(), &len, NULL, 0);
	vendor.resize(len);
	QString sVendor = QString("%1").arg(vendor.data()).trimmed();
	if (!sVendor.isEmpty())
		log(QString("Vendor: %1").arg(sVendor));

	mib[0] = CTL_HW;
	mib[1] = HW_PRODUCT;
	len = 1024;
	sysctl(mib, 2, machine.data(), &len, NULL, 0);
	machine.resize(len);
	QString sMachine = QString("%1").arg(machine.data()).trimmed();
	if (!sMachine.isEmpty())
		log(QString("Machine: %1").arg(sMachine));
#endif

#ifdef Q_OS_SOLARIS
	processor_info_t pinfo;
	processor_info(0, &pinfo);
	//log(QString("CPU name: %1").arg(pinfo.pi_processor_type));
	log(QString("CPU speed: %1 MHz").arg(pinfo.pi_clock));

	int ncpu = sysconf( _SC_NPROCESSORS_ONLN );
	log(QString("CPU logical cores: %1").arg(ncpu));

	// memory info
	uint64_t totalRAM = (size_t)sysconf( _SC_PHYS_PAGES ) * (size_t)sysconf( _SC_PAGESIZE );
	log(QString("Total physical memory: %1 MB").arg(totalRAM/(1024<<10)));
#endif

#ifdef Q_OS_HAIKU
	// Idea and some code is catching from Haiku OS (MIT license)
	uint32 topologyNodeCount = 0;

	get_cpu_topology_info(NULL, &topologyNodeCount);
	std::unique_ptr<cpu_topology_node_info> topology(topologyNodeCount != 0 ? new cpu_topology_node_info[topologyNodeCount] : NULL);
	get_cpu_topology_info(topology.get(), &topologyNodeCount);

	enum cpu_platform platform = B_CPU_UNKNOWN;
	enum cpu_vendor cpuVendor = B_CPU_VENDOR_UNKNOWN;
	uint32 cpuModel = 0;

	for (uint32 i = 0; i < topologyNodeCount; i++)
	{
		switch (topology.get()[i].type)
		{
			case B_TOPOLOGY_ROOT:
				platform = topology.get()[i].data.root.platform;
				break;
			case B_TOPOLOGY_PACKAGE:
				cpuVendor = topology.get()[i].data.package.vendor;
				break;
			case B_TOPOLOGY_CORE:
				cpuModel = topology.get()[i].data.core.model;
				break;
			default:
				break;
		}
	}

	int32 frequency = get_rounded_cpu_speed();
	QString clockSpeed;
	if (frequency < 1000)
		clockSpeed = QString("%1 MHz").arg(frequency);
	else
		clockSpeed = QString("%1 GHz").arg(QString::number(frequency/1000.f, 'f', 2));

        log(QString("CPU name: %1 %2 @ %3").arg(get_cpu_vendor_string(cpuVendor), get_cpu_model_string(platform, cpuVendor, cpuModel), clockSpeed));
        log(QString("CPU speed: %1 MHz").arg(frequency));

	system_info hwinfo;
	get_system_info(&hwinfo);

        log(QString("CPU logical cores: %1").arg(hwinfo.cpu_count));

	// memory info
	uint64_t totalRAM = round(hwinfo.max_pages*B_PAGE_SIZE/1048576.0 + hwinfo.ignored_pages*B_PAGE_SIZE/1048576.0);
	log(QString("Total physical memory: %1 MB").arg(totalRAM));
#endif
}
