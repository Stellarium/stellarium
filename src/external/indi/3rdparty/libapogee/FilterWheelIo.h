/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class FilterWheelIo
* \brief Derived fx2 class for the usb filter wheel 
* 
*/ 


#ifndef FILTERWHEELIO_INCLUDE_H__ 
#define FILTERWHEELIO_INCLUDE_H__ 


#include "DefDllExport.h"
#include <stdint.h>
#include <string>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

class IUsb;

class DLL_EXPORT FilterWheelIo
{ 
    public: 
        FilterWheelIo( const std::string & DeviceAddr ); 
        virtual ~FilterWheelIo(); 

        void Program( const std::string & FilenameFx2, 
            const std::string & FilenameDescriptor );

        uint16_t GetVendorId();
        uint16_t GetProductId();
        uint16_t GetDeviceId();

        std::string GetUsbFirmwareRev();

        void ReadCtrlPort( uint8_t & control, uint8_t & pin );
        void WriteCtrlPort( uint8_t control, uint8_t pin );

    private:
        void DownloadFirmware();
         std::string m_fileName;

          void IncrEepromAddrBlockBank(uint16_t IncrSize,
            uint16_t & Addr, uint8_t & Bank, 
            uint8_t & Block);

//this code removes vc++ compiler warning C4251
//from http://www.unknownroad.com/rtfm/VisualStudio/warningC4251.html
#ifdef WIN_OS
#if _MSC_VER < 1600
        template class DLL_EXPORT std::shared_ptr<IUsb>;
#endif
#endif

          std::shared_ptr<IUsb> m_Usb; 
}; 

#endif
