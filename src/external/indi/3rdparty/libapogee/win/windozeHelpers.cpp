/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class windozeHelpers 
* \brief namespace with helper functions for windows usb and win32api funcitons 
* 
*/ 


#include <windows.h>
#include "windozeHelpers.h" 



#include <sstream>

#include "GUID_USB.h"
#include "apgHelper.h" 

namespace
{

    const uint16_t USB_MAX_DEVICES = 255;
}

  
//////////////////////////// 
//      ENUMERATE         DEVICE
bool windozeHelpers::EnumerateDevice(const uint16_t DeviceNum, HDEVINFO & deviceInfo, 
                                     SP_DEVICE_INTERFACE_DATA & interfaceData, 
                                     std::string & errMsg )
{
    //most of the code for this function came from
    //http://www.osronline.com/article.cfm?id=532

    LPGUID DeviceGuid = (LPGUID)&GUID_USBWDM_DEVICE_INTERFACE_CLASS;

    //
    // Find device information with our GUID.
    //
    deviceInfo = SetupDiGetClassDevs(
            DeviceGuid,
		    NULL, NULL,
		    DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);

    if( INVALID_HANDLE_VALUE == deviceInfo ) 
    {
        errMsg = "SetupDiGetClassDevs failed with error "
               + windozeHelpers::GetLastWinError();
        
        return false;
    }

    // Enumerate the devices that support this guid.
    memset(&interfaceData,0,sizeof(interfaceData));
    interfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    BOOL result = SetupDiEnumDeviceInterfaces(deviceInfo,
							    NULL,
							    DeviceGuid,
							    DeviceNum,
							    &interfaceData);
    if( !result )
    {
        errMsg = "SetupDiEnumDeviceInterfaces failed with error "
                    + windozeHelpers::GetLastWinError();
        
        return false;
    }

    return true;
}


//////////////////////////// 
// GET      LAST        WIN     ERROR
std::string windozeHelpers::GetLastWinError()
{
    std::stringstream ss;
    ss << GetLastError();
    return ss.str();
}

//////////////////////////// 
//  SEARCH      4       USB     DEVICE
bool windozeHelpers::Search4UsbDevice( const uint16_t DeviceNum )
{
    HDEVINFO deviceInfo = 0;
    SP_DEVICE_INTERFACE_DATA interfaceData;

    std::string errMsg;
    const bool result = EnumerateDevice(DeviceNum, deviceInfo, interfaceData, errMsg);


    return( result );
}


//////////////////////////// 
// FETCH    USB     DEVICE      PATH
void windozeHelpers::FetchUsbDevicePath( const uint16_t DeviceNum, CString & path )
{
    //most of the code for this function came from
    //http://www.osronline.com/article.cfm?id=532

    HDEVINFO deviceInfo = 0;
    SP_DEVICE_INTERFACE_DATA interfaceData;
    
    std::string msg;

    if( !EnumerateDevice(DeviceNum, deviceInfo, interfaceData, msg) )
    {
        apgHelper::throwRuntimeException( __FILE__, msg, __LINE__, Apg::ErrorType_Connection );
    }

    //
    // Get information about the found device interface.  We don't
    // know how much memory to allocate to get this information, so
    // we will ask by passing in a null buffer and location to
    // receive the size of the buffer needed.
    //
    ULONG requiredLength=0;
    BOOL result = SetupDiGetDeviceInterfaceDetail(deviceInfo,
		                  &interfaceData,
		                  NULL, 0,
		                  &requiredLength,
		                  NULL);

    if(!requiredLength) 
    {
        SetupDiDestroyDeviceInfoList(deviceInfo);

        std::string errMsg = "Failed to get device memory size.  Last error = "
            + windozeHelpers::GetLastWinError();
        apgHelper::throwRuntimeException( __FILE__, errMsg, __LINE__, Apg::ErrorType_Connection );
    }

    //
    // Okay, we got a size back, so let's allocate memory 
    // for the interface detail information we want.
    //
    PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = 
        (PSP_DEVICE_INTERFACE_DETAIL_DATA) LocalAlloc(LMEM_FIXED,requiredLength);

    if( detailData == 0) 
    {
	    SetupDiDestroyDeviceInfoList(deviceInfo);

	    std::string errMsg = "LocalAlloc failed.  Last error = "
            + windozeHelpers::GetLastWinError();
        apgHelper::throwRuntimeException( __FILE__, errMsg, __LINE__, Apg::ErrorType_Connection );
    }

    detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
    ULONG length = requiredLength;

    result = SetupDiGetDeviceInterfaceDetail(deviceInfo,
							       &interfaceData,
							       detailData,
							       length,
							       &requiredLength,
							       NULL);

    if( !result )  
    {
	    SetupDiDestroyDeviceInfoList(deviceInfo);
	    LocalFree(detailData);

        std::string errMsg = "SetupDiGetDeviceInterfaceDetail error "
            + windozeHelpers::GetLastWinError();
        apgHelper::throwRuntimeException( __FILE__, errMsg, __LINE__, Apg::ErrorType_Connection );
    }


    //store the path
    path = detailData->DevicePath;
    SetupDiDestroyDeviceInfoList(deviceInfo);
	LocalFree(detailData);

}

