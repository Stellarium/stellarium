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

#include "CamRegMirror.h" 

#include "apgHelper.h" 
#include <string>
#include <sstream>

//////////////////////////// 
// CTOR
CamRegMirror::CamRegMirror()
{
}

//////////////////////////// 
// DTOR 
CamRegMirror::~CamRegMirror() 
{ 

} 

//////////////////////////// 
// READ
uint16_t CamRegMirror::Read( const uint16_t reg )
{
    std::map<uint16_t, uint16_t>::iterator iter = m_RegMirror.find(reg);

    if( iter == m_RegMirror.end() )
    {
        std::stringstream ss;
        ss << reg;
        std::string errStr = "Could not find register " + ss.str() + " in the mirror of the camera registers";
        apgHelper::throwRuntimeException( __FILE__, errStr, 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //found a value before we reached the end
    return (*iter).second;
    
}
   
//////////////////////////// 
// WRITE
void CamRegMirror::Write(const uint16_t reg, const uint16_t val)
{
    m_RegMirror[reg] = val;
}


//////////////////////////// 
// CLEAR 
void CamRegMirror::Clear()
{
    m_RegMirror.clear();
}
