/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class Aspen
* \brief Derived class for the Aspen (aka g series) series cameras
* 
*/ 

#include "Aspen.h" 
#include "apgHelper.h" 
#include "AspenIo.h"  
#include "CamHelpers.h" 
#include "ApnCamData.h"
#include "AspenData.h"
#include "CamGen2ModeFsm.h" 
#include "CamGen2CcdAcqParams.h" 
#include "ImgFix.h" 
#include "ApgLogger.h" 
#include "ApgTimer.h"
#include <sstream>

//////////////////////////// 
// CTOR 
Aspen::Aspen() : 
             CamGen2Base(CamModel::ASPEN),
             m_fileName( __FILE__ )
{

    //alloc and set the camera constants
    m_CameraConsts = std::shared_ptr<PlatformData>( new AspenData() );

} 

//////////////////////////// 
// DTOR 
Aspen::~Aspen() 
{ 

    // trying to leave the camera in a good imaging state
    // from Ticket #111 in the Alta project and ticket #87 in Zenith
    if( m_IsConnected )
    {
        try
        {
            CloseConnection();
        }
        catch( std::exception & err )
        {
            std::string msg ("Exception caught in ~Aspen msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~Aspen" );
        }
    }

} 

//////////////////////////// 
//      OPEN      CONNECTION
void Aspen::OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id )
{

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::OpenConnection -> ioType= %s, DeviceAddr = %s, FW = %d, ID = %d", 
        ioType.c_str(), DeviceAddr.c_str(), FirmwareRev, Id  );
#endif

    CreateCamIo(ioType, DeviceAddr);

     // save the input data
    m_FirmwareVersion =  FirmwareRev;
    m_Id = Id;

    //make the input id and firmware rev
    //match the camera we just connected to
    VerifyFrmwrRev();

    VerifyCamId();

    //create the ccd specific object
    CfgCamFromId( m_Id );

    // overwrite cfg matrix data with
    // information from the camera.
	// use data from registers to
	// avoid accessing string database
	// and resetting the fpga.
    UpdateCfgWithRegisterInfo();

    //set the camera mode fsm
    m_CamMode = std::shared_ptr<ModeFsm>( new CamGen2ModeFsm(m_CamIo,
        m_CamCfgData, m_FirmwareVersion) );

    //create the adc and pattern file handler object
    m_CcdAcqSettings = std::shared_ptr<CcdAcqParams>( 
        new CamGen2CcdAcqParams(m_CamCfgData,m_CamIo,m_CameraConsts) );

    m_IsConnected = true;

}

//////////////////////////// 
//      CLOSE        CONNECTION
void Aspen::CloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::DefaultCloseConnection");
#endif

    DefaultCloseConnection();
}

//////////////////////////// 
// CREATE      CAMIO     
void Aspen::CreateCamIo(const std::string & ioType,
     const std::string & DeviceAddr)
{
    //create the camera interface
    CamModel::InterfaceType type = InterfaceHelper::DetermineInterfaceType( ioType );
    m_CamIo = std::shared_ptr<CameraIo>( new AspenIo( type, DeviceAddr ) );

    if( !m_CamIo )
    {
        std::string errStr("failed to create a camera interface io object");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
            __LINE__, Apg::ErrorType_Connection );
    }

}

//////////////////////////// 
// VERIFY        CAM        ID 
void Aspen::VerifyCamId()
{
    const uint16_t id = m_CamIo->GetId();

    if( id != m_Id )
    {
        std::stringstream msg;
        msg << "Error: Expected camera id of " << m_Id << ". Read from camera id of " << id;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_Connection );
    }

}

//////////////////////////// 
// CFG      CAM       FROM  ID
void Aspen::CfgCamFromId( const uint16_t CameraId )
{
    //create and set the camera's cfg data
    DefaultCfgCamFromId( CameraId );
}


//////////////////////////// 
//      UPDATE     CFG        WITH       REGISTER       INFO
void Aspen::UpdateCfgWithRegisterInfo()
{
    const uint16_t AD1Default = m_CamIo->ReadReg( CameraRegs::AD1_DEFAULT_VALUES );
    const uint16_t AD2Default = m_CamIo->ReadReg( CameraRegs::AD2_DEFAULT_VALUES );
	if( AD1Default & CameraRegs::AD_DEFAULT_VALID_BIT )
    {
		uint16_t AD1Gain = ( AD1Default & CameraRegs::AD_DEFAULT_GAIN_BITS ) >> 
								CameraRegs::AD_DEFAULT_GAIN_SHIFT;
		uint16_t AD1Offset = ( AD1Default & CameraRegs::AD_DEFAULT_OFFSET_BITS ) >> 
								CameraRegs::AD_DEFAULT_OFFSET_SHIFT;
		
		m_CamCfgData->m_MetaData.DefaultGainLeft = AD1Gain;
		m_CamCfgData->m_MetaData.DefaultOffsetLeft = AD1Offset;

    }
	if( AD2Default & CameraRegs::AD_DEFAULT_VALID_BIT )
    {
		uint16_t AD2Gain = ( AD2Default & CameraRegs::AD_DEFAULT_GAIN_BITS ) >> 
								CameraRegs::AD_DEFAULT_GAIN_SHIFT;
		uint16_t AD2Offset = ( AD2Default & CameraRegs::AD_DEFAULT_OFFSET_BITS ) >> 
								CameraRegs::AD_DEFAULT_OFFSET_SHIFT;
        
		m_CamCfgData->m_MetaData.DefaultGainRight = AD2Gain;
		m_CamCfgData->m_MetaData.DefaultOffsetRight = AD2Offset;

    }
}

//////////////////////////// 
//      UPDATE     CFG        WITH       STR    DB       INFO
void Aspen::UpdateCfgWithStrDbInfo()
{
    CamInfo::StrDb infoStruct = std::dynamic_pointer_cast<AspenIo>(
        m_CamIo)->ReadStrDatabase();

    if( 0 != infoStruct.Ad1Gain.compare("Not Set") )
    {
        std::stringstream ss( infoStruct.Ad1Gain );
        ss >> m_CamCfgData->m_MetaData.DefaultGainLeft;
    }

    if( 0 != infoStruct.Ad1Offset.compare("Not Set") )
    {
        std::stringstream ss( infoStruct.Ad1Offset );
        ss >> m_CamCfgData->m_MetaData.DefaultOffsetLeft;
    }

    if( 0 != infoStruct.Ad2Gain.compare("Not Set") )
    {
        std::stringstream ss( infoStruct.Ad2Gain );
        ss >> m_CamCfgData->m_MetaData.DefaultGainRight;
    }

    if( 0 != infoStruct.Ad2Offset.compare("Not Set") )
    {
        std::stringstream ss( infoStruct.Ad2Offset );
        ss >> m_CamCfgData->m_MetaData.DefaultOffsetRight;
    }

}

//////////////////////////// 
//      FIX      IMG        FROM          CAMERA
void Aspen::FixImgFromCamera( const std::vector<uint16_t> & data,
                           std::vector<uint16_t> & out,  const int32_t rows, 
                           const int32_t cols )
{
     int32_t offset = 0; 

    switch( m_CamCfgData->m_MetaData.NumAdOutputs )
    {
        case 1:
            offset = m_CcdAcqSettings->GetPixelShift();
            ImgFix::SingleOuputCopy( data, out, rows, cols, offset );
        break;

        case 2:
            offset = m_CcdAcqSettings->GetPixelShift() * 2;
            ImgFix::DualOuputFix( data, out, rows, cols, offset );
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invaild number of ad ouputs = " << m_CamCfgData->m_MetaData.NumAdOutputs;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }
}

//////////////////////////// 
// GET       MAC      ADDRESS
std::string Aspen::GetMacAddress( )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::GetMacAddress" );
#endif

    return std::dynamic_pointer_cast<AspenIo>(m_CamIo)->GetMacAddress();
}
//////////////////////////// 
//      START        EXPOSURE
void Aspen::StartExposure( const double Duration, const bool IsLight )
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::StartExposure -> Duration = %d, Light -> %d", Duration, IsLight  );
#endif

      // if this is a dual read out camera make sure the columns
    // are centered
    if( 2 == m_CamCfgData->m_MetaData.NumAdOutputs )
    {
        if( !AreColsCentered() )
        {
            std::stringstream msg;
            msg << "Colmns not centered on dual readout system: ";
            msg << "; start col = " << GetRoiStartCol();
            msg << "; # roi cols = " << GetRoiNumCols();
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
    }

	m_ExposureTimer->Start();
	m_LastExposureTime = Duration;
    DefaultStartExposure( Duration, IsLight, false );
}


//////////////////////////// 
//      ARE    COLS   CENTERED
bool Aspen::AreColsCentered()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::GetNumAdChannels" );
#endif
    const int32_t CENTER = (GetMaxImgCols() / GetRoiBinCol()) / 2;

    const int32_t START_DIFF = GetRoiStartCol() - CENTER;

    if( START_DIFF >= 0 )
    {
        return false;
    }

    const uint16_t imgCols= m_CcdAcqSettings->GetRoiNumCols();

    const int32_t END_POS = GetRoiStartCol() +  imgCols;

    const int32_t END_DIFF = CENTER - END_POS;

    if( END_DIFF >= 0 )
    {
        return false;
    }

    // off by plus minus one ok because of odd column
    // rois
    if( END_DIFF < (START_DIFF-1) || 
        END_DIFF > (START_DIFF+1)  )
    {
        return false;
    }

    return true;
}