//////////////////////////// 
// FETCH    USB     DRIVER      VERSION
void windozeHelpers::FetchUsbDriverVersion( const uint16_t DeviceNum, std::string & version )
{
    HDEVINFO deviceInfo = 0;
    SP_DEVICE_INTERFACE_DATA interfaceData;

     std::string msg;

    if( !EnumerateDevice( DeviceNum, deviceInfo, interfaceData, msg ) )
    {
        apgHelper::throwRuntimeException( __FILE__, msg, __LINE__, Apg::ErrorType_Connection );
    }

    
    SP_DEVINFO_DATA deviceData;
    deviceData.cbSize = sizeof(SP_DEVINFO_DATA);
    if( !SetupDiEnumDeviceInfo( deviceInfo, DeviceNum, &deviceData ) )
    {
        //getting the windows error message b4 the next winapi calls
        //nukes it out.  then throw the error
        std::string errMsg = "SetupDiEnumDeviceInfo failed "
            + windozeHelpers::GetLastWinError();

        SetupDiDestroyDeviceInfoList(deviceInfo);

        apgHelper::throwRuntimeException( __FILE__, 
           errMsg , __LINE__,Apg::ErrorType_Connection );
    }
  
    //from BOOL DumpDeviceDriverNodes in dump.cpp in 
    //C:\WinDDK\6001.18002\src\setup\devcon
    SP_DEVINSTALL_PARAMS deviceInstallParams;
    SP_DRVINFO_DATA driverInfoData;

    ZeroMemory(&deviceInstallParams, sizeof(deviceInstallParams));
    ZeroMemory(&driverInfoData, sizeof(driverInfoData));

    driverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    deviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if( !SetupDiGetDeviceInstallParams(deviceInfo, &deviceData, 
        &deviceInstallParams) )
    {
       std::string errMsg = "SetupDiGetDeviceInstallParams failed "
            + windozeHelpers::GetLastWinError();

        SetupDiDestroyDeviceInfoList(deviceInfo);

        apgHelper::throwRuntimeException( __FILE__, 
           errMsg , __LINE__,Apg::ErrorType_Connection );
    }

    //
    // Set the flags that tell SetupDiBuildDriverInfoList to allow excluded drivers.
    //
    deviceInstallParams.FlagsEx |= (DI_FLAGSEX_INSTALLEDDRIVER | DI_FLAGSEX_ALLOWEXCLUDEDDRVS);

    if( !SetupDiSetDeviceInstallParams(deviceInfo, &deviceData, 
        &deviceInstallParams) )
    {
         std::string errMsg = "SetupDiGetDeviceInstallParams failed "
            + windozeHelpers::GetLastWinError();

        SetupDiDestroyDeviceInfoList(deviceInfo);

        apgHelper::throwRuntimeException( __FILE__, 
           errMsg , __LINE__,Apg::ErrorType_Connection );
    }

    if( !SetupDiBuildDriverInfoList(deviceInfo, &deviceData, 
        SPDIT_COMPATDRIVER) )
    {
         std::string errMsg = "SetupDiBuildDriverInfoList failed "
            + windozeHelpers::GetLastWinError();

        SetupDiDestroyDeviceInfoList(deviceInfo);

        apgHelper::throwRuntimeException( __FILE__, 
           errMsg , __LINE__,Apg::ErrorType_Connection );
    }

  
    // believe the member index should be set to 0 for all
    // devices...not 100% sure what this parameter is referring
    // to, the number of devices, the number of driver interfaces,
    // number of device interfaces????
    // http://msdn.microsoft.com/en-us/library/windows/hardware/ff551018(v=vs.85).aspx
    if( !SetupDiEnumDriverInfo(deviceInfo, &deviceData, 
        SPDIT_COMPATDRIVER, 0, &driverInfoData) )
    {
        std::string errMsg = "SetupDiEnumDriverInfo failed "
            + windozeHelpers::GetLastWinError();

        SetupDiDestroyDriverInfoList(deviceInfo, &deviceData,
        SPDIT_COMPATDRIVER);
        SetupDiDestroyDeviceInfoList(deviceInfo);

        apgHelper::throwRuntimeException( __FILE__, 
           errMsg , __LINE__,Apg::ErrorType_Connection );
    }
 
    ULARGE_INTEGER Version;
    Version.QuadPart = driverInfoData.DriverVersion;
    
    std::stringstream ss;
    ss << HIWORD(Version.HighPart) << ".";
    ss << LOWORD(Version.HighPart) << ".";
    ss << HIWORD(Version.LowPart) << ".";
    ss << LOWORD(Version.LowPart);
                       
    version = ss.str();

    SetupDiDestroyDriverInfoList(deviceInfo, &deviceData,
        SPDIT_COMPATDRIVER);
    SetupDiDestroyDeviceInfoList(deviceInfo);

}

