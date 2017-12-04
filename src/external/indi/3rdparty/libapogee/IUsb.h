/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class IUsb 
* \brief Interface class for usb io 
* 
*/ 


#ifndef IUSB_INCLUDE_H__ 
#define IUSB_INCLUDE_H__ 

#include <vector>
#include <string>
#include <stdint.h>

class IUsb 
{ 
    public: 
        virtual ~IUsb(); 

        virtual uint16_t ReadReg(uint16_t FpgaReg ) = 0;

        virtual void WriteReg(uint16_t FpgaReg, 
            const uint16_t FpgaData ) = 0;

        virtual void GetVendorInfo( uint16_t & VendorId,
            uint16_t & ProductId, uint16_t  & DeviceId) = 0;

        virtual void SetupSingleImgXfer(uint16_t Rows, 
            uint32_t Cols) = 0;

        virtual void SetupSequenceImgXfer(uint16_t Rows,
            uint16_t Cols, uint16_t NumOfImages) = 0;

        virtual void CancelImgXfer() = 0;

        virtual void ReadImage( uint16_t * ImageData, 
					            const uint32_t InSizeInBytes,
					            uint32_t &OutSizeInBytes) = 0;	

        virtual void GetStatus(uint8_t * status, uint32_t NumBytes) = 0;

        virtual void UsbRequestIn(uint8_t RequestCode,
            uint16_t Index, uint16_t	Value,
            uint8_t * ioBuf, uint32_t BufSzInBytes) = 0;

        virtual void UsbRequestOut(uint8_t RequestCode,
            uint16_t Index, uint16_t	Value,
            const uint8_t * ioBuf, uint32_t BufSzInBytes) = 0;

        virtual void GetSerialNumber(int8_t * ioBuf, uint32_t BufSzInBytes) = 0;

        virtual void GetUsbFirmwareVersion(int8_t * ioBuf, uint32_t BufSzInBytes) = 0;

        virtual std::string GetDriverVersion() = 0;

        virtual bool IsError() = 0;

        virtual uint16_t GetDeviceNum() = 0;

        virtual void UsbReqOutWithExtendedTimeout(uint8_t RequestCode,
            uint16_t Index, uint16_t	Value,
            const uint8_t * ioBuf, uint32_t BufSzInBytes) = 0;

        virtual void ReadSerialPort( uint16_t PortId, 
            uint8_t * ioBuf, uint16_t BufSzInBytes ) = 0;
}; 

#endif
