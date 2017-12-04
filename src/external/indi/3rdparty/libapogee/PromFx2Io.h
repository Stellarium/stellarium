/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class PromFx2Io 
* \brief helper class for downloading the fx2 romloader and device firmware into the proms 
* 
*/ 


#ifndef PROMFX2IO_INCLUDE_H__ 
#define PROMFX2IO_INCLUDE_H__ 

#include "CamHelpers.h" 

#include <string>
#include <vector>

#ifdef WIN_OS
#include <memory>
#else
#include <tr1/memory>
#endif

#include <stdint.h>

class IUsb;

class PromFx2Io 
{ 
    public: 
        PromFx2Io( std::shared_ptr<IUsb> & usb,
            uint32_t MaxBlocks, uint32_t MaxBanks );
        virtual ~PromFx2Io(); 

        void FirmwareDownload(const std::vector<UsbFrmwr::IntelHexRec> & Records);
 
        void WriteFile2Eeprom(const std::string & filename, uint8_t StartBank, 
            uint8_t StartBlock, uint16_t StartAddr, uint32_t & NumBytesWritten);
 
        void BufferWriteEeprom(uint8_t StartBank, uint8_t StartBlock,
            uint16_t StartAddr, const std::vector<uint8_t> & Buffer );

        void BufferReadEeprom(uint8_t StartBank, uint8_t StartBlock,
            uint16_t StartAddr, std::vector<uint8_t> & Buffer );
       
         void WriteEepromHdr( const Eeprom::Header & hdr,
            uint8_t StartBank, uint8_t StartBlock,
            uint16_t StartAddr);

        void ReadEepromHdr( Eeprom::Header & hdr,
            uint8_t StartBank, uint8_t StartBlock,
            uint16_t StartAddr);

        std::vector<uint8_t> ReadFirmwareFile( const std::string & filename );

    private:
        
        void IncrEepromAddrBlockBank(uint16_t IncrSize,
            uint16_t & Addr, uint8_t & Bank, uint8_t & Block);

        void WriteEeprom(uint16_t Addr,
            uint8_t Bank, uint8_t Block,
            const uint8_t * data, uint32_t DataSzInBytes);

        void ReadEeprom(uint16_t Addr,
            uint8_t Bank, uint8_t Block,
            uint8_t * data, uint32_t DataSzInBytes);

        std::shared_ptr<IUsb> m_Usb; 
        uint32_t m_MaxBlocks;
        uint32_t m_MaxBanks;
        std::string m_fileName;
}; 

#endif
