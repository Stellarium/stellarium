/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2012 Apogee Imaging Systems, Inc. 
* \class AscentBasedIo 
* \brief io class for ascent, alta f and other ascent based cameras 
* 
*/ 


#ifndef ASCENTBASEDIO_INCLUDE_H__ 
#define ASCENTBASEDIO_INCLUDE_H__ 

#include "CameraIo.h" 
#include "CameraInfo.h" 
#include <string>

class DLL_EXPORT AscentBasedIo : public CameraIo
{ 
    public: 
        AscentBasedIo( CamModel::InterfaceType type, 
               const std::string & deviceAddr ); 
        virtual ~AscentBasedIo(); 

        void Program(const std::string & FilenameFpga,
            const std::string & FilenameFx2, 
            const std::string & FilenameDescriptor,
            bool Print2StdOut=false);

        void WriteStrDatabase( const CamInfo::StrDb & info );
        CamInfo::StrDb ReadStrDatabase();

        uint16_t GetId();

        private:
            std::string m_fileName;
}; 

#endif
