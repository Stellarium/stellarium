/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class GeeFx2 
* \brief Class that manages io and behavoir for Alta-G series Cypress EZ-USB FX2 chips 
* 
*/ 


#ifndef GEEFX2_INCLUDE_H__ 
#define GEEFX2_INCLUDE_H__ 

#include "Fx2Base.h" 
#include <string>
#include <vector>

class GeeFx2 : public Fx2Base
{ 
    public: 
        GeeFx2(std::shared_ptr<Fx2Io> io,
            const bool Print2StdOut);
        virtual ~GeeFx2(); 

        void SetSerialNumber(const std::string & num);
        std::string GetSerialNumber();

        void ProgramCamera(const std::string & FilenameFpga,
            const std::string & FilenameFx2, const std::string & FilenameDescriptor,
            const std::string & FilenameWebPage, const std::string & FilenameWebServer,
            const std::string & FilenameWebCfg);

        void ReadHeader( Eeprom::Header & hdr );

    private:
        void WriteFlash(uint32_t StartAddr,
            const std::vector<uint8_t> & data);
        
        void ReadFlash(const uint32_t StartAddr,
            std::vector<uint8_t> & data);

        void DownloadFirmware();
        void IncrEepromAddrBlockBank(uint16_t IncrSize,
            uint16_t & Addr, uint8_t & Bank, uint8_t & Block);

        std::string m_fileName;
}; 

#endif
