/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AspenIo 
* \brief io class for alta-g, aka aspen, aka ??, cameras 
* 
*/ 


#ifndef ASPENIO_INCLUDE_H__ 
#define ASPENIO_INCLUDE_H__ 

#include "CameraIo.h" 
#include "CameraInfo.h" 
#include <string>

class DLL_EXPORT AspenIo : public CameraIo
{ 
    public: 
        AspenIo(CamModel::InterfaceType type, 
               const std::string & deviceAddr); 

        virtual ~AspenIo(); 

        void Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, const std::string & FilenameDescriptor,
            const std::string & FilenameWebPage, const std::string & FilenameWebServer,
            const std::string & FilenameWebCfg, bool Print2StdOut=false);

        uint16_t GetId();

        uint16_t GetIdFromStrDB();

        std::string GetMacAddress();

        void WriteStrDatabase( const CamInfo::StrDb & info );
        CamInfo::StrDb ReadStrDatabase();

        void WriteNetDatabase( const CamInfo::NetDb & input );
        CamInfo::NetDb ReadNetDatabase();

        std::vector<uint8_t> GetFlashBuffer( uint32_t StartAddr, uint32_t numBytes );

        private:
            std::string m_fileName;
}; 

#endif
