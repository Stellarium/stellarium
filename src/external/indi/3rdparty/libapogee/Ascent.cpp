/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class Ascent 
* \brief Implementation of the acent camera 
* 
*/ 

#include "Ascent.h" 
#include "AscentData.h"
#include "AscentBasedIo.h" 
#include "apgHelper.h" 
#include "CamHelpers.h" 
#include "CamGen2ModeFsm.h" 
#include "CamGen2CcdAcqParams.h" 
#include "ApnCamData.h"
#include "ImgFix.h"
#include "ApgTimer.h"
#include "ApgLogger.h" 
#include <sstream>

namespace
{
    std::map<Ascent::FilterWheelType, Ascent::FilterWheelInfo> GetInfoMap()
    {
        std::map<Ascent::FilterWheelType, Ascent::FilterWheelInfo>  result;
        Ascent::FilterWheelInfo i = { Ascent::CFW25_6R, "CFW25 6R", 6 };
        result[ Ascent::CFW25_6R ] = i;

        Ascent::FilterWheelInfo j = { Ascent::CFW31_8R, "CFW31 8R", 8 };
        result[ Ascent::CFW31_8R ] = j;

        Ascent::FilterWheelInfo k = { Ascent::FW_UNKNOWN_TYPE , "Unknown", 0 };
        result[ Ascent::FW_UNKNOWN_TYPE ] = k;

        return result;
    }

    Ascent::FilterWheelInfo GetFilterWheelInfo( const Ascent::FilterWheelType type )
    {
        std::map<Ascent::FilterWheelType, Ascent::FilterWheelInfo> fwMap = GetInfoMap();

         std::map<Ascent::FilterWheelType, Ascent::FilterWheelInfo>::iterator iter =
            fwMap.find( type);

        if( iter != fwMap.end() )
        {
            return (*iter).second;
        }

        return fwMap[ Ascent::FW_UNKNOWN_TYPE ];
    }

    double GetFwMoveTime( const uint16_t numMoves )
    {
        double result = 3.6;

        switch( numMoves )
        {
            case 0:
                result = 0.0;
            break;

            case 1:
                result = 0.9;
            break;

            case 2:
                result = 1.4;
            break;

            case 3:
                result = 1.7;
            break;

            case 4:
                result = 2.3;
            break;

            case 5:
                result = 2.7;
            break;

            case 6:
                result = 3.3;
            break;

            case 7:
                result = 3.6;
            break;

            default:
                result = 3.6;
            break;
        }

        return result;
    }
}



//////////////////////////// 
// CTOR 
Ascent::Ascent() : 
                CamGen2Base(CamModel::ASCENT),
                m_fileName( __FILE__ ),
                m_filterWheelType( Ascent::FW_UNKNOWN_TYPE ),
                m_FwDiffTime( 0.0 ),
				m_FwTimer( new ApgTimer )
{ 

    // ticket 54
    //init the timer
    m_FwTimer->Start();
    
    //alloc and set the camera constants
    m_CameraConsts = std::shared_ptr<PlatformData>( new AscentData() );

}

//////////////////////////// 
// DTOR 
Ascent::~Ascent()
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
            std::string msg ("Exception caught in ~Ascent msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~Ascent" );
        }
    }
} 

//////////////////////////// 
// OPEN     CONNECTION
void Ascent::OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id )
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::OpenConnection -> ioType= %s, DeviceAddr = %s, FW = %d, ID = %d", 
        ioType.c_str(), DeviceAddr.c_str(), FirmwareRev,Id  );
#endif

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
void Ascent::CloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::DefaultCloseConnection");
#endif

    DefaultCloseConnection();
}

//////////////////////////// 
//      FILTER       WHEEL      OPEN
 void Ascent::FilterWheelOpen( Ascent::FilterWheelType type )
 {
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::FilterWheelOpen -> type = %d", type );
#endif

    if ( Ascent::FW_UNKNOWN_TYPE == type )
    {
        apgHelper::throwRuntimeException( m_fileName, 
            "FilterWheelOpen failed.  Invalid input type.", 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    m_filterWheelType = type;
 }
	
//////////////////////////// 
//      FILTER       WHEEL      CLOSE
 void Ascent::FilterWheelClose()
 {
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "Ascent::FilterWheelClose" );
 #endif

     m_filterWheelType = Ascent::FW_UNKNOWN_TYPE;
 }


//////////////////////////// 
//  SET     FILTER       WHEEL       POS
void Ascent::SetFilterWheelPos( const uint16_t pos )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::SetFilterWheelPos -> pos = %d", pos );
#endif

    if ( Ascent::FW_UNKNOWN_TYPE == m_filterWheelType )
    {
        apgHelper::throwRuntimeException( m_fileName, "SetFilterWheelPos failed.  No filter wheel connected", 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    // if the firmware does not have the fw status bit, use the guestiamate timer
    if( m_FirmwareVersion <= CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
        StartFwTimer( pos );
    }

    const uint16_t FilterMask = CameraRegs::OP_C_FILTER_1_BIT |
            CameraRegs::OP_C_FILTER_2_BIT |
            CameraRegs::OP_C_FILTER_3_BIT;

    const uint16_t value = ( pos << 8 ) & FilterMask ;

    const uint16_t curReg = ReadReg( CameraRegs::OP_C );

    const uint16_t curRegFilterCleared = curReg & ~FilterMask;

    const uint16_t value2write = value | curRegFilterCleared;
    
    m_CamIo->WriteReg(CameraRegs::OP_C, value2write);

}


//////////////////////////// 
//      START      FW           TIMER
void Ascent::StartFwTimer( const uint16_t pos )
{
    // ticket 54
    //cacluate the estimated time to go from one position to 
    //another
    const uint16_t maxPos = GetFilterWheelMaxPositions();

    uint16_t posWalk = GetFilterWheelPos();
    uint16_t numMoves = 0;

    while( posWalk != pos )
    {
        ++numMoves;
        ++posWalk;
        if( posWalk >= maxPos )
        {
            posWalk = 0;
        }
    }
   
    m_FwDiffTime = GetFwMoveTime( numMoves );
    m_FwTimer->Start();
}

//////////////////////////// 
// GET     FILTER       WHEEL       POS
uint16_t Ascent::GetFilterWheelPos()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetFilterWheelPos" );
#endif

    if ( Ascent::FW_UNKNOWN_TYPE == m_filterWheelType )
    {
        apgHelper::throwRuntimeException( m_fileName, "GetFilterWheelPos failed.  No filter wheel connected", 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    const uint16_t value = ReadReg( CameraRegs::OP_C );

    const uint16_t FilterMask = CameraRegs::OP_C_FILTER_1_BIT |
            CameraRegs::OP_C_FILTER_2_BIT |
            CameraRegs::OP_C_FILTER_3_BIT;

    return (value & FilterMask ) >> 8;
}

//////////////////////////// 
// GET     FILTER       WHEEL    STATUS
ApogeeFilterWheel::Status Ascent::GetFilterWheelStatus()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetFilterWheelStatus" );
#endif

    if ( Ascent::FW_UNKNOWN_TYPE == m_filterWheelType )
    {
        return ApogeeFilterWheel::NOT_CONNECTED;
    }

    // if the firmware does not have the fw status bit, use the guestiamate timer
    if( m_FirmwareVersion <= CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
        return FwStatusFromTimer();
    }

    return FwStatusFromCamera();
}

//////////////////////////// 
//      FW       STATUS        FROM       TIMER
ApogeeFilterWheel::Status Ascent::FwStatusFromTimer()
{
    //ticket 54
    m_FwTimer->Stop();

    if( m_FwTimer->GetTimeInSec() > m_FwDiffTime )
    {
        m_FwDiffTime = 0.0;
        return ApogeeFilterWheel::READY;
    }
    else
    {
        return ApogeeFilterWheel::ACTIVE;
    }
}

//////////////////////////// 
//      FW       STATUS    FROM   CAMERA
ApogeeFilterWheel::Status Ascent::FwStatusFromCamera()
{
    const uint16_t RegVal = ReadReg( CameraRegs::STATUS );

    const uint16_t fwStatus = RegVal & CameraRegs::STATUS_FILTER_SENSE_BIT;

    return (  fwStatus ?  ApogeeFilterWheel::ACTIVE :  ApogeeFilterWheel::READY );
}

//////////////////////////// 
// GET     FILTER       WHEEL        NAME
std::string Ascent::GetFilterWheelName()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetFilterWheelName" );
#endif

    Ascent::FilterWheelInfo info = GetFilterWheelInfo(  m_filterWheelType );
    return info.name;
}

//////////////////////////// 
// GET     FILTER       WHEEL        MAX       POSITIONS
uint16_t Ascent::GetFilterWheelMaxPositions()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetFilterWheelMaxPositions" );
#endif

    Ascent::FilterWheelInfo info = GetFilterWheelInfo(  m_filterWheelType );
    return info.maxPositions;
}

