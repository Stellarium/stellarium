/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \namespace windozeHelpers 
* \brief namespace with helper functions for windows usb and win32api funcitons 
* 
*/ 


#ifndef WINDOZEHELPERS_INCLUDE_H__ 
#define WINDOZEHELPERS_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>
#include <atlstr.h>
#include "Setupapi.h"

namespace windozeHelpers 
{ 
    bool EnumerateDevice(const uint16_t DeviceNum, 
        HDEVINFO & deviceInfo, 
        SP_DEVICE_INTERFACE_DATA & interfaceData,
        std::string & errMsg );
    /*!
     * Returns the result of the GetLastError function call as a
     * std::string
     * \return std::string
     */
    std::string GetLastWinError();

    /*!
     * Searches for an Apogee with the input device number
     * \param [in] DeviceNum Device number to look for
     * \return true = device found, false = device not found.
     */
    bool Search4UsbDevice( const uint16_t DeviceNum );

    /*!
     * Returns the UNC path for the Apogee device at the input DeviceNum.
     * Throws a std::runtime exception if the device is not found.
     * \param [in] DeviceNum Device number to look for
     * \param [out] path UNC device path
     */
    void FetchUsbDevicePath( const uint16_t DeviceNum, CString & path );

    void FetchUsbDriverVersion( const uint16_t DeviceNum, std::string & version );

    void GetVidAndPid( const uint16_t DeviceNum, uint16_t & vid,
                                   uint16_t & pid );

    std::vector< std::vector<uint16_t> > GetDevicesWin();

    bool IsDeviceAlreadyOpen( uint16_t deviceNum );
}; 

#endif