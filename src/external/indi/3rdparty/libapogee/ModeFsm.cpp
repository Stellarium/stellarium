/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class ModeFsm 
* \brief This is class is the implementation of the finite state machine (fsm) for camera modes 
* 
*/ 

#include "ModeFsm.h" 

#include "apgHelper.h" 
#include "ApgLogger.h" 
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "ApnCamData.h"

#include <sstream>

//////////////////////////// 
// CTOR 
ModeFsm::ModeFsm( std::shared_ptr<CameraIo> & io,
                 std::shared_ptr<CApnCamData> & camData, 
                 uint16_t rev) :
                 m_fileName(__FILE__),
                 m_mode(Apg::CameraMode_Normal),
                 m_CamIo(io),
                 m_CamData(camData),
                 m_FirmwareVersion(rev),
                 m_IsBulkDownloadOn(false),
                 m_IsPipelineDownloadOn(true),
                 m_TdiRows( 1 )
{
    m_CamIo->ReadOrWriteReg( CameraRegs::CMD_A, CameraRegs::CMD_A_PIPELINE_BIT );
}

//////////////////////////// 
// DTOR 
ModeFsm::~ModeFsm() 
{ 

} 

//////////////////////////// 
// SET    MODE 
void ModeFsm::SetMode( const Apg::CameraMode newMode )
{
    if( newMode == m_mode )
    {
        //exit no need to change mode
        return;
    }

    if( !IsModeValid( newMode ) )
    {
        std::stringstream msg;
        msg << "Invalid mode detected " << newMode;
        msg << " setting camera to mode " << Apg::CameraMode_Normal;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        SetMode( Apg::CameraMode_Normal );
        return;
    }

    
    ExitOldMode();

    EnterNewMode( newMode );

    Apg::CameraMode old = m_mode;
    m_mode = newMode;

    std::stringstream msg;
    msg << "Succesfully transitioned from mode " << old;
    msg << " to mode " << m_mode;
    std::string str = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info",str); 
}

//////////////////////////// 
// IS    MODE     VALID
bool ModeFsm::IsModeValid( const Apg::CameraMode newMode )
{
    if( newMode == Apg::CameraMode_ExternalShutter )
    {
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Apg::CameraMode_ExternalShutter depericated", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    if( newMode == Apg::CameraMode_ExternalTrigger )
    {
        std::string vinfo = apgHelper::mkMsg( m_fileName, "Apg::CameraMode_ExternalTrigger depericated", __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        return false;
    }

    if( newMode  == Apg::CameraMode_Kinetics )
    {
        if( !IsKineticsAvailable() )
        {
            std::string vinfo = apgHelper::mkMsg( m_fileName, "Kinetics mode not supported", __LINE__);
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
            return false;
        }
    }
    
    if (newMode  == Apg::CameraMode_TDI) 
    {
        if( !IsTdiAvailable() )
        {
            std::string vinfo = apgHelper::mkMsg( m_fileName, "TDI mode not supported", __LINE__);
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
            return false;
        }
    }
    
    return true;
}

//////////////////////////// 
// EXIT   OLD  MODE
void  ModeFsm::ExitOldMode()
{
    if( Apg::CameraMode_Test == m_mode )
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
             static_cast<uint16_t>(~CameraRegs::OP_B_AD_SIMULATION_BIT) );
    }

    //otherwise no op
}
    
//////////////////////////// 
// ENTER   NEW      MODE
void  ModeFsm::EnterNewMode( Apg::CameraMode newMode )
{
    if( Apg::CameraMode_Test == newMode )
    {
         m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
            CameraRegs::OP_B_AD_SIMULATION_BIT);
    }

    //otherwise no op
}



//////////////////////////// 
// SET       BULK         DOWNLOAD
void ModeFsm::SetBulkDownload( bool TurnOn )
{
	/* Turn this check off for Aspen

    //checking pre-conditions
    if( false == TurnOn && CamModel::ETHERNET == m_CamIo->GetInterfaceType() )
    {
        // TODO - figure out if this is true for AltaG cameras
        std::string msg( "Cannot turn bulk sequece off for generation one ethernet interfaces.");
        apgHelper::throwRuntimeException(m_fileName, msg, 
            __LINE__, Apg::ErrorType_InvalidOperation);

    }
    */
    m_IsBulkDownloadOn = TurnOn;
}



//////////////////////////// 
// SET       PIPELINE         DOWNLOAD
void ModeFsm::SetPipelineDownload( bool TurnOn )
{
    //checking pre-conditions
    if( true == TurnOn && CamModel::ETHERNET == m_CamIo->GetInterfaceType() )
    {
        // This property does not apply to ethernet systems, 
		// because there is no way to tell if data is in the fifo.
        return;

    }
    if( TurnOn)
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::CMD_A, CameraRegs::CMD_A_PIPELINE_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg(CameraRegs::CMD_A, static_cast<uint16_t>(~CameraRegs::CMD_A_PIPELINE_BIT) );
    }
    m_IsPipelineDownloadOn = TurnOn;
}



//////////////////////////// 
// GET     TRIGS        THAT   ARE        ON
std::vector< std::pair<Apg::TriggerMode, Apg::TriggerType> > ModeFsm::GetTrigsThatAreOn()
{
    std::vector< std::pair<Apg::TriggerMode,Apg::TriggerType> > trigs;

    if( IsTriggerNormEachOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_Normal, Apg::TriggerType_Each) );
    }
    
    if( IsTriggerNormGroupOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_Normal, Apg::TriggerType_Group) );
    }

    if( IsTriggerTdiKinEachOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_TdiKinetics, Apg::TriggerType_Each) );
    }

    if( IsTriggerTdiKinGroupOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_TdiKinetics, Apg::TriggerType_Group) );
    }

    if( IsTriggerExternalShutterOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_ExternalShutter, Apg::TriggerType_Each) );
    }

    if( IsTriggerExternalReadoutOn() )
    {
         trigs.push_back( 
            std::pair<Apg::TriggerMode,Apg::TriggerType>(
            Apg::TriggerMode_ExternalReadoutIo, Apg::TriggerType_Each) );
    }

    return trigs;
}


//////////////////////////// 
// SET  EXTERNAL TRIGGER
void ModeFsm::SetExternalTrigger( const bool TurnOn, 
                                 const Apg::TriggerMode trigMode,
                                 const Apg::TriggerType trigType)
{
    if( TurnOn )
    {
        if( !IsExternalTriggerAvailable( trigMode ) )
        {
            std::stringstream msg;
            msg << "Cannot activate trigger mode " << trigMode;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
    }
    else
    {
        if( !IsExternalTriggerAvailable( trigMode ) )
        {
             std::string vinfo = apgHelper::mkMsg( m_fileName, 
                 "External trigger not available, exiting without setting", __LINE__);
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
            return;
        }
    }

    switch(trigMode)
    {
        // ATTENTION: Two conditions here Norm and Tdi/Kin
        case Apg::TriggerMode_Normal:
        case Apg::TriggerMode_TdiKinetics:
            SetNormTdiKinTriggers( TurnOn,  trigMode, trigType);
        break;

        case Apg::TriggerMode_ExternalShutter:
            SetShutterTrigger( TurnOn );
        break;

        case Apg::TriggerMode_ExternalReadoutIo:
            SetReadoutIoTrigger( TurnOn );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid trigger mode " << trigMode;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }
}

//////////////////////////// 
// SET  SHUTTER  TRIGGER
void ModeFsm::SetShutterTrigger( bool TurnOn )
{
    if( TurnOn)
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
        CameraRegs::OP_A_SHUTTER_SOURCE_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg(CameraRegs::OP_A,
        static_cast<uint16_t>(~CameraRegs::OP_A_SHUTTER_SOURCE_BIT) );
    }

}

//////////////////////////// 
// SET  SHUTTER  TRIGGER
void ModeFsm::SetReadoutIoTrigger( bool TurnOn )
{
    if( TurnOn)
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
        CameraRegs::OP_A_EXTERNAL_READOUT_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg(CameraRegs::OP_A,
        static_cast<uint16_t>(~CameraRegs::OP_A_EXTERNAL_READOUT_BIT) );
    }

}

//////////////////////////// 
// SET  NORM TDI KIN  TRIGGERS
void ModeFsm::SetNormTdiKinTriggers( const bool TurnOn, 
                                 const Apg::TriggerMode trigMode,
                                 const Apg::TriggerType trigType)
{   
    switch(trigMode)
    {
        case Apg::TriggerMode_Normal:
           SetNormTrigger( TurnOn, trigType );
        break;

        case Apg::TriggerMode_TdiKinetics:
           SetTdiKinTrigger( TurnOn, trigType );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid trigger mode " << trigMode;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage);
        }
        break;
    }

    

}

//////////////////////////// 
//      SET     NORM    TRIGGER
void ModeFsm::SetNormTrigger( const bool TurnOn, 
                                 const Apg::TriggerType trigType)
{
    const uint16_t trigBit2Touch =  GetNormTrigMask( trigType );

    //toggle the bit
    if( TurnOn )
    {
       TurnTrigOn( trigBit2Touch );
    }
    else
    {
        TurnTrigOff( 
            static_cast<uint16_t>(~trigBit2Touch)  );
    }
}

//////////////////////////// 
//      SET         TDI      KIN      TRIGGER
void ModeFsm::SetTdiKinTrigger( const bool TurnOn, 
                                 const Apg::TriggerType trigType)
{
    //checking pre-conditions
    switch(trigType)
    {
        case Apg::TriggerType_Each:
            if( IsTriggerTdiKinGroupOn() && TurnOn)
            {
                apgHelper::throwRuntimeException(m_fileName, 
                    "Error cannot set tdi-kinetics each trigger: group trigger on", 
                    __LINE__, Apg::ErrorType_InvalidUsage );
            }

            if( TurnOn )
            {
                 m_CamIo->WriteReg( CameraRegs::TDI_ROWS, 0 );
            }
            else
            {
                m_CamIo->WriteReg( CameraRegs::TDI_ROWS, m_TdiRows );
            }

        break;

        case Apg::TriggerType_Group:
             if( IsTriggerTdiKinEachOn() && TurnOn )
            {
                apgHelper::throwRuntimeException(m_fileName, 
                    "Error cannot set tdi-kinetics group trigger: each trigger on", 
                    __LINE__, Apg::ErrorType_InvalidUsage);
            }
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid trigger type " << trigType;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage);
        }
        break;
    }

    const uint16_t trigBit2Touch = GetTdiKinTrigMask( trigType );

     //toggle the bit
    if( TurnOn )
    {
       TurnTrigOn( trigBit2Touch );
    }
    else
    {
        TurnTrigOff( 
            static_cast<uint16_t>(~trigBit2Touch)  );
    }
}

//////////////////////////// 
// GET       NORM    TRIG        MASK
uint16_t ModeFsm::GetNormTrigMask( const Apg::TriggerType trigType )
{
    //detemerine which bit we need to touch
    uint16_t  trigBit2Touch = 0;

    switch(trigType)
    {
        case Apg::TriggerType_Each:
             trigBit2Touch = CameraRegs::OP_C_IMAGE_TRIGGER_EACH_BIT;
        break;

        case Apg::TriggerType_Group:
             trigBit2Touch = CameraRegs::OP_C_IMAGE_TRIGGER_GROUP_BIT;
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid trigger type " << trigType;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    return  trigBit2Touch;
}

//////////////////////////// 
// GET       TDI       KIN    TRIG        MASK
uint16_t ModeFsm::GetTdiKinTrigMask( const Apg::TriggerType trigType)
{
     //detemerine which bit we need to touch
    uint16_t  trigBit2Touch = 0;

    switch(trigType)
    {
        case Apg::TriggerType_Each:
            trigBit2Touch = CameraRegs::OP_C_TDI_TRIGGER_EACH_BIT;
        break;

        case Apg::TriggerType_Group:
             trigBit2Touch = CameraRegs::OP_C_TDI_TRIGGER_GROUP_BIT;
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid trigger type " << trigType;
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage);
        }
        break;
    }

    return trigBit2Touch;
}


//////////////////////////// 
// TURN     TRIG     ON
void ModeFsm::TurnTrigOn(const uint16_t mask)
{
    EnableIoPortBit();
    m_CamIo->ReadOrWriteReg( CameraRegs::OP_C, 
        mask );
}
 
//////////////////////////// 
// TURN     TRIG     OFF
void ModeFsm::TurnTrigOff(const uint16_t mask)
{
    m_CamIo->ReadAndWriteReg( CameraRegs::OP_C,
        mask );
}

//////////////////////////// 
//  ENABLE     IO        PORT        BIT
void ModeFsm::EnableIoPortBit()
{
    uint16_t value = m_CamIo->ReadMirrorReg( 
        CameraRegs::IO_PORT_ASSIGNMENT );

    if( !(value & CameraRegs::IO_PORT_ASSIGNMENT_TRIG_IN_BIT) )
    {
        //the bit is off, so turn it on
        m_CamIo->ReadOrWriteReg( CameraRegs::IO_PORT_ASSIGNMENT,
            CameraRegs::IO_PORT_ASSIGNMENT_TRIG_IN_BIT );
    }

}

//////////////////////////// 
//  DISABLE     IO        PORT        BIT
void ModeFsm::DisableIoPortBit()
{
    //disable io bit if all triggers are off
    if( !IsTriggerNormEachOn() && !IsTriggerNormGroupOn() &&
        !IsTriggerTdiKinEachOn() && !IsTriggerTdiKinGroupOn() )
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::IO_PORT_ASSIGNMENT,
            static_cast<uint16_t>(~CameraRegs::IO_PORT_ASSIGNMENT_TRIG_IN_BIT) );
    }
}


//////////////////////////// 
// SET    FAST      SEQUENCE
void ModeFsm::SetFastSequence( const bool TurnOn )
{
    if( TurnOn )
    {
        //precondition correct sensor type and trigger mode
        if( !IsInterlineCcd() )
        {
            apgHelper::throwRuntimeException(m_fileName, 
                "Cannot turn on fast sequences camera doesn't have a interline ccd.", 
                __LINE__, Apg::ErrorType_InvalidOperation );
        }

        //checking if we are in a good mode to turn on     
        if( IsTriggerNormEachOn() )
        {
              apgHelper::throwRuntimeException(m_fileName, 
                "Cannot turn on fast sequences TriggerNormalEach on", __LINE__,
                Apg::ErrorType_InvalidMode );
        }

        m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
            CameraRegs::OP_A_RATIO_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_RATIO_BIT) );
    }
}

//////////////////////////// 
// IS      FAST     SEQUENCE     ON
bool ModeFsm::IsFastSequenceOn()
{
    return( (m_CamIo->ReadReg(CameraRegs::OP_A) 
        & CameraRegs::OP_A_RATIO_BIT) ? true : false );
}

//////////////////////////// 
// IS      INTERLINE    CCD
bool ModeFsm::IsInterlineCcd()
{
    return m_CamData->m_MetaData.InterlineCCD;
}

//////////////////////////// 
// UPDATE      APNCAM        DATA
void  ModeFsm::UpdateApnCamData( std::shared_ptr<CApnCamData> & newCamData )
{
    m_CamData = newCamData;
}


//////////////////////////// 
//      SET         TDI      ROWS 
void ModeFsm::SetTdiRows( const uint16_t rows )
{
    m_TdiRows = rows;

    if( !IsTriggerTdiKinEachOn() )
    {
        m_CamIo->WriteReg( CameraRegs::TDI_ROWS,
            rows );
    }
}

