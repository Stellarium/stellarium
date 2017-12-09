/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AltaUsbIo 
* \brief class for managing the AltaU series IO 
* 
*/ 


#ifndef ALTAUSBIO_INCLUDE_H__ 
#define ALTAUSBIO_INCLUDE_H__ 

#include "CamUsbIo.h" 
#include "IAltaSerialPortIo.h" 

class AltaUsbIo : public CamUsbIo, public IAltaSerialPortIo
{ 
    public: 
        AltaUsbIo( const std::string & DeviceEnum );
        ~AltaUsbIo(); 

         void Program(const std::string & FilenameCamCon,
            const std::string & FilenameBufCon, const std::string & FilenameFx2,
            const std::string & FilenameGpifCamCon,const std::string & FilenameGpifBufCon,
            const std::string & FilenameGpifFifo, bool Print2StdOut=false );

        void ReadHeader( Eeprom::Header & hdr );

        void SetSerialNumber(const std::string & num);
        std::string GetSerialNumber();

        void SetSerialBaudRate( uint16_t PortId , uint32_t BaudRate );

        uint32_t GetSerialBaudRate(  uint16_t PortId  );

        Apg::SerialFC GetSerialFlowControl( uint16_t PortId );

        void SetSerialFlowControl( uint16_t PortId, 
            Apg::SerialFC FlowControl );

         Apg::SerialParity GetSerialParity( uint16_t PortId );
        
        void SetSerialParity( uint16_t PortId, Apg::SerialParity Parity );
        
        void ReadSerial( uint16_t PortId, std::string & buffer );
        
        void WriteSerial( uint16_t PortId, const std::string & buffer );

    private:
#pragma pack( push, 1 )
        struct SerialPortSettings
        {
            uint32_t BaudRate;
            uint8_t PortCtrl;
        };
#pragma pack( pop )

        AltaUsbIo::SerialPortSettings ReadSerialSettings( uint16_t PortId );

        void WriteSerialSettings( uint16_t PortId, 
            AltaUsbIo::SerialPortSettings & settings);

        void DownloadFirmware();

        void IncrEepromAddrBlockBank(uint16_t IncrSize,
            uint16_t & Addr, uint8_t & Bank, 
            uint8_t & Block);

        std::string m_fileName;
}; 

#endif