//////////////////////////// 
// GET     VID   AND   PID
void windozeHelpers::GetVidAndPid( const uint16_t DeviceNum, 
                                  uint16_t & vid,
                                  uint16_t & pid )
{

    CString Path;
    windozeHelpers::FetchUsbDevicePath( DeviceNum, Path );
    
    CString vv( _T("vid_") );
    int32_t start = Path.Find( vv );
    int32_t stop = Path.Find( _T("&"), start);
    start = start + vv.GetLength();
    int32_t len = stop-start;
    

    CString vidStr = Path.Mid( start, len );

    vid = static_cast<uint16_t>( _tcstoul(vidStr, 0, 16) );

    CString pp( _T("pid_") );
    start = Path.Find( pp );
    stop =Path.Find( _T("#"), start);
    start = start + pp.GetLength();
    len = stop-start;
    

    CString pidStr = Path.Mid( start, len );

    pid = static_cast<uint16_t>( _tcstoul(pidStr, 0, 16) );
}


////////////////////////////
// GET		DEVICES		WIN
std::vector< std::vector<uint16_t> > windozeHelpers::GetDevicesWin()
{
    std::vector< std::vector<uint16_t> > deviceVect;

    for( uint16_t i=0; i< USB_MAX_DEVICES; ++i )
    {
        if( !windozeHelpers::Search4UsbDevice( i ) )
        {
            //didn't find anything return to the 
            //top for more instructions ;>
            continue;
        }

        //got one, get the info, store, next
        uint16_t vid = 0, pid = 0;
        windozeHelpers::GetVidAndPid( i, vid, pid );

        std::vector<uint16_t> temp; 
        temp.push_back( i );
        temp.push_back( vid );
        temp.push_back( pid );

        deviceVect.push_back( temp );
    }

    return deviceVect;
}

////////////////////////////
//      IS     DEVICE      ALREADY       OPEN
bool windozeHelpers::IsDeviceAlreadyOpen( const uint16_t deviceNum )
{
    CString Path;
    windozeHelpers::FetchUsbDevicePath( deviceNum, Path );

    // Open the device
    HANDLE hDevice = CreateFile( Path,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL );

    const bool result = INVALID_HANDLE_VALUE == hDevice ? true : false;

    if( false == result )
    {
        CloseHandle( hDevice );
    }

    return result;
}