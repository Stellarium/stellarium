/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class AltaF 
* \brief class for the F serise cameras 
* 
*/ 

#include "AltaF.h" 
#include "AscentData.h"
#include "AscentBasedIo.h" 
#include "apgHelper.h" 
#include "CamHelpers.h" 
#include "CamGen2ModeFsm.h" 
#include "CamGen2CcdAcqParams.h" 
#include "ApnCamData.h"
#include "ImgFix.h" 
#include "ApgLogger.h" 
#include <sstream>

//////////////////////////// 
// CTOR 
AltaF::AltaF() :  CamGen2Base(CamModel::ALTAF),
                m_fileName( __FILE__ )
{
     //alloc and set the camera constants
    m_CameraConsts = std::shared_ptr<PlatformData>( new AscentData() );
}

//////////////////////////// 
// DTOR 
AltaF::~AltaF() 
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
            std::string msg ("Exception caught in ~AltaF msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~AltaF" );
        }
    }
} 

//////////////////////////// 
// OPEN       CONNECTION
void AltaF::OpenConnection( const std::string & ioType,
                const std::string & DeviceAddr,
                const uint16_t FirmwareRev,
                const uint16_t Id)

{
    //create the camera interface
    CreateCamIo( ioType, DeviceAddr );

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
    // information from the camera
    UpdateCfgWithStrDbInfo();

    //set the camera mode fsm
    m_CamMode = std::shared_ptr<ModeFsm>( new CamGen2ModeFsm(m_CamIo,
        m_CamCfgData, m_FirmwareVersion) );

    //create the adc and pattern file handler object
    m_CcdAcqSettings = std::shared_ptr<CcdAcqParams>( 
        new CamGen2CcdAcqParams(m_CamCfgData,m_CamIo,m_CameraConsts) );

    m_IsConnected = true;
    LogConnectAndDisconnect( true );
} 

//////////////////////////// 
//      CLOSE     CONNECTION
void AltaF::CloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::DefaultCloseConnection");
#endif

    DefaultCloseConnection();
}

//////////////////////////// 
// CREATE      CAMIO        AND        CONSTS 
void AltaF::CreateCamIo(const std::string & ioType,
     const std::string & DeviceAddr)
{
        
    CamModel::InterfaceType type = InterfaceHelper::DetermineInterfaceType( ioType );

    m_CamIo = std::shared_ptr<CameraIo>( new AscentBasedIo( type,DeviceAddr ) );


    if( !m_CamIo )
    {
        std::string errStr("failed to create a camera interface io object");
        apgHelper::throwRuntimeException( m_fileName, errStr, __LINE__,
            Apg::ErrorType_Connection );
    }

}

//////////////////////////// 
// CFG      CAM       FROM  ID
void AltaF::CfgCamFromId( const uint16_t CameraId )
{
     //create and set the camera's cfg data
    DefaultCfgCamFromId( CameraId );
}

//////////////////////////// 
//      UPDATE     CFG        WITH       STR    DB       INFO
void AltaF::UpdateCfgWithStrDbInfo()
{
    CamInfo::StrDb infoStruct = std::dynamic_pointer_cast<AscentBasedIo>(
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
// VERIFY        CAM        ID 
void AltaF::VerifyCamId()
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
//      FIX      IMG        FROM          CAMERA
void AltaF::FixImgFromCamera( const std::vector<uint16_t> & data,
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
//      START        EXPOSURE
void AltaF::StartExposure( const double Duration, const bool IsLight )
{
    bool IssueReset = false;

    // alta f must have greater than v108 for this feature
    if( m_FirmwareVersion <= CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
        IssueReset = true;
    }
 
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

    DefaultStartExposure( Duration, IsLight, IssueReset );
    
}

//////////////////////////// 
//      ARE    COLS   CENTERED
bool AltaF::AreColsCentered()
{
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
void AltaF::ExposureAndGetImgRC(uint16_t & r, uint16_t & c)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF:::ExposureAndGetImgRC" );
#endif

    //detemine the exposure height
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() )
    {
        r =  1;
    }
    else
    {
        r = m_CcdAcqSettings->GetRoiNumRows();
    }

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
int32_t AltaF::GetNumAdChannels() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::GetNumAdChannels" );
#endif
    return 2; 
}  

//////////////////////////// 
//      WRITE    ID    2   CAM       REG
void AltaF::WriteId2CamReg()
{
    // alta f must have 109 or greater for this feature
    if( m_FirmwareVersion <= CamconFrmwr::ASC_BASED_BASIC_FEATURES )
     {
         return;
     }

 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::WriteId2CamReg" );
#endif
    m_CamIo->WriteReg( CameraRegs::ID_FROM_PROM,
        m_Id );
}

//////////////////////////// 
//      SET     IS    INTERLINE     BIT
void AltaF::SetIsInterlineBit()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::SetIsInterlineBit" );
#endif

    if( m_FirmwareVersion >  CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
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
    	
}


//////////////////////////// 
//      INIT
void AltaF::Init()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::Init" );
#endif

    DefaultInit();

    // once we have initalized the camera 
    // put the input id against the StrDb
    // save the value to a register in the camera so that
    // the firmware can see it
    WriteId2CamReg();

    SetIsInterlineBit();
}

//////////////////////////// 
//  SET      FAN       MODE
void AltaF::SetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::SetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif

   DefaultSetFanMode( mode, PreCondCheck );
}

//////////////////////////// 
//  GET      FAN       MODE
Apg::FanMode AltaF::GetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "AltaF::GetFanMode" );
#endif
    return DefaultGetFanMode();
}

