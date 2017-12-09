/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AscentBasedUsbIo 
* \brief usb io class for ascent, alta f and other ascent based cameras  
* 
*/ 


#ifndef ASCENTBASEDUSBIO_INCLUDE_H__ 
#define ASCENTBASEDUSBIO_INCLUDE_H__ 

#include "CamUsbIo.h" 
#include <string>
#include <vector>

class AscentBasedUsbIo : public CamUsbIo
{ 
    public: 
        AscentBasedUsbIo( const std::string & DeviceEnum );
        virtual ~AscentBasedUsbIo(); 

        void Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, 
            const std::string & FilenameDescriptor,
            bool Print2StdOut=false);

        void ReadHeader( Eeprom::Header & hdr );

        void SetSerialNumber(const std::string & num);
        std::string GetSerialNumber();

        void WriteStrDatabase( const std::vector<std::string> & info );
        std::vector<std::string> ReadStrDatabase();

    private:
        void DownloadFirmware();
     
        std::string m_fileName;

}; 

#endif
