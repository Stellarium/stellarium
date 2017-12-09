/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class HiC 
* \brief Camera object for the HiC 
* 
*/ 


#ifndef HIC_INCLUDE_H__ 
#define HIC_INCLUDE_H__ 

#include "Quad.h" 
#include "CameraInfo.h" 
#include <string>

class DLL_EXPORT HiC : public Quad
{ 
    public: 
        HiC();

        virtual ~HiC(); 
       
        void SetSerialNumber(const std::string & num);
        CamInfo::StrDb GetCamInfo();
        void SetCamInfo( CamInfo::StrDb & info );

        void Get4kby4kImage( std::vector<uint16_t> & out );

    private:
        const std::string m_fileName;
        
        HiC (const HiC&);
        HiC& operator=(HiC&);
}; 

#endif