//////////////////////////// 
// CREATE      CAMIO       
void Ascent::CreateCamIo(const std::string & ioType,
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
void Ascent::CfgCamFromId( const uint16_t CameraId )
{
     //create and set the camera's cfg data
    DefaultCfgCamFromId( CameraId );
}

//////////////////////////// 
//      UPDATE     CFG        WITH       STR    DB       INFO
void Ascent::UpdateCfgWithStrDbInfo()
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
void Ascent::VerifyCamId()
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
void Ascent::FixImgFromCamera( const std::vector<uint16_t> & data,
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
void Ascent::StartExposure( const double Duration, const bool IsLight )
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::StartExposure -> Duration = %d, Light -> %d", Duration, IsLight  );
#endif

    bool IssueReset = false;

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
bool Ascent::AreColsCentered()
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
void Ascent::ExposureAndGetImgRC(uint16_t & r, uint16_t & c)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::ExposureAndGetImgRC" );
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
int32_t Ascent::GetNumAdChannels() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetNumAdChannels" );
#endif
    return 1; 
}  



//////////////////////////// 
//      UPDATE     CAM        REG        IF    NEEDED
void Ascent::UpdateCamRegIfNeeded()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::UpdateCamRegIfNeeded" );
#endif

    if( m_FirmwareVersion >  CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
        CamInfo::StrDb db = std::dynamic_pointer_cast<AscentBasedIo>(
            m_CamIo)->ReadStrDatabase();

        if( 0 != db.Id.compare("Not Set") )
        {
            uint16_t id = 0;
            std::stringstream ss( db.Id );
            ss >> id;

            m_CamIo->WriteReg( CameraRegs::ID_FROM_PROM,
                m_Id );
        }
    }
}

//////////////////////////// 
//      SET     IS    INTERLINE     BIT
void Ascent::SetIsInterlineBit()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::SetIsInterlineBit" );
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
//      SET     IS    ASCENT     BIT
void Ascent::SetIsAscentBit()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::SetIsAscentBit" );
#endif
    if( m_FirmwareVersion > CamconFrmwr::ASC_BASED_BASIC_FEATURES )
    {
        //set high this is an ascent camera
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_C,
            CameraRegs::OP_C_IS_ASCENT_BIT );
    }
}

//////////////////////////// 
//      INIT
void Ascent::Init()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::Init" );
#endif
    DefaultInit();
    SetIsInterlineBit();
    SetIsAscentBit();

    //single readout by default
     SetDualReadout( false );

    // for ascents without dip switch
    // write the id cam into the camera
    UpdateCamRegIfNeeded();
}

//////////////////////////// 
//      IS         DUAL      READOUT          SUPPORTED
bool Ascent::IsDualReadoutSupported()
{
    if( m_FirmwareVersion > CamconFrmwr::ASC_BASED_BASIC_FEATURES &&
        m_CamCfgData->m_MetaData.SupportsSingleDualReadoutSwitching )
    {
        return true;
    }

    return false;
}

//////////////////////////// 
//      SET    DUAL       READOUT
void Ascent::SetDualReadout( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::SetDualReadout -> TurnOn = %d", TurnOn);
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
bool Ascent::GetDualReadout()
{
    return( 2 == m_CamCfgData->m_MetaData.NumAdOutputs ? true : false );
}

//////////////////////////// 
//  SET      FAN       MODE
void Ascent::SetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::SetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif
    // no op on purpose, no fan control for ascent based cameras
    // just ignore request...

}

//////////////////////////// 
//  GET      FAN       MODE
Apg::FanMode Ascent::GetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Ascent::GetFanMode" );
#endif

    // no fan control on ascent based cameras
    return Apg::FanMode_Off;
}