//////////////////////////// 
// EXPOSURE    AND     GET        IMG     RC
void Aspen::ExposureAndGetImgRC(uint16_t & r, uint16_t & c)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::ExposureAndGetImgRC" );
#endif

    //detemine the exposure height
    r = m_CcdAcqSettings->GetRoiNumRows();

    //detemine the exposure width
    if( 2 == m_CamCfgData->m_MetaData.NumAdOutputs )
    {
        // if this is an odd number of cols, then we are actually
        // requesting 1 less column than desired, see how
        // CamGen2CcdAcqParams::GetCcdImgCols() is used by
        // CcdAcqParams
        const uint16_t NUM_COLS = m_CcdAcqSettings->GetRoiNumCols() -  
            std::dynamic_pointer_cast<CamGen2CcdAcqParams>(m_CcdAcqSettings)->GetOddColsAdjust();

         // double the number of adc latency pixels for dual readout systems
        const uint16_t PIXEL_SHIFT = m_CcdAcqSettings->GetPixelShift()*2;
        
        c = NUM_COLS + PIXEL_SHIFT;
    }
    else
    {
        c = m_CcdAcqSettings->GetRoiNumCols() + m_CcdAcqSettings->GetPixelShift();
    }

}

//////////////////////////// 
//  GET    NUM        ADC        CHANNELS
int32_t Aspen::GetNumAdChannels() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::GetNumAdChannels" );
#endif
    return 1; 
}  


//////////////////////////// 
//      INIT
void Aspen::Init()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::Init" );
#endif

    DefaultInit();

    SetIsInterlineBit();

     //single readout by default
     SetDualReadout( false );



}

//////////////////////////// 
//      WRITE    ID    2   CAM       REG
void Aspen::WriteId2CamReg()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::WriteId2CamReg" );
#endif
    m_CamIo->WriteReg( CameraRegs::ID_FROM_PROM,
        m_Id );
}

//////////////////////////// 
//      SET     IS    INTERLINE     BIT
void Aspen::SetIsInterlineBit()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::SetIsInterlineBit" );
#endif


    if( m_CamCfgData->m_MetaData.InterlineCCD )
    {
        //set high this is an interline ccd
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_C,
            CameraRegs::OP_C_IS_INTERLINE_BIT);
    }
    else
    {
         m_CamIo->ReadAndWriteReg( CameraRegs::OP_C,
             static_cast<uint16_t>(~CameraRegs::OP_C_IS_INTERLINE_BIT) );
    }   	
}

//////////////////////////// 
//  SET      FAN       MODE
void Aspen::SetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::SetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif

   DefaultSetFanMode( mode, PreCondCheck );
}

//////////////////////////// 
//  GET      FAN       MODE
Apg::FanMode Aspen::GetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::GetFanMode" );
#endif
    return DefaultGetFanMode();
}

//////////////////////////// 
//      IS         DUAL      READOUT          SUPPORTED
bool Aspen::IsDualReadoutSupported()
{
    return ( true == m_CamCfgData->m_MetaData.SupportsSingleDualReadoutSwitching ? true : false );
}

//////////////////////////// 
//      SET    DUAL       READOUT
void Aspen::SetDualReadout( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Aspen::SetDualReadout -> TurnOn = %d", TurnOn);
#endif

    //exit if camera is already in the desired state
    if( GetDualReadout() == TurnOn )
    {
        return;
    }

    if( TurnOn )
    {
        if( !IsDualReadoutSupported() )
        {
            apgHelper::throwRuntimeException( m_fileName, 
                "Dual read out not supported on this camera", 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        
        //set R12B3 high to turn on
        m_CamIo->ReadMirrorOrWriteReg(CameraRegs::OP_D, 
            CameraRegs::OP_D_DUALREADOUT_BIT);

        m_CamCfgData->m_MetaData.NumAdOutputs = 2;
    }
    else
    {
        //set R12B3 low to turn off
        m_CamIo->ReadMirrorAndWriteReg( CameraRegs::OP_D,
            static_cast<uint16_t>(~CameraRegs::OP_D_DUALREADOUT_BIT) );

        m_CamCfgData->m_MetaData.NumAdOutputs = 1;

    }

    // load the correct pattern files for dual or single mode
    // setting
    const Apg::AdcSpeed curSpeed = m_CcdAcqSettings->GetSpeed();
    m_CcdAcqSettings->SetSpeed( curSpeed );
}

//////////////////////////// 
//      GET        DUAL       READOUT
bool Aspen::GetDualReadout()
{
    return( 2 == m_CamCfgData->m_MetaData.NumAdOutputs ? true : false );
}

