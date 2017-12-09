/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AspenUsbIo 
* \brief usb io class for alta-g, aka aspen, aka ??, cameras  
* 
*/ 


#ifndef ASPENUSBIO_INCLUDE_H__ 
#define ASPENUSBIO_INCLUDE_H__ 

#include "CamUsbIo.h" 
#include "CameraInfo.h" 

class AspenUsbIo : public CamUsbIo
{ 
    public: 
        AspenUsbIo( const std::string & DeviceEnum );
        virtual ~AspenUsbIo(); 

         void Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, const std::string & FilenameDescriptor,
            const std::string & FilenameWebPage, const std::string & FilenameWebServer,
            const std::string & FilenameWebCfg, bool Print2StdOut=false);

         void Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, const std::string & FilenameDescriptor,
            const std::string & FilenameWebPage, const std::string & FilenameWebServer,
            const std::string & FilenameWebCfg, const std::vector<uint8_t> & StrDb,
            bool Print2StdOut=false);

        void ReadHeader( Eeprom::Header & hdr );

        void SetSerialNumber(const std::string & num);
        std::string GetSerialNumber();

        void WriteStrDatabase( const std::vector<std::string> & info );
        std::vector<std::string> ReadStrDatabase();

        void WriteNetDatabase( const CamInfo::NetDb & input );
        CamInfo::NetDb ReadNetDatabase();

        std::vector<uint8_t> GetFlashBuffer( uint32_t StartAddr, uint32_t numBytes );

        private:
            void EraseEntireFlash();
            void EraseStrDb();
            void EraseNetDb();

            void EnableFlashProgramMode();
            void DisableFlashProgramMode();

            void WriteFlash(uint32_t StartAddr,
                const std::vector<uint8_t> & data);

            void ReadFlash(const uint32_t StartAddr,
                std::vector<uint8_t> & data);

            void DownloadFirmware();

            void IncrEepromAddrBlockBank(uint16_t IncrSize,
                uint16_t & Addr, uint8_t & Bank, 
                uint8_t & Block);
            
            std::string m_fileName;

}; 

#endif
