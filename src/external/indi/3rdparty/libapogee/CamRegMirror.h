/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CamRegMirror 
* \brief This class mirrors the registers in the camera.  This allows us to "read" register that were once write only. 
* 
*/ 

#ifndef CAMREGMIRROR_INCLUDE_H__ 
#define CAMREGMIRROR_INCLUDE_H__ 

#include <map>
#include <stdint.h>

class CamRegMirror 
{ 
    public: 
        CamRegMirror();
        virtual ~CamRegMirror(); 

        uint16_t Read( uint16_t reg );
        void Write(uint16_t reg, uint16_t val);

        void Clear();

    private:
        std::map<uint16_t, uint16_t> m_RegMirror;
}; 

#endif
