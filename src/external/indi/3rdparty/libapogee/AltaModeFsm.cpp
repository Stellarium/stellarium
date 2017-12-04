/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class AltaModeFsm 
* \brief "finite state machine' for alta cameras.  use the fms lightly, because this isn't a true state machine. 
* 
*/ 

#include "AltaModeFsm.h" 
#include "ApgLogger.h" 
#include "apgHelper.h" 
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include <sstream>

namespace
{
    const uint16_t MIN_KINETICS_CI_REV = 17;
    const uint16_t MIN_TRIGGER_REV = 27;
}

//////////////////////////// 
// CTOR 
AltaModeFsm::AltaModeFsm( std::shared_ptr<CameraIo> & io,
             std::shared_ptr<CApnCamData> & camData,
             uint16_t rev):
            ModeFsm(io, camData, rev),
            m_fileName(__FILE__)
{
}

//////////////////////////// 
// DTOR 
AltaModeFsm::~AltaModeFsm()
{
}


//////////////////////////// 
// IS      TDI      AVAILABLE 
bool AltaModeFsm::IsTdiAvailable()
{
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
bool AltaModeFsm::IsKineticsAvailable()
{
    if( MIN_KINETICS_CI_REV > m_FirmwareVersion )
    {
        std::stringstream msg;
        msg << "Firmware version " << m_FirmwareVersion << " does not support kinetics mode";
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    if( IsInterlineCcd() )
    {
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Interline ccds do not support kinetics mode.", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    return true;
}

//////////////////////////// 
// IS          CONTINUOUS       IMAGING        AVAILABLE
bool AltaModeFsm::IsContinuousImagingAvailable()
{
       
    if( MIN_KINETICS_CI_REV >  m_FirmwareVersion)
    {
        std::stringstream msg;
        msg << "Firmware version " << m_FirmwareVersion << " does not support continuous imaging";
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    if( CamModel::ETHERNET == m_CamIo->GetInterfaceType() )
    {
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Alta Ethernet cameras do not support continuous imaging", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    return true;
}

//////////////////////////// 
// IS      EXTERNAL        TRIGGER       AVAILABLE
bool AltaModeFsm::IsExternalTriggerAvailable( const Apg::TriggerMode trigMode )
{


    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        std::stringstream msg;
        msg << "Firmware version " << m_FirmwareVersion;
        msg << " does not support trigger mode " << trigMode << ".";
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }
    
    if( Apg::TriggerMode_Normal == trigMode ||
        Apg::TriggerMode_TdiKinetics == trigMode )
    {
        if( MIN_KINETICS_CI_REV >  m_FirmwareVersion )
        {
            std::stringstream msg;
            msg << "Firmware version " << m_FirmwareVersion;
            msg << " does not support trigger mode " << trigMode << ".";
            std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
            return false;
        }
    }

    return true;

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

    return result;
    */
}

//////////////////////////// 
//  IS     TRIGGER   NORM    EACH       ON
bool AltaModeFsm::IsTriggerNormEachOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_IMAGE_TRIGGER_EACH_BIT ? true : false);
}
        
//////////////////////////// 
//  IS     TRIGGER   NORM    GROUP       ON
bool AltaModeFsm::IsTriggerNormGroupOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_IMAGE_TRIGGER_GROUP_BIT ? true : false);
}
//////////////////////////// 
//  IS     TRIGGER   TDIKIN    EACH     ON
bool AltaModeFsm::IsTriggerTdiKinEachOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_TDI_TRIGGER_EACH_BIT ? true : false);
}

//////////////////////////// 
//  IS     TRIGGER   TDIKIN    GROUP     ON
bool AltaModeFsm::IsTriggerTdiKinGroupOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    const uint16_t value = m_CamIo->ReadReg( CameraRegs::OP_C );
    return (value & CameraRegs::OP_C_TDI_TRIGGER_GROUP_BIT ? true : false);
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL      SHUTTER   ON
bool AltaModeFsm::IsTriggerExternalShutterOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    //reading mirror reg here, because this function is called
    //with every status update and the camera doesn't appear
    //to like to be hammered with a bunch of read reg calls
    uint16_t RegVal = m_CamIo->ReadMirrorReg(
        CameraRegs::OP_A);

    return( (RegVal & CameraRegs::OP_A_SHUTTER_SOURCE_BIT) ?  true : false );
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL        READOUT     ON
bool AltaModeFsm::IsTriggerExternalReadoutOn()
{
    if( MIN_TRIGGER_REV >  m_FirmwareVersion )
    {
        return false;
    }

    uint16_t RegVal = m_CamIo->ReadReg(
        CameraRegs::OP_A);

    return( (RegVal & CameraRegs::OP_A_EXTERNAL_READOUT_BIT) ?  true : false );
}
