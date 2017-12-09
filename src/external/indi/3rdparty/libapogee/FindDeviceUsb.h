/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class FindDeviceUsb 
* \brief Searches usb bus for apogee devices.
* 
*/ 


#ifndef FINDDEVICEUSB_INCLUDE_H__ 
#define FINDDEVICEUSB_INCLUDE_H__ 

#include <string>
#include <vector>
#include <stdint.h>
#include "DefDllExport.h"

class CamUsbIo;

class DLL_EXPORT FindDeviceUsb 
{ 
    public: 
        /*! */
        virtual ~FindDeviceUsb();

        /*! 
         * Searches the USB bus for Apogee devices
         * \return Returns a <d></d> demlited string for found devices.  A
         * null string of <d></d> is returned if no device is found.  This example
         * shows the string return with one camera and one filter wheel is attached
         * to the host PC,
         * \verbatim <d>address=0,interface=usb,deviceType=camera,id=0x49,firmwareRev=0x21,model=AltaU-4020ML,interfaceStatus=NA</d><d>address=1,interface=usb,model=Filter Wheel,deviceType=filterWheel,id=0xFFFF,firmwareRev=0xFFEE</d> \endverbatim
         */
        std::string Find();

    private:
        std::string AltaInfo( const std::string & deviceAddr );
        std::string AscentInfo( const std::string & deviceAddr );
        std::string AspenInfo( const std::string & deviceAddr );

        std::string MkCamInfoStr( uint16_t Id, 
            uint16_t FrmwrRev );

        std::vector< std::vector<uint16_t> > GetApgDevices();
        std::string CameraInfo(CamUsbIo & usbIo);
        bool IsDeviceAlreadyOpen( uint16_t deviceNum );
}; 

#endif
