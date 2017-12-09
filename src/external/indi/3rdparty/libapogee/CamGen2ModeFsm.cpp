/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class CamGen2ModeFsm 
* \brief "finite state machine' for ascent and alta-g cameras.  use the fms lightly, because this isn't a true state machine. 
* 
*/ 

#include "CamGen2ModeFsm.h" 
#include "ApgLogger.h" 
#include "apgHelper.h" 
#include "ApnCamData.h"
#include "CameraIo.h" 
#include "CamHelpers.h" 

#include <sstream>


//////////////////////////// 
// CTOR 
CamGen2ModeFsm::CamGen2ModeFsm( std::shared_ptr<CameraIo> & io,
             std::shared_ptr<CApnCamData> & camData,
             uint16_t rev):
            ModeFsm(io, camData, rev),
            m_fileName(__FILE__)
{
}

//////////////////////////// 
// DTOR 
CamGen2ModeFsm::~CamGen2ModeFsm() 
{ 

} 

//////////////////////////// 
// IS      TDI      AVAILABLE 
bool CamGen2ModeFsm::IsTdiAvailable()
{ // TODO - Ascents don't support this
	if( IsInterlineCcd() )
	{
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Interline ccds do not support TDI mode.", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
	}

    return true;
}

//////////////////////////// 
// IS      KINETICS      AVAILABLE 
bool CamGen2ModeFsm::IsKineticsAvailable()
{ // TODO - Ascents don't support this
	if( IsInterlineCcd() )
	{
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Interline ccds do not support Kinetics mode.", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
	}

    return true;
}

//////////////////////////// 
// IS          CONTINUOUS       IMAGING        AVAILABLE
bool CamGen2ModeFsm::IsContinuousImagingAvailable()
{
    return true;
}

//////////////////////////// 
// IS      EXTERNAL        TRIGGER       AVAILABLE
bool CamGen2ModeFsm::IsExternalTriggerAvailable( const Apg::TriggerMode trigMode )
{
    //No io port on HiC camera, so cannot set the triggers
    const uint16_t HIC_ID = 511;
    return ( HIC_ID != m_CamData->m_MetaData.CameraId ? true : false );
   

    /*
    TODO: Fix MaxIm plug-in so it will not try to set tdi-kin triggers on
    camera that don't support them.  Comment this section out for
    now for backwards compatibility.
    bool result = true;

    //make sure tdi and kinetics are supported before
    //allowing the Apg::TriggerMode_TdiKinetics  trigger mode
    if( Apg::TriggerMode_TdiKinetics == trigMode )
    {
        if( !IsTdiAvailable() )
        {
            result = false;
        }
        
         if( !IsKineticsAvailable() )
        {
            result = false;
        }
    }

    return result ;
    */
}

//////////////////////////// 
//  IS     TRIGGER   NORM    EACH       ON
bool CamGen2ModeFsm::IsTriggerNormEachOn()
{

    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_IMAGE_TRIGGER_EACH_BIT ? true : false);
}
        
//////////////////////////// 
//  IS     TRIGGER   NORM    GROUP       ON
bool CamGen2ModeFsm::IsTriggerNormGroupOn()
{
    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_IMAGE_TRIGGER_GROUP_BIT ? true : false);
}
//////////////////////////// 
//  IS     TRIGGER   TDIKIN    EACH     ON
bool CamGen2ModeFsm::IsTriggerTdiKinEachOn()
{
    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_TDI_TRIGGER_EACH_BIT ? true : false);
}

//////////////////////////// 
//  IS     TRIGGER   TDIKIN    GROUP     ON
bool CamGen2ModeFsm::IsTriggerTdiKinGroupOn()
{
    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_TDI_TRIGGER_GROUP_BIT ? true : false);
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL      SHUTTER   ON
bool CamGen2ModeFsm::IsTriggerExternalShutterOn()
{
    //reading mirror reg here, because this function is called
    //with every status update and the camera doesn't appear
    //to like to be hammered with a bunch of read reg calls
    uint16_t RegVal = m_CamIo->ReadMirrorReg(
        CameraRegs::OP_A);

    return( (RegVal & CameraRegs::OP_A_SHUTTER_SOURCE_BIT) ?  true : false );
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL        READOUT     ON
bool CamGen2ModeFsm::IsTriggerExternalReadoutOn()
{
    uint16_t RegVal = m_CamIo->ReadReg(
        CameraRegs::OP_A);

    return( (RegVal & CameraRegs::OP_A_EXTERNAL_READOUT_BIT) ?  true : false );
}
