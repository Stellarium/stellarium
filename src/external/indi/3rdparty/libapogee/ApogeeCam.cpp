/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class ApogeeCam 
* \brief Base class for apogee cameras 
* 
*/ 

#include "ApogeeCam.h" 
 
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "PlatformData.h" 
#include "apgHelper.h" 
#include "ApgLogger.h" 
#include "ApnCamData.h"
#include "ModeFsm.h" 
#include "CcdAcqParams.h" 
#include "helpers.h"
#include "versionNo.h"
#include "ApgTimer.h"
#include "AscentBasedIo.h" 
#include "AspenIo.h"  
#include "AltaIo.h"  
#include <sstream>

namespace
{    
    // 12-28-2011 per disucssion with Wayne the firmware adds the value
    // of 2 to what is in register 20, so sw is going to pin the max 16 bit
    // value that can be calucated to 65530
    const double SHUTTER_CLOSE_DELAY_MAX = 0.49996;
    const double SHUTTER_CLOSE_DELAY_MIN = 0.000008;
    const double SHUTTER_CLOSE_DELAY_SLOPE = 131070;
    
    const int NUM_TEMP_2_AVG = 8;
}

//////////////////////////// 
// CTOR 
ApogeeCam::ApogeeCam(const CamModel::PlatformType platform) : 
                                m_PlatformType( platform ),
                                m_fileName( __FILE__ ),
                                m_FirmwareVersion( 0 ),
                                m_Id( 0 ),
                                m_NumImgsDownloaded(0),
                                m_ImageInProgress( false ),
                                m_IsPreFlashOn( false ),
                                m_IsInitialized( false ),
                                m_IsConnected(false),
								m_ExposureTimer( new ApgTimer ),
								m_LastExposureTime(0)
{ 
#ifdef DEBUGGING_CAMERA 
    ApgLogger::Instance().SetLogLevel( ApgLogger::LEVEL_DEBUG );
#endif

#ifdef DEBUGGING_CAMERA_STATUS
    ApgLogger::Instance().SetLogLevel( ApgLogger::LEVEL_DEBUG );
#endif

	// make sure start and stop have valid values
	m_ExposureTimer->Start();
	m_ExposureTimer->Stop();

} 

//////////////////////////// 
// DTOR 
ApogeeCam::~ApogeeCam() 
{ 
    std::string info;
    info.append( "Model: " + GetModel() + "\n" );
    info.append(  "Sensor: " + GetSensor() + "\n" );
   
    std::string msg = "Deleting camera:\n" + info;
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",msg); 
} 

//////////////////////////// 
// DEFAULT     INIT     
void ApogeeCam::DefaultInit()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::Init" );
#endif

     //issue camera reset
    Reset( false );

    SetFlushCommands( false );
    SetPostExposeFlushing( false );

    ClearAllRegisters();

    //issue camera reset
    Reset( false );

    // Load Inversion Mask
    WriteReg( CameraRegs::VRAM_INV_MASK, 
        m_CamCfgData->m_VerticalPattern.Mask );

	// Load the veritcal Pattern Files
    m_CamIo->LoadVerticalPattern( m_CamCfgData->m_VerticalPattern );

    WriteReg( CameraRegs::CLAMP_COUNT, 
        m_CamCfgData->m_MetaData.ClampColumns);

    // Set horizontal and vertical imaging roi registers
    m_CcdAcqSettings->SetImagingRegs( m_FirmwareVersion );

    // Since the default state of m_DigitizeOverscan is false, set the count to zero.
    WriteReg( CameraRegs::OVERSCAN_COUNT, 0x0 );

    //setup the ccd adc and load the appropreate horiztonal 
    //pattern files
    m_CcdAcqSettings->Init();

	// we don't use write_FlushBinningV() here because that would include additional RESETs
    WriteReg( CameraRegs::VFLUSH_BINNING, 
        m_CamCfgData->m_MetaData.VFlushBinning );

    if ( m_CamCfgData->m_MetaData.HFlushDisable )
    {
        m_CamIo->ReadOrWriteReg(CameraRegs::OP_A, 
            CameraRegs::OP_A_DISABLE_H_CLK_BIT);
    }

    Reset(true);

    SetImageCount( 1 );

    SetBulkDownload( true );
    SetSequenceDelay( m_CameraConsts->m_SequenceDelayMinimum );

    SetVariableSequenceDelay( true );

    //shutter close delay
   InitShutterCloseDelay();
 
    //set LED state
    //doing this becase reg 52 is write only, so we have
    //to write values here to set up the internal map
    WriteReg( CameraRegs::LED, 0);
    SetLedMode( Apg::LedMode_EnableAll );

    // Default values for I/O Port - the CLEAR op doesn't clear these values
	// This will also init our private vars to 0x0
	SetIoPortAssignment( 0x0 );
    SetIoPortBlankingBits( 0x0 );
	SetIoPortDirection( 0x0 );

	// Set the default TDI variables.  These also will automatically initialize
	// the "virtual" kinetics mode variables.
    SetTdiRate( m_CameraConsts->m_TdiRateDefault );
	SetTdiRows( 1 );
	SetTdiBinningRows( 1 );

    // Set the shutter strobe values to their defaults
    SetShutterStrobePeriod( m_CameraConsts->m_StrobePeriodDefault );
    SetShutterStrobePosition( m_CameraConsts->m_StrobePositionDefault );

	// Set default averaging state
    if ( m_CamCfgData->m_MetaData.DefaultDataReduction )
	{
        m_CamIo->ReadOrWriteReg(CameraRegs::OP_B, 
            CameraRegs::OP_B_AD_AVERAGING_BIT);
	}

	// Program our initial cooler values.  The only cooler value that we reset
	// at init time is the backoff point.  Everything else is left untouched, and
	// state information is determined from the camera controller.
	SetCoolerBackoffPoint( m_CamCfgData->m_MetaData.TempBackoffPoint );

    WriteReg(CameraRegs::TEMP_RAMP_DOWN_A,	
        m_CamCfgData->m_MetaData.TempRampRateOne );

	WriteReg(CameraRegs::TEMP_RAMP_DOWN_B, 
        m_CamCfgData->m_MetaData.TempRampRateTwo );

	// Set the Fan State. 
    SetFanMode( Apg::FanMode_Low, false );

    //set the vdd state
    if( 1 == m_CamCfgData->m_MetaData.AmpCutoffDisable )
    {
          m_CamIo->ReadOrWriteReg(CameraRegs::OP_A, 
            CameraRegs::OP_A_AMP_CUTOFF_DISABLE_BIT);
    }
    else
    {
         m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_AMP_CUTOFF_DISABLE_BIT) );
    }


    m_IsInitialized = true;

}

//////////////////////////// 
//      CLEAR        ALL     REGISTERS
void ApogeeCam::ClearAllRegisters()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::ClearAllRegisters" );
#endif
     // Issue a clear command, so the registers are zeroed out
	// This will put the camera in a known state for us.
    m_CamIo->ClearAllRegisters();
}


//////////////////////////// 
// INIT      SHUTTER   CLOSE       DELAY
void ApogeeCam::InitShutterCloseDelay()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::InitShutterCloseDelay" );
#endif
  
    //convert 300 ms to 0.3 sec
    double CloseDelay = m_CamCfgData->m_MetaData.ShutterCloseDelay / 1000.0;

    //if the default m_MetaData.ShutterCloseDelay is zero.
    //then set it to the min value
    if( CloseDelay < SHUTTER_CLOSE_DELAY_MIN )
    {
        CloseDelay = SHUTTER_CLOSE_DELAY_MIN;
    }
     
    SetShutterCloseDelay( CloseDelay );
}

//////////////////////////// 
//      DEFAULT    CFG        CAM    FROM    ID
void ApogeeCam::DefaultCfgCamFromId( const uint16_t CameraId )
{
    m_CamCfgData = std::shared_ptr<CApnCamData>( new CApnCamData );
    m_CamCfgData->Set( apgHelper::GetCamCfgDir(), 
        apgHelper::GetCfgFileName(), CameraId );
}

//////////////////////////// 
// RESET 
void ApogeeCam::Reset()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::Reset");
#endif
    //public function
    //log the call to reset the camera
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
        apgHelper::mkMsg( m_fileName, "Camera Reset Called", __LINE__) );
    //if( m_ImageInProgress )
    {
        // reseting camera and cancel pending image
        // transfer
        HardStopExposure( "Called from Reset()" );
    }
    //else
    {
        m_CamIo->Reset( true );
    }

}

//////////////////////////// 
// RESET 
void ApogeeCam::Reset(const bool Flush)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::Reset -> Flush = %d", Flush );
#endif
    
    m_CamIo->Reset( Flush );

    if( m_ImageInProgress )
    {

    }
}

//////////////////////////// 
//      CLOSE     CONNECTION
void ApogeeCam::DefaultCloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::DefaultCloseConnection");
#endif

     //log that we are closing the connection to
    LogConnectAndDisconnect( false );

    // try to cancel an exposure if it is in progress
    CancelExposureNoThrow();

    //deleting classes that depend on cam IO
    m_CamMode.reset();

    m_CcdAcqSettings.reset();

    //delete the camera IO itself
    m_CamIo.reset();

    m_IsConnected = false;
}

//////////////////////////// 
// LOG      CONNECT     AND     DISCONNECT
void ApogeeCam::LogConnectAndDisconnect( bool Connect )
{
    std::string msg;

    if( Connect )
    {
        msg.append( "Successfully created connected to camera:\n" );
        msg.append( GetInfo() );
    }
    else
    {
        msg.append( "Disconnecting camera:\n" );
        msg.append( "Model: " + GetModel() + "\n" );
        msg.append( "Sensor: " + GetSensor() + "\n" );
    }
    
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",msg); 
}



//////////////////////////// 
// READ     REG
uint16_t ApogeeCam::ReadReg( const uint16_t reg )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::ReadReg -> reg = %d", reg );
#endif
 
    return m_CamIo->ReadReg( reg );
}


//////////////////////////// 
// WRITE     REG
void  ApogeeCam::WriteReg( const uint16_t reg,  
                          const uint16_t value)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::WriteReg -> reg = %d, value = 0x%X", reg, value );
#endif
    
    m_CamIo->WriteReg( reg, value );
}



//////////////////////////// 
// SET       ROI       NUM     ROWS 
void ApogeeCam::SetRoiNumRows( const uint16_t rows )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiNumRows -> rows = %d", rows );
#endif
    
    m_CcdAcqSettings->SetRoiNumRows( rows );
}

//////////////////////////// 
// SET       ROI       NUM     COLS
void ApogeeCam::SetRoiNumCols( const uint16_t cols )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiNumCols -> cols = %d", cols );
#endif

    m_CcdAcqSettings->SetRoiNumCols( cols );
}

//////////////////////////// 
// GET       ROI       NUM     ROWS 
uint16_t ApogeeCam::GetRoiNumRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiNumRows" );
#endif
    return m_CcdAcqSettings->GetRoiNumRows();
}

//////////////////////////// 
// GET       ROI       NUM     COLS
uint16_t ApogeeCam::GetRoiNumCols()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiNumCols" );
#endif

    return m_CcdAcqSettings->GetRoiNumCols();
}

//////////////////////////// 
// SET       ROI       START ROW
void ApogeeCam::SetRoiStartRow( const uint16_t row )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiStartRow -> row = %d", row );
#endif
    
    return m_CcdAcqSettings->SetRoiStartRow( row );
}

//////////////////////////// 
// SET       ROI       START     COL
void ApogeeCam::SetRoiStartCol( const uint16_t col )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiStartCol ->col = %d", col );
#endif
    
    return m_CcdAcqSettings->SetRoiStartCol( col );
}
        
//////////////////////////// 
// GET       ROI       START     ROW
uint16_t ApogeeCam::GetRoiStartRow()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiStartRow" );
#endif

    return m_CcdAcqSettings->GetRoiStartRow();
}

//////////////////////////// 
// GET       ROI       START     COL
uint16_t ApogeeCam::GetRoiStartCol()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiStartCol" );
#endif

    return m_CcdAcqSettings->GetRoiStartCol();
}

//////////////////////////// 
// SET       ROI       START     COL
void ApogeeCam::SetRoiBinRow( const uint16_t bin )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiBinRow -> bin = %d", bin );
#endif

    m_CcdAcqSettings->SetNumRows2Bin( bin );
}

//////////////////////////// 
// GET       ROI       START     COL
uint16_t ApogeeCam::GetRoiBinRow()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiBinRow" );
#endif

    return m_CcdAcqSettings->GetNumRows2Bin();
}

//////////////////////////// 
// SET       ROI       START     COL
void ApogeeCam::SetRoiBinCol( const uint16_t bin )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetRoiBinCol -> bin = %d", bin );
#endif

    m_CcdAcqSettings->SetNumCols2Bin( bin );
}

//////////////////////////// 
// GET       ROI       START     COL
uint16_t ApogeeCam::GetRoiBinCol()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetRoiBinCol" );
#endif

    return m_CcdAcqSettings->GetNumCols2Bin();
}

//////////////////////////// 
// VERIFY       FRMWR       REV  
void ApogeeCam::VerifyFrmwrRev()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::VerifyFrmwrRev" );
#endif

    const uint16_t rev = m_CamIo->GetFirmwareRev();

    if( rev != m_FirmwareVersion )
    {
        std::string errStr = "firmware rev mis-match expected rev =" + 
            help::uShort2Str(m_FirmwareVersion) + " received from camera rev = " +
            help::uShort2Str(rev);
        apgHelper::throwRuntimeException( m_fileName, errStr, 
            __LINE__, Apg::ErrorType_Connection );
    }
    
}

//////////////////////////// 
// SET    IMAGE   COUNT
void ApogeeCam::SetImageCount( const uint16_t count )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetImageCount -> count = %d", count );
#endif
     
    if ( count == 0 )
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing image count  from " << count << " to " << 1;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        WriteReg( CameraRegs::IMAGE_COUNT, 1 );
    }
    else
    {
         WriteReg( CameraRegs::IMAGE_COUNT, count );
    }
}
//////////////////////////// 
// GET    IMAGE   COUNT
uint16_t  ApogeeCam::GetImageCount()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetImageCount" );
#endif

    return m_CamIo->ReadMirrorReg( CameraRegs::IMAGE_COUNT );
}

//////////////////////////// 
// GET  IMG    SEQUENCE        COUNT
uint16_t ApogeeCam::GetImgSequenceCount()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetImgSequenceCount" );
#endif

    CameraStatusRegs status = GetStatus();

    if( m_CamMode->IsBulkDownloadOn() )
    {
        return status.GetSequenceCounter();
    }
    else
    {
        return status.GetReadyFrame();
    }
}

//////////////////////////// 
// SET    SEQUENCE    DELAY
void ApogeeCam::SetSequenceDelay( double delay )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetSequenceDelay -> delay= %f", delay );
#endif

    // boundary checking AND correcting...
    if ( delay > m_CameraConsts->m_SequenceDelayMaximum )
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input sequence delay  from " << delay << " to " << m_CameraConsts->m_SequenceDelayMaximum;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

		delay = m_CameraConsts->m_SequenceDelayMaximum;
    }

    if ( delay < m_CameraConsts->m_SequenceDelayMinimum )
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input sequence delay  from " << delay << " to " << m_CameraConsts->m_SequenceDelayMinimum;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

		delay = m_CameraConsts->m_SequenceDelayMinimum;
    }

    uint16_t SequenceDelay = static_cast<uint16_t>(delay / m_CameraConsts->m_SequenceDelayResolution );

	WriteReg( CameraRegs::SEQUENCE_DELAY, SequenceDelay );
}

//////////////////////////// 
// GET    SEQUENCE    DELAY
double ApogeeCam::GetSequenceDelay()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetSequenceDelay" );
#endif

    uint16_t SequenceDelay = m_CamIo->ReadMirrorReg( CameraRegs::SEQUENCE_DELAY);

    return (m_CameraConsts->m_SequenceDelayResolution*SequenceDelay);
}

//////////////////////////// 
// SET    VARIABLE      SEQUENCE    DELAY
void ApogeeCam::SetVariableSequenceDelay( const bool variable )
{
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::SetVariableSequenceDelay -> variable = %d", variable );
 #endif
   
	if ( variable )
    {
        // variable when zero
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_A, 
            static_cast<uint16_t>(~CameraRegs::OP_A_DELAY_MODE_BIT) );
    }
	else
    {
        // constant when one
        m_CamIo->ReadOrWriteReg(CameraRegs::OP_A, 
            CameraRegs::OP_A_DELAY_MODE_BIT );
    }
}

//////////////////////////// 
// GET    VARIABLE      SEQUENCE    DELAY
 bool ApogeeCam::GetVariableSequenceDelay()
 {
 #ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::GetVariableSequenceDelay" );
 #endif

     const uint16_t val = ReadReg( CameraRegs::OP_A );
     
     // constant when one
    // variable when zero
     return ( val & CameraRegs::OP_A_DELAY_MODE_BIT ? false : true );
 }

//////////////////////////// 
// SET   TDI    RATE
void ApogeeCam::SetTdiRate( double TdiRate )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetTdiRate -> rate = %f", TdiRate );
#endif

	if ( TdiRate <  m_CameraConsts->m_TdiRateMin )
    {
         //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input tdi rate from " << TdiRate << " to " << m_CameraConsts->m_TdiRateMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        TdiRate = m_CameraConsts->m_TdiRateMin;
    }
	
	if ( TdiRate > m_CameraConsts->m_TdiRateMax )
    {
         //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input tdi rate from " << TdiRate << " to " << m_CameraConsts->m_TdiRateMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

		TdiRate = m_CameraConsts->m_TdiRateMax;
    }

    uint16_t Rate2Set = static_cast<uint16_t>( TdiRate / 
        m_CameraConsts->m_TdiRateResolution );

    WriteReg( CameraRegs::TDI_RATE, Rate2Set );
}

//////////////////////////// 
// GET   TDI    RATE
double ApogeeCam::GetTdiRate()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTdiRate" );
#endif

    const double rate = m_CamIo->ReadMirrorReg(CameraRegs::TDI_RATE)*
        m_CameraConsts->m_TdiRateResolution;

    return rate;
}

//////////////////////////// 
// SET   TDI    ROWS
void ApogeeCam::SetTdiRows( const uint16_t TdiRows )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetTdiRows ->TdiRows = %d", TdiRows );
#endif

    // Make sure the TDI row count is at least 1
	if ( TdiRows == 0 )	
    {
         //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input tdi rows from " << TdiRows << " to " << 1;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        // not using driect register io here, because we
        // have to save a copy of the number of rows, because the
        // tdi rows must be set to 0 in trigger each mode
        m_CamMode->SetTdiRows( 1 );
        
    }
    else
    {
         // not using driect register io here, because we
        // have to save a copy of the number of rows, because the
        // tdi rows must be set to 0 in trigger each mode
        m_CamMode->SetTdiRows( TdiRows );
    }
}

//////////////////////////// 
// GET   TDI    ROWS
uint16_t ApogeeCam::GetTdiRows()
{
    // not using driect register io here, because we
    // have to save a copy of the number of rows, because the
    // tdi rows must be set to 0 in trigger each mode
    const uint16_t rows =  m_CamMode->GetTdiRows();

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTdiRows -> rows = %d", rows );
#endif

    return rows;
}

//////////////////////////// 
// GET   TDI    COUTNER
uint16_t ApogeeCam::GetTdiCounter()
{
    CameraStatusRegs status = GetStatus();
    const uint16_t result = IsBulkDownloadOn() ? status.GetTdiCounter() : status.GetReadyFrame();

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTdiCounter -> counter = %d", result );
#endif

    return( result  );
}


//////////////////////////// 
// SET   TDI    BINNING  ROWS
void ApogeeCam::SetTdiBinningRows( const uint16_t bin )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetTdiBinningRows -> bin = %d", bin );
#endif

	if ( 0 == bin)	
    {
        // Make sure the TDI binning is at least 1
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input tdi bining v from " << bin << " to " << 1;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);
        WriteReg( CameraRegs::TDI_BINNING, 1 );
    }
    else
    {
        WriteReg( CameraRegs::TDI_BINNING, bin );
    }      
}

//////////////////////////// 
// GET   TDI    BINNING  ROWS
uint16_t ApogeeCam::GetTdiBinningRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTdiBinningRows" );
#endif

    return m_CamIo->ReadMirrorReg( CameraRegs::TDI_BINNING );
}

//////////////////////////// 
// SET          KINETICS   SECTION       HEIGHT
void ApogeeCam::SetKineticsSectionHeight( uint16_t height )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetKineticsSectionHeight -> height = %d", height );
#endif

    SetTdiBinningRows( height );
}

//////////////////////////// 
// GET      KINETICS   SECTION       HEIGHT
uint16_t ApogeeCam::GetKineticsSectionHeight()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetKineticsSectionHeight" );
#endif
    return GetTdiBinningRows();
}

//////////////////////////// 
// SET          KINETICS           SECTIONS
void ApogeeCam::SetKineticsSections( uint16_t sections )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetKineticsSections -> sections = %d", sections );
#endif

    SetTdiRows( sections );
}

//////////////////////////// 
// GET      KINETICS       SECTIONS
uint16_t ApogeeCam::GetKineticsSections()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetKineticsSections" );
#endif

    return GetTdiRows();
}

//////////////////////////// 
// SET      KINETICS      SHIFT         INTERVAL
void ApogeeCam::SetKineticsShiftInterval( double  interval)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetKineticsShiftInterval -> interval = %f", interval );
#endif

    SetTdiRate( interval );
}

//////////////////////////// 
// GET      KINETICS      SHIFT         INTERVAL
double ApogeeCam::GetKineticsShiftInterval()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetKineticsSections" );
#endif

    return GetTdiRate();
}

//////////////////////////// 
// SET    SHUTTER       STROBE     POSITION
void ApogeeCam::SetShutterStrobePosition( double position )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetShutterStrobePosition -> pos = %f", position);
#endif
	
	//boundary checking
    if ( position < m_CameraConsts->m_StrobePositionMin)
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input strobe position from " << position << " to " << m_CameraConsts->m_StrobePositionMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        position = m_CameraConsts->m_StrobePositionMin;
    }


    if ( position > m_CameraConsts->m_StrobePositionMax)
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input strobe position from " << position << " to " << m_CameraConsts->m_StrobePositionMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        position = m_CameraConsts->m_StrobePositionMax;
    }

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "m_CameraConsts->m_StrobePositionMin = %e", m_CameraConsts->m_StrobePositionMin);
    apgHelper::DebugMsg( "m_CameraConsts->m_StrobePositionMax = %e", m_CameraConsts->m_StrobePositionMax);
//    apgHelper::DebugMsg( "m_CameraConsts->m_preflashTimerResolution = %e", m_CameraConsts->m_preflashTimerResolution);
    apgHelper::DebugMsg( "m_CameraConsts->m_StrobePositionMin = %e", m_CameraConsts->m_StrobePositionMin);
#endif
    double temp = position - m_CameraConsts->m_StrobePositionMin;
    temp /= m_CameraConsts->m_StrobeTimerResolution;
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "calculated ShutterStrobePosition as float = %e", temp);
    if (temp > 65535.0) {
        apgHelper::DebugMsg( "calculated ShutterStrobePosition will overflow register" );
    }
#endif
    uint16_t ShutterStrobePosition = static_cast<uint16_t>(temp);

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "calculated ShutterStrobePosition as uint16_t = %d", ShutterStrobePosition);
    uint16_t current = ReadReg(CameraRegs::SHUTTER_STROBE_POSITION);
    apgHelper::DebugMsg( "read ShutterStrobePosition = 0x%X", current);
#endif

    WriteReg(CameraRegs::SHUTTER_STROBE_POSITION, ShutterStrobePosition);
#ifdef DEBUGGING_CAMERA
    current = ReadReg(CameraRegs::SHUTTER_STROBE_POSITION);
    apgHelper::DebugMsg( "read ShutterStrobePosition = 0x%X", current);
#endif
}

//////////////////////////// 
// GET    SHUTTER       STROBE     POSITION
double ApogeeCam::GetShutterStrobePosition()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetShutterStrobePosition" );
#endif

    uint16_t ShutterStrobePosition =  m_CamIo->ReadMirrorReg(CameraRegs::SHUTTER_STROBE_POSITION);
    return (  (ShutterStrobePosition*m_CameraConsts->m_StrobeTimerResolution) 
    + m_CameraConsts->m_StrobePositionMin );
}

//////////////////////////// 
// SET    SHUTTER       STROBE     PERIOD
void ApogeeCam::SetShutterStrobePeriod( double period )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetShutterStrobePeriod -> period = %f", period);
#endif

    //boundary checking
     if ( period < m_CameraConsts->m_StrobePeriodMin)
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input strobe period from " << period << " to " << m_CameraConsts->m_StrobePeriodMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        period = m_CameraConsts->m_StrobePeriodMin;
    }

    if ( period > m_CameraConsts->m_StrobePeriodMax)
    {
        //log a warning that we are changing the input value under the sheets
        std::stringstream msg;
        msg << "Changing input strobe position from " <<period << " to " << m_CameraConsts->m_StrobePeriodMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        period = m_CameraConsts->m_StrobePeriodMax;
    }

	uint16_t ShutterStrobePeriod = static_cast<uint16_t>((period - m_CameraConsts->m_StrobePeriodMin) / 
        m_CameraConsts->m_PeriodTimerResolution);

    WriteReg(CameraRegs::SHUTTER_STROBE_PERIOD,ShutterStrobePeriod );
}

//////////////////////////// 
// GET    SHUTTER       STROBE     POSITION
double ApogeeCam::GetShutterStrobePeriod()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetShutterStrobePeriod" );
#endif
   
    uint16_t ShutterStrobePeriod = m_CamIo->ReadMirrorReg(CameraRegs::SHUTTER_STROBE_PERIOD);
    
    return ( (ShutterStrobePeriod*m_CameraConsts->m_PeriodTimerResolution) + m_CameraConsts->m_StrobePeriodMin );
}

//////////////////////////// 
// SET    SHUTTER      CLOSE       DELAY
void ApogeeCam::SetShutterCloseDelay( double delay )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetShutterCloseDelay -> delay = %f", delay );
#endif

    if( delay > SHUTTER_CLOSE_DELAY_MAX )
    {
        std::stringstream msg;
        msg << "Changing input shutter delay from " << delay;
        msg << " to " << SHUTTER_CLOSE_DELAY_MAX;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        delay = SHUTTER_CLOSE_DELAY_MAX;
    }

    if( delay < SHUTTER_CLOSE_DELAY_MIN )
    {
        std::stringstream msg;
        msg << "Changing input shutter delay from " << delay;
        msg << " to " << SHUTTER_CLOSE_DELAY_MIN;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        delay = SHUTTER_CLOSE_DELAY_MIN;
    }

    uint16_t ShutterCloseDelay = static_cast<uint16_t>( delay*SHUTTER_CLOSE_DELAY_SLOPE );
   
    WriteReg(CameraRegs::SHUTTER_CLOSE_DELAY, ShutterCloseDelay );
}

//////////////////////////// 
// GET    SHUTTER      CLOSE       DELAY
double ApogeeCam::GetShutterCloseDelay()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetShutterCloseDelay" );
#endif

    uint16_t delay = m_CamIo->ReadMirrorReg(CameraRegs::SHUTTER_CLOSE_DELAY);

    double val = delay / SHUTTER_CLOSE_DELAY_SLOPE;

    return( val );
}

//////////////////////////// 
// SET    COOLER        BACKOFF       POINT
void ApogeeCam::SetCoolerBackoffPoint( double point )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCoolerBackoffPoint -> point = %f", point );
#endif
    if (point == 0.0)
    {
        WriteReg( CameraRegs::TEMP_BACKOFF, 0 );
        return;
    }

	double TempVal = point;

    //boundary checking
    if ( point < m_CameraConsts->m_TempBackoffpointMin )
    {
        std::stringstream msg;
        msg << "Changing input cooler backoff point from " << point;
        msg << " to " << m_CameraConsts->m_TempBackoffpointMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

		TempVal = m_CameraConsts->m_TempBackoffpointMin;
    }

	if ( point  > m_CameraConsts->m_TempBackoffpointMax )
    {
        std::stringstream msg;
        msg << "Changing input cooler backoff point from " << point;
        msg << " to " << m_CameraConsts->m_TempBackoffpointMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        TempVal = m_CameraConsts->m_TempBackoffpointMax;
    }

	const uint16_t CoolerBackoffPoint = static_cast<uint16_t>( 
        TempVal / m_CameraConsts->m_TempDegreesPerBit);
	
    WriteReg( CameraRegs::TEMP_BACKOFF, CoolerBackoffPoint );
}

//////////////////////////// 
// GET    COOLER        BACKOFF       POINT
double ApogeeCam::GetCoolerBackoffPoint()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCoolerBackoffPoint" );
#endif

    return (m_CamIo->ReadMirrorReg( CameraRegs::TEMP_BACKOFF ) * 
        m_CameraConsts->m_TempDegreesPerBit);
}

//////////////////////////// 
// SET    COOLER      SET    POINT
void ApogeeCam::SetCoolerSetPoint( const double point )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCoolerSetPoint -> point = %f", point );
#endif
  
    //The hardware document defines a number of ranges for the cooler set point
    //register (r55) in Kelvin.  We will deal with the temperature in Celsius, because
    //that is what the end user sees.  We can do this because the temperature intervals
    //for K and C are the same and we have clamped the min and max values in C to
    //what is defined in the firmware document in K.  For example on the alta, 
    //min = 213 K or ~-60 C and max = 313 K or ~39 C. 
	double TempVal = point;

    //boundary checking
    if ( point < m_CameraConsts->m_TempSetpointMin )
    {
        std::stringstream msg;
        msg << "Changing input cooler set point from " << point;
        msg << " to " << m_CameraConsts->m_TempSetpointMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        TempVal = m_CameraConsts->m_TempSetpointMin;
    }

    if ( point > m_CameraConsts->m_TempSetpointMax )
    {
        std::stringstream msg;
        msg << "Changing input cooler set point from " << point;
        msg << " to " << m_CameraConsts->m_TempSetpointMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        TempVal = m_CameraConsts->m_TempSetpointMax;
    }

    //using y=mx+b to figure out the input register value
    // b = emperically determined register zero crossing value
    // x = the input temp divided by the degrees per bit
    // m = 1
    const uint16_t CoolerSetPoint = static_cast<uint16_t>( 
        (TempVal / m_CameraConsts->m_TempDegreesPerBit) + m_CameraConsts->m_TempSetpointZeroPoint);
	
    WriteReg( CameraRegs::TEMP_DESIRED, CoolerSetPoint );
}

//////////////////////////// 
// GET    COOLER      SET    POINT
double ApogeeCam::GetCoolerSetPoint()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCoolerSetPoint" );
#endif

    //12 bit value
    const uint16_t CoolerSetPoint = ReadReg( CameraRegs::TEMP_DESIRED ) & 0x0FFF;

    return( (CoolerSetPoint - m_CameraConsts->m_TempSetpointZeroPoint ) * 
        m_CameraConsts->m_TempDegreesPerBit );
}

//////////////////////////// 
// ISSUE     EXPOSE     CMD  
void ApogeeCam::IssueExposeCmd( const bool IsLight )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IssueExposeCmd -> IsLight = %d", IsLight );
#endif

    //figure out what exposure method we are using
	uint16_t CmdValue = 0;

    switch ( m_CamMode->GetMode() )
    {
        case Apg::CameraMode_Normal:
            if ( IsLight )
            {
                CmdValue = CameraRegs::CMD_A_EXPOSE_BIT;
            }
            else
            {
                CmdValue = CameraRegs::CMD_A_DARK_BIT;
            }
        break;

        case Apg::CameraMode_TDI:
            CmdValue = CameraRegs::CMD_A_TDI_BIT;

            if ( !IsLight && !IsShutterForcedClosed() )
            {
                SetShutterState( Apg::ShutterState_ForceClosed );
                //TODO
                //m_pvtRestoreShutterControl = true;
            }
        break;

        case Apg::CameraMode_Kinetics:
            CmdValue = CameraRegs::CMD_A_KINETICS_BIT;
        break;

        default:
       {
            std::stringstream msg;
            msg << "Invalid camera mode " << m_CamMode->GetMode();
            apgHelper::throwRuntimeException(m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidMode );
       }
        break;
    }

	// Send the expose type to the camera
	WriteReg( CameraRegs::CMD_A, CmdValue );
}

//////////////////////////// 
//  IS     THERE       A  STATUS     ERROR
void ApogeeCam::IsThereAStatusError( const uint16_t statusReg )
{
    if( statusReg & CameraRegs::STATUS_PATTERN_ERROR_BIT )
    {
        std::stringstream ss;
        ss << "Camera Pattern Error Bit Set. Status reg = " << statusReg;
        apgHelper::throwRuntimeException( m_fileName, ss.str(), 
            __LINE__, Apg::ErrorType_Serious );
    }

    if( statusReg & CameraRegs::STATUS_DATA_HALTED_BIT )
    {
        // per a discussion with wayne, just log this error
        // don't throw, b/c reading the image clears it
        std::stringstream msg2log;
        msg2log << "Status_DataError";
        msg2log << "; status register = " << statusReg;
        std::string msgStr = apgHelper::mkMsg(m_fileName, msg2log.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE, "error", msgStr);
    }

    if( m_CamIo->IsError() )
    {
         apgHelper::throwRuntimeException( m_fileName, 
             "Camera IO comms error" , __LINE__, Apg::ErrorType_Critical );
    }

}

//////////////////////////// 
//  IS     IMG     DONE
bool ApogeeCam::IsImgDone( const CameraStatusRegs & statusObj)
{
	bool doneBitSet = statusObj.GetStatus() & CameraRegs::STATUS_IMAGE_DONE_BIT ? true : false;
	bool dataFlag = false;

	if( CamModel::ASPEN == m_PlatformType && m_CamIo->GetInterfaceType() == CamModel::ETHERNET && true == IsPipelineDownloadOn())
    {
		m_ExposureTimer->Stop();
        dataFlag = m_ExposureTimer->GetTimeInSec() >= m_LastExposureTime ? true : false;
    }
    else if( m_CamIo->GetInterfaceType() == CamModel::ETHERNET || false == IsPipelineDownloadOn())
    {
		dataFlag = false;
    }
    else 
    {
        //pipeline download by reporting image done as soon as data is available
        dataFlag = statusObj.GetDataAvailFlag();
    }
    return( doneBitSet || dataFlag ? true : false );
}
//////////////////////////// 
//LOG        AND      RETURN        STATUS
Apg::Status ApogeeCam::LogAndReturnStatus( const Apg::Status status,
                                         const CameraStatusRegs & statusObj)
{
    //log the error
    if( status == Apg::Status_ConnectionError	||
        status == Apg::Status_DataError ||
        status == Apg::Status_PatternError)
    {
        std::stringstream msg2log;
        msg2log << "Imaging status = " << status;
        msg2log << "; Camera Mode = " << GetCameraMode() << "; ";
        msg2log << statusObj.GetStatusStr();
        std::string msgStr = apgHelper::mkMsg(m_fileName, msg2log.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE, "error", msgStr);
    }
    else
    {
         #ifdef DEBUGGING_CAMERA
            apgHelper::DebugMsg( "ApogeeCam::LogAndReturnStatus -> status = %d",  status);
        #endif
    }

    return status;
}


//////////////////////////// 
// GET       CAMERA     MODE 
Apg::CameraMode ApogeeCam::GetCameraMode()
{
    const Apg::CameraMode mode = m_CamMode->GetMode();

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCameraMode -> mode = %d", mode );
#endif

    return mode;
}

//////////////////////////// 
// SET       CAMERA     MODE 
void ApogeeCam::SetCameraMode( const Apg::CameraMode mode )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCameraMode -> mode = %d", mode );
#endif

    m_CamMode->SetMode( mode );
}

//////////////////////////// 
// SET      FAST   SEQUENCE
void ApogeeCam::SetFastSequence( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetFastSequence -> TurnOn = %d", TurnOn );
#endif

    m_CamMode->SetFastSequence( TurnOn );

    if( TurnOn )
    {
        //set the shutter delay to zero
        SetShutterCloseDelay( 0.0 );
    }
    else
    {
        //restore shutter delay to it default
        InitShutterCloseDelay();
    }
}
        
//////////////////////////// 
// IS      FAST      SEQUENCE    ON
bool ApogeeCam::IsFastSequenceOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsFastSequenceOn" );
#endif

    return m_CamMode->IsFastSequenceOn();
}


//////////////////////////// 
//  SET         BULK      DOWNLOAD
void ApogeeCam::SetBulkDownload( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetBulkDownload -> TurnOn = %d", TurnOn );
#endif

    m_CamMode->SetBulkDownload( TurnOn );
}

//////////////////////////// 
//      IS         BULK          DOWNLOAD   ON
bool ApogeeCam::IsBulkDownloadOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsBulkDownloadOn" );
#endif

     return m_CamMode->IsBulkDownloadOn();
}


//////////////////////////// 
//  SET         PIPELINE      DOWNLOAD
void ApogeeCam::SetPipelineDownload( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetPipelineDownload -> TurnOn = %d", TurnOn );
#endif

    m_CamMode->SetPipelineDownload( TurnOn );
}

//////////////////////////// 
//      IS         PIPELINE          DOWNLOAD   ON
bool ApogeeCam::IsPipelineDownloadOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsPipelineDownloadOn" );
#endif

     return m_CamMode->IsPipelineDownloadOn();
}

//////////////////////////// 
// SET    IO     PORT    ASSIGNMENT
void ApogeeCam::SetIoPortAssignment( const uint16_t assignment )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetIoPortAssignment -> assignment = %d", assignment );
#endif

   const uint16_t val2write = assignment & CameraRegs::IO_PORT_ASSIGNMENT_MASK;
    WriteReg( CameraRegs::IO_PORT_ASSIGNMENT, val2write);
}

//////////////////////////// 
// GET    IO     PORT    ASSIGNMENT
uint16_t ApogeeCam::GetIoPortAssignment()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetIoPortAssignment" );
#endif

    return m_CamIo->ReadMirrorReg( 
        CameraRegs::IO_PORT_ASSIGNMENT );
}

//////////////////////////// 
// SET    IO        PORT    BLANKING     BITS
void ApogeeCam::SetIoPortBlankingBits( const uint16_t blankingBits )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetIoPortBlankingBits -> blankingBits = %d", blankingBits );
#endif

    const uint16_t val2write = blankingBits & CameraRegs::IO_PORT_BLANKING_BITS_MASK;
    WriteReg( CameraRegs::IO_PORT_BLANKING_BITS, 
        val2write );
}

//////////////////////////// 
// GET    IO        PORT    BLANKING     BITS
uint16_t ApogeeCam::GetIoPortBlankingBits()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetIoPortBlankingBits" );
#endif

    return m_CamIo->ReadMirrorReg( 
        CameraRegs::IO_PORT_BLANKING_BITS );
}
	
//////////////////////////// 
// SET    IO     PORT    DIRECTION
void ApogeeCam::SetIoPortDirection( const uint16_t direction )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetIoPortDirection -> direction = %d", direction );
#endif

    const uint16_t val2write = direction & CameraRegs::IO_PORT_DIRECTION_MASK;
    WriteReg( CameraRegs::IO_PORT_DIRECTION, val2write );
}

//////////////////////////// 
// GET    IO     PORT    DIRECTION
uint16_t ApogeeCam::GetIoPortDirection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetIoPortDirection" );
#endif

    return m_CamIo->ReadMirrorReg( 
        CameraRegs::IO_PORT_DIRECTION );
}

//////////////////////////// 
// SET    IO     PORT      DATA
void ApogeeCam::SetIoPortData( const uint16_t data )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetIoPortData -> data = %d", data );
#endif

    const uint16_t val2write = data & CameraRegs::IO_PORT_DATA_MASK;
    WriteReg( CameraRegs::IO_PORT_DATA_WRITE, val2write );
}

//////////////////////////// 
// GET    IO     PORT      DATA
uint16_t ApogeeCam::GetIoPortData()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetIoPortData" );
#endif

    const uint16_t data = ReadReg( CameraRegs::IO_PORT_DATA_READ ) &
        CameraRegs::IO_PORT_DATA_MASK;
    return  data;
}


//////////////////////////// 
// EXECTUE        PREFLASH
void ApogeeCam::ExectuePreFlash()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::ExectuePreFlash" );
#endif

    //save the external trigger values and 
    //turn them off to take the dark preflash
    //image
    std::vector< std::pair<Apg::TriggerMode,Apg::TriggerType> > trigs =
        m_CamMode->GetTrigsThatAreOn();

    //turn off the triggers that are on
   std::vector< std::pair<Apg::TriggerMode,Apg::TriggerType> >::iterator iter;
    for(iter = trigs.begin(); iter != trigs.end(); ++iter)
    {
        m_CamMode->SetExternalTrigger(false, iter->first, iter->second );
    }

    //save the current strobe value to reset later
    const double strobePos = GetShutterStrobePosition();
    const double PreFlashDuration = m_CamCfgData->m_MetaData.IRPreflashTime / 1000.0;
    // write the time for the IR pre-flash, which is different than
    // the strobe position for a regular exposure
    SetShutterStrobePosition( PreFlashDuration );

    // disable fpga fifo writes
    m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
        CameraRegs::OP_B_FIFO_WRITE_BLOCK_BIT);

    //set the preflash bit
    m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
        CameraRegs::OP_B_IR_PREFLASH_ENABLE_BIT);

    // Calculate and set the preflash exposure time 
    const double PreFlashExposureTime = PreFlashDuration + 0.050;
    SetExpsoureTime( PreFlashExposureTime );

    // start the dark exposure
    WriteReg( CameraRegs::CMD_A, 
        CameraRegs::CMD_A_DARK_BIT );

    apgHelper::ApogeeSleep( m_CamCfgData->m_MetaData.IRPreflashTime );    

    //wait for the image
    int32_t WaitCounter = 0;
    while( GetImagingStatus() != Apg::Status_Flushing )
	{
        apgHelper::ApogeeSleep( 20 );

		++WaitCounter;
		
		if ( WaitCounter > 1000 )
		{
            // we've waited longer than 3s to start flushing in the camera head
            // something is amiss...abort the expose command to avoid an infinite loop
            std::string msg( "Preflash dark image failed to finish.");
            apgHelper::throwRuntimeException(m_fileName, msg, 
                __LINE__, Apg::ErrorType_Critical );
		}
	}

    //turn off pre-flash
    m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
        static_cast<uint16_t>(~CameraRegs::OP_B_IR_PREFLASH_ENABLE_BIT) );

    // allow fifo writes!!
    m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
        static_cast<uint16_t>(~CameraRegs::OP_B_FIFO_WRITE_BLOCK_BIT) );

    //restore the triggers to its previous state
    for(iter = trigs.begin(); iter != trigs.end(); ++iter)
    {
        m_CamMode->SetExternalTrigger(true, iter->first, iter->second );
    }

    //restore strob pos, reg23 to is original value
    //for regular exposures
    SetShutterStrobePosition( strobePos );
}

//////////////////////////// 
//  SET      EXPSOURE    TIME
void ApogeeCam::SetExpsoureTime( const double Duration )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetExpsoureTime -> Duration = %f", Duration );
#endif

    const uint32_t ExpTime = 
        static_cast<uint32_t>( (Duration / m_CameraConsts->m_TimerResolution) ) + 
        m_CameraConsts->m_TimerOffsetCount;

    const uint16_t ExpTimeLow = (ExpTime & 0xFFFF);
    WriteReg( CameraRegs::TIMER_LOWER, ExpTimeLow);
    const uint16_t ExpTimeHigh = ( (ExpTime >> 16) & 0xFFFF);
    WriteReg( CameraRegs::TIMER_UPPER, ExpTimeHigh);
}

//////////////////////////// 
//  SET   EXTERNAL TRIGGER 
void ApogeeCam::SetExternalTrigger( bool TurnOn, const Apg::TriggerMode trigMode,
            const Apg::TriggerType trigType )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetExternalTrigger -> TurnOn = %d, trigMode = %d, trigType = %d", 
        TurnOn, trigMode, trigType );
#endif

    m_CamMode->SetExternalTrigger( TurnOn, trigMode, trigType );
}

//////////////////////////// 
//  IS     TRIGGER   NORM    EACH       ON
bool ApogeeCam::IsTriggerNormEachOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsTriggerNormEachOn" );
#endif

    return m_CamMode->IsTriggerNormEachOn();
}
        
//////////////////////////// 
//  IS     TRIGGER   NORM    GROUP       ON
bool ApogeeCam::IsTriggerNormGroupOn()
{
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::IsTriggerNormGroupOn" );
 #endif

    return m_CamMode->IsTriggerNormGroupOn();
}
//////////////////////////// 
//  IS     TRIGGER   TDIKIN    EACH     ON
bool ApogeeCam::IsTriggerTdiKinEachOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsTriggerTdiKinEachOn" );
#endif

    return m_CamMode->IsTriggerTdiKinEachOn();
}

//////////////////////////// 
//  IS     TRIGGER   TDIKIN    GROUP     ON
bool ApogeeCam::IsTriggerTdiKinGroupOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsTriggerTdiKinGroupOn" );
#endif

    return m_CamMode->IsTriggerTdiKinGroupOn();
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL      SHUTTER   ON
bool ApogeeCam::IsTriggerExternalShutterOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsTriggerExternalShutterOn" );
#endif

    return m_CamMode->IsTriggerExternalShutterOn();
}

//////////////////////////// 
//  IS     TRIGGER   EXTERNAL        READOUT     ON
bool ApogeeCam::IsTriggerExternalReadoutOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsTriggerExternalShutterOn" );
#endif

    return m_CamMode->IsTriggerExternalReadoutOn();
}

//////////////////////////// 
//  SET     SHUTTER      STATE
void ApogeeCam::SetShutterState( const Apg::ShutterState state )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetShutterState -> state = %d", state );
#endif

    switch( state )
    {
        case Apg::ShutterState_Normal:
            //for normal operations bits 11 and 12 are low
            m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_FORCE_SHUTTER_BIT) );

             m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_DISABLE_SHUTTER_BIT) );
         break;

        case Apg::ShutterState_ForceOpen:
            //force open bit 11 is HIGH and 12 is LOW
            m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
            CameraRegs::OP_A_FORCE_SHUTTER_BIT);

            m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_DISABLE_SHUTTER_BIT) );
        break;

        case Apg::ShutterState_ForceClosed:
            //force closed bit 11 is LOW and 12 is HIGH
            m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_FORCE_SHUTTER_BIT) );

            m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
            CameraRegs::OP_A_DISABLE_SHUTTER_BIT );
        break;

        default:
            apgHelper::throwRuntimeException( m_fileName, 
                "Invalid shutter state.", __LINE__, Apg::ErrorType_InvalidUsage );
        break;
    }
}

//////////////////////////// 
//  GET     SHUTTER      STATE     
Apg::ShutterState ApogeeCam::GetShutterState()
{

    const uint16_t val = ReadReg( CameraRegs::OP_A );
    const uint16_t mask = 0x1800;  //bits 11 and 12
    const uint16_t result =  val & mask;

    Apg::ShutterState state =  Apg::ShutterState_Unkown;
    switch( result )
    {
        case 0x0:
            state = Apg::ShutterState_Normal;
        break;

        case CameraRegs::OP_A_FORCE_SHUTTER_BIT:
        case (CameraRegs::OP_A_DISABLE_SHUTTER_BIT | CameraRegs::OP_A_FORCE_SHUTTER_BIT):
            state = Apg::ShutterState_ForceOpen;
        break;

        case CameraRegs::OP_A_DISABLE_SHUTTER_BIT:
            state = Apg::ShutterState_ForceClosed;
        break;

        default:
             apgHelper::throwRuntimeException(m_fileName, 
                 "Unknown shutter state.", __LINE__, Apg::ErrorType_InvalidUsage);
        break;
    }

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetShutterState -> state = %d, val = 0x%x, result = 0x%x", state, val, result );
#endif

    return state;
}


//////////////////////////// 
//  IS     SHUTTER      FORCED    OPEN
bool ApogeeCam::IsShutterForcedOpen()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsShutterForcedOpen" );
#endif

    return( GetShutterState() ==  Apg::ShutterState_ForceOpen ?  true : false );
}

//////////////////////////// 
//  IS     SHUTTER      FORCED    CLOSED
bool ApogeeCam::IsShutterForcedClosed()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsShutterForcedClosed" );
#endif

    return( GetShutterState() ==  Apg::ShutterState_ForceClosed ?  true : false );
}

//////////////////////////// 
//  IS     SHUTTER  OPEN
bool ApogeeCam::IsShutterOpen()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsShutterOpen" );
#endif

    CameraStatusRegs StatusObj = GetStatus();
    return( 
        (StatusObj.GetStatus() & CameraRegs::STATUS_SHUTTER_OPEN_BIT ) ?
        true : false );
}


//////////////////////////// 
//  SET     SHUTTER      AMP        CTRL      ON
void ApogeeCam::SetShutterAmpCtrl( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetShutterAmpCtrl -> TurnOn = %d", TurnOn );
#endif

    if( TurnOn )
    {
         m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
            CameraRegs::OP_A_SHUTTER_AMP_CONTROL_BIT);
    }
    else
    {
          m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,
            static_cast<uint16_t>(~CameraRegs::OP_A_SHUTTER_AMP_CONTROL_BIT) );
    }
}

//////////////////////////// 
//  IS     SHUTTER      AMP        CTRL      ON
bool ApogeeCam::IsShutterAmpCtrlOn()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsShutterAmpCtrlOn" );
#endif

    const uint16_t opA = ReadReg( CameraRegs::OP_A );

     return( 
        (opA & CameraRegs::OP_A_SHUTTER_AMP_CONTROL_BIT ) ?
        true : false );
}


//////////////////////////// 
//  SET   COOLER 
void ApogeeCam::SetCooler( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCooler -> TurnOn = %d", TurnOn );
#endif

    if( TurnOn )
    {
        WriteReg( CameraRegs::CMD_B,
            CameraRegs::CMD_B_RAMP_TO_SETPOINT_BIT );
    }
    else
    {
        WriteReg( CameraRegs::CMD_B,
            CameraRegs::CMD_B_RAMP_TO_AMBIENT_BIT );
    }
}

//////////////////////////// 
//  GET   COOLER   STATUS
Apg::CoolerStatus ApogeeCam::GetCoolerStatus()
{
    CameraStatusRegs StatusObj = GetStatus();
   
    //check if the cooler is even turned on
    if( (StatusObj.GetStatus() & CameraRegs::STATUS_TEMP_ACTIVE_BIT) != CameraRegs::STATUS_TEMP_ACTIVE_BIT )
    {
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCoolerStatus -> status = %d", Apg::CoolerStatus_Off );
#endif
        return Apg::CoolerStatus_Off;
    }

    //debug message at bottom of function

     const uint16_t TEMP_MASK =  
         CameraRegs::STATUS_TEMP_REVISION_BIT | 
         CameraRegs::STATUS_TEMP_AT_TEMP_BIT | 
         CameraRegs::STATUS_TEMP_ACTIVE_BIT | 
         CameraRegs::STATUS_TEMP_SUSPEND_ACK_BIT;

     const uint16_t tempStatus = StatusObj.GetStatus() & TEMP_MASK;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCoolerStatus -> status reg & temp mask = %x", tempStatus );
#endif

    Apg::CoolerStatus currentStatus = Apg::CoolerStatus_Off;
    switch( tempStatus )
    {
        case CameraRegs::STATUS_TEMP_REVISION_BIT:
        case (CameraRegs::STATUS_TEMP_ACTIVE_BIT | CameraRegs::STATUS_TEMP_REVISION_BIT):
        case (CameraRegs::STATUS_TEMP_AT_TEMP_BIT | CameraRegs::STATUS_TEMP_ACTIVE_BIT | CameraRegs::STATUS_TEMP_REVISION_BIT):
            currentStatus = Apg::CoolerStatus_Revision;
        break;

        case CameraRegs::STATUS_TEMP_AT_TEMP_BIT:
        case (CameraRegs::STATUS_TEMP_ACTIVE_BIT | CameraRegs::STATUS_TEMP_AT_TEMP_BIT):
            currentStatus = Apg::CoolerStatus_AtSetPoint;
        break;

        case CameraRegs::STATUS_TEMP_ACTIVE_BIT:
            currentStatus = Apg::CoolerStatus_RampingToSetPoint;
        break;

        //TODO ask wayne if the temp suspend bit can be high
        //with any other bits
        case CameraRegs::STATUS_TEMP_SUSPEND_ACK_BIT:
        case (CameraRegs::STATUS_TEMP_SUSPEND_ACK_BIT | CameraRegs::STATUS_TEMP_ACTIVE_BIT):
           currentStatus = Apg::CoolerStatus_Suspended;
        break;

        default:
           currentStatus = Apg::CoolerStatus_Off;
        break;
    }

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCoolerStatus -> status = %d", currentStatus );
#endif

    return currentStatus;
}

//////////////////////////// 
//  IS     COOLER    ON
bool ApogeeCam::IsCoolerOn()
{
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::IsCoolerOn" );
#endif

    const Apg::CoolerStatus cooler = GetCoolerStatus();

    return( cooler != Apg::CoolerStatus_Off ? true : false );
}


//////////////////////////// 
//  SUPSEND   COOLER
void ApogeeCam::SupsendCooler( bool & resume )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SupsendCooler" );
#endif

    if( IsCoolerOn() )
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,
            CameraRegs::OP_A_TEMP_SUSPEND_BIT );

        WaitForCoolerSuspendBit( 
            CameraRegs::STATUS_TEMP_SUSPEND_ACK_BIT, true );

        resume = true;
    }
}

//////////////////////////// 
//  RESUME  COOLER
void ApogeeCam::ResumeCooler()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::ResumeCooler" );
#endif

    // ticket #39 always allow this write
    m_CamIo->ReadAndWriteReg( CameraRegs::OP_A, 
    static_cast<uint16_t>(~CameraRegs::OP_A_TEMP_SUSPEND_BIT) );

    //wait for the status temp suspend bit to go low
    const uint16_t mask = 
        static_cast<uint16_t>(~CameraRegs::STATUS_TEMP_SUSPEND_ACK_BIT);
    WaitForCoolerSuspendBit(  mask, false );

}

//////////////////////////// 
//  WAIT        FOR      COOLER    SUSPEND      BIT
void ApogeeCam::WaitForCoolerSuspendBit( const uint16_t mask, const bool IsHigh )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::WaitForCoolerSuspendBit" );
#endif

    int32_t loopCount = 0;
    const int32_t NUM_LOOPS = 10;

    bool waiting = true;

    while( waiting )
    { 
        if( loopCount >= NUM_LOOPS )
        {
            //throw an error we have been waiting too long
            apgHelper::throwRuntimeException( m_fileName,
                "Waiting for temp suspension timed out.", __LINE__,
                Apg::ErrorType_Critical );
        }

        //pause and increment - try again
        apgHelper::ApogeeSleep( 100 );
        ++loopCount;

       uint16_t value = ReadReg( CameraRegs::STATUS );
       uint16_t result = 0;
       // loop until the desired bit goes high (the &) or low (the |)
       if( IsHigh )
       {
           result = ( value & mask );
       }
       else 
       {
           result = ( value | mask );
       }

       waiting = result != mask ? true : false;

    } // while
}


//////////////////////////// 
//  GET    TEMP      CCD
double ApogeeCam::GetTempCcd()
{
    double summedRegValues = 0;

    for( int i=0; i < NUM_TEMP_2_AVG; ++i )
    {
        CameraStatusRegs curStatus = GetStatus();
        summedRegValues += curStatus.GetTempCcd();
    }

    const double avgRegVal = summedRegValues / (double)NUM_TEMP_2_AVG;

    double result =   ( avgRegVal - m_CameraConsts->m_TempSetpointZeroPoint ) * 
                    m_CameraConsts->m_TempDegreesPerBit;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTempCcd -> temp = %f", result );
#endif

    return result;
}

//////////////////////////// 
//  DEFAULT GET    TEMP      HEATSINK
double ApogeeCam::DefaultGetTempHeatsink()
{
    double summedRegValues = 0;

    for( int i=0; i < NUM_TEMP_2_AVG; ++i )
    {
        CameraStatusRegs curStatus = GetStatus();
        summedRegValues += curStatus.GetTempHeatSink() & CameraRegs::MASK_TEMP_PARAMS;
    }

    const double avgRegVal = summedRegValues / (double)NUM_TEMP_2_AVG;

     const double temp = 
         ( avgRegVal - m_CameraConsts->m_TempHeatsinkZeroPoint ) * 
        m_CameraConsts->m_TempDegreesPerBit;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTempHeatsink -> temp = %f", temp );
#endif

     return temp;
}

//////////////////////////// 
// SET      ADC     RESOLUTION
void ApogeeCam::SetCcdAdcResolution(const Apg::Resolution res )
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCcdAdcResolution -> res = %d", res );
#endif

    m_CcdAcqSettings->SetResolution( res );
}

//////////////////////////// 
// GET      ADC     RESOLUTION
Apg::Resolution ApogeeCam::GetCcdAdcResolution()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCcdAdcResolution" );
#endif

    return m_CcdAcqSettings->GetResolution();
}

//////////////////////////// 
// SET      ADC     SPEED
void ApogeeCam::SetCcdAdcSpeed( const Apg::AdcSpeed speed )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCcdAdcSpeed -> speed = %d", speed );
#endif

    if( GetCcdAdcSpeed() == speed )
    {
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetCcdAdcSpeed speed already set, exiting function" );
#endif
        // exit here cannot hammer on the ascents cause image 
        // flicker in alta ticket #114
        return;
    }

    m_CcdAcqSettings->SetSpeed( speed );
}
  
//////////////////////////// 
// GET      ADC     SPEED
Apg::AdcSpeed ApogeeCam::GetCcdAdcSpeed()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetCcdAdcSpeed" );
#endif

    return  m_CcdAcqSettings->GetSpeed();
}

//////////////////////////// 
//  GET     MAX       BIN      COLS
uint16_t ApogeeCam::GetMaxBinCols()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMaxBinCols" );
#endif

    return m_CcdAcqSettings->GetMaxBinCols();
}

//////////////////////////// 
//  GET     MAX       BIN      ROWS
uint16_t ApogeeCam::GetMaxBinRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMaxBinRows" );
#endif
    return m_CcdAcqSettings->GetMaxBinRows(); 
}

//////////////////////////// 
//  GET     MAX       IMG     COLS
uint16_t ApogeeCam::GetMaxImgCols()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMaxImgCols" );
#endif

    return m_CamCfgData->m_MetaData.ImagingColumns;
}

//////////////////////////// 
//  GET     MAX       IMG      ROWS
uint16_t ApogeeCam::GetMaxImgRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMaxImgRows" );
#endif

    return m_CamCfgData->m_MetaData.ImagingRows;
}


//////////////////////////// 
//  GET    TOTAL    ROW
uint16_t ApogeeCam::GetTotalRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTotalRows" );
#endif

    return m_CamCfgData->m_MetaData.TotalRows;
}
        
//////////////////////////// 
//  GET    TOTAL    COLS
uint16_t ApogeeCam::GetTotalCols()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetTotalCols" );
#endif

    return m_CamCfgData->m_MetaData.TotalColumns;
}

//////////////////////////// 
//  GET    NUM        OVERSCAN       COLS
uint16_t ApogeeCam::GetNumOverscanCols()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetNumOverscanCols" );
#endif

    return m_CamCfgData->m_MetaData.OverscanColumns;
}

//////////////////////////// 
//  IS     INTERLINE
bool ApogeeCam::IsInterline()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsInterline" );
#endif

    return( m_CamCfgData->m_MetaData.InterlineCCD ? true : false );
}

//////////////////////////// 
//  SET     LED     A     STATE
 void ApogeeCam::SetLedAState( Apg::LedState state )
 {
 #ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::SetLedAState -> state = %d", state );
#endif

     uint16_t value = m_CamIo->ReadMirrorReg( CameraRegs::LED );
    
    //clear bits 0:2
     value &= ~CameraRegs::LED_A_MASK;
     
     uint16_t newState = static_cast<uint16_t>(state);

     //or in the new state into bits 0:2
     value |= newState;

     //write the new value to the camera
     WriteReg( CameraRegs::LED, value );
 }
 
//////////////////////////// 
//  GET     LED     A     STATE
 Apg::LedState ApogeeCam::GetLedAState()
 {
 #ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::GetLedAState" );
#endif

     const uint16_t value = m_CamIo->ReadMirrorReg( CameraRegs::LED ) & CameraRegs::LED_A_MASK;
     return apgHelper::ConvertUShort2ApnLedState( value );
 }
 
//////////////////////////// 
//  SET     LED     B     STATE
 void ApogeeCam::SetLedBState( Apg::LedState state )
 {
 #ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::SetLedBState -> state = %d", state );
#endif

     uint16_t value = m_CamIo->ReadMirrorReg( CameraRegs::LED );
    
     //clear bits  4:6
     value &= ~CameraRegs::LED_B_MASK;

      //set the lower 3 bits to zero and keep bits 4:6 led B values     value &= CameraRegs::LED_B_MASK;

     uint16_t newState = ( static_cast<uint16_t>(state) << CameraRegs::LED_BIT_SHIFT);

    //or in the new state into bits 4:6
     value |= newState;

     //write the new value to the camera
     WriteReg( CameraRegs::LED, value );
 }

//////////////////////////// 
//  GET     LED     B     STATE
Apg::LedState ApogeeCam::GetLedBState()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetLedBState" );
#endif

    const uint16_t value = (m_CamIo->ReadMirrorReg( CameraRegs::LED ) & CameraRegs::LED_B_MASK ) >> CameraRegs::LED_BIT_SHIFT;
    return apgHelper::ConvertUShort2ApnLedState( value );
}

//////////////////////////// 
//  SET     LED     MODE
void ApogeeCam::SetLedMode( const Apg::LedMode mode )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetLedMode -> mode = %d", mode );
#endif

    uint16_t value = ReadReg( CameraRegs::OP_A );

    switch ( mode )
    {
        case Apg::LedMode_DisableAll:
	        value |= CameraRegs::OP_A_LED_DISABLE_BIT;
            value &= ~CameraRegs::OP_A_LED_EXPOSE_DISABLE_BIT;
	    break;
        
        case Apg::LedMode_DisableWhileExpose:
	        value &= ~CameraRegs::OP_A_LED_DISABLE_BIT;
	        value |= CameraRegs::OP_A_LED_EXPOSE_DISABLE_BIT;
	    break;

        case Apg::LedMode_EnableAll:
	        value &= ~CameraRegs::OP_A_LED_DISABLE_BIT;
	        value &= ~CameraRegs::OP_A_LED_EXPOSE_DISABLE_BIT;
	    break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid led mode: " <<  mode;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    WriteReg( CameraRegs::OP_A, value );

}

//////////////////////////// 
//  GET     LED     MODE
Apg::LedMode ApogeeCam::GetLedMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetLedMode" );
#endif

    const uint16_t ledMask = ( CameraRegs::OP_A_LED_DISABLE_BIT |
            CameraRegs::OP_A_LED_EXPOSE_DISABLE_BIT);
    const uint16_t value = (ReadReg( CameraRegs::OP_A ) & ledMask);

    Apg::LedMode result = Apg::LedMode_DisableAll;

    switch( value )
    {
        case CameraRegs::OP_A_LED_DISABLE_BIT:
            result = Apg::LedMode_DisableAll;
        break;

        case CameraRegs::OP_A_LED_EXPOSE_DISABLE_BIT:
            result = Apg::LedMode_DisableWhileExpose;
        break;

        default:
            result = Apg::LedMode_EnableAll;
        break;
    }

    return result;
}

//////////////////////////// 
//  GET    INFO
std::string ApogeeCam::GetInfo()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetInfo" );
#endif

    std::string info;

    info.append( "Model: " + GetModel() + "\n" );
    info.append( "Sensor: " + GetSensor() + "\n" );
    
    std::stringstream libapgStr;
    libapgStr << "libapogee: " << APOGEE_MAJOR_VERSION << ".";
    libapgStr << APOGEE_MINOR_VERSION << ".";
    libapgStr << APOGEE_PATCH_VERSION << "\n";
    info.append( libapgStr.str() );
  
    info.append( m_CamIo->GetInfo() );

    return info;
}

//////////////////////////// 
//      GET  MODEL
std::string ApogeeCam::GetModel()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetModel" );
#endif

    std::string result("Unknown");

    if( 0 != m_CamCfgData )
    {
        result = m_CamCfgData->m_MetaData.CameraLine;

        if( CamModel::ALTAE == m_PlatformType )
        {
            result.append( "E" );
        }

        if( CamModel::ALTAU == m_PlatformType )
        {
            result.append( "U" );
        }
        result.append( "-" );
        result.append( m_CamCfgData->m_MetaData.CameraModel );
    }

    return result;
}

//////////////////////////// 
//      GET    SENSOR
std::string ApogeeCam::GetSensor()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetSensor" );
#endif

    if( 0 == m_CamCfgData )
    {
        std::string result("No Sensor");
        return result;
    }
    else
    {
        return m_CamCfgData->m_MetaData.Sensor;
    }
}

//////////////////////////// 
//  SET     FLUSH        COMMANDS
void ApogeeCam::SetFlushCommands( const bool Disable )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetFlushCommands -> Disable = %d", Disable );
#endif

    std::string msg;
    if( Disable )
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
            CameraRegs::OP_B_DISABLE_FLUSH_COMMANDS_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
            static_cast<uint16_t>(~CameraRegs::OP_B_DISABLE_FLUSH_COMMANDS_BIT) );
    }
}
  
//////////////////////////// 
//  ARE    FLUSH    CMDS     DISABLED
bool ApogeeCam::AreFlushCmdsDisabled()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::AreFlushCmdsDisabled" );
#endif

    return ( (ReadReg(CameraRegs::OP_B) & CameraRegs::OP_B_DISABLE_FLUSH_COMMANDS_BIT) ?
        true : false );
}

//////////////////////////// 
//  SET POST EXPOSE FLUSHING
void ApogeeCam::SetPostExposeFlushing( const bool Disable )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetPostExposeFlushing -> Disable = %d", Disable );
#endif

    if( Disable )
    {
         m_CamIo->ReadOrWriteReg( CameraRegs::OP_B,
             CameraRegs::OP_B_DISABLE_POST_EXP_FLUSH_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_B,
            static_cast<uint16_t>(~CameraRegs::OP_B_DISABLE_POST_EXP_FLUSH_BIT) );
    }
}

//////////////////////////// 
//  IS     POST  EXPOSE        FLUSHING      DISABLED
bool ApogeeCam::IsPostExposeFlushingDisabled()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsPostExposeFlushingDisabled" );
#endif

     return ( (ReadReg(CameraRegs::OP_B) & CameraRegs::OP_B_DISABLE_POST_EXP_FLUSH_BIT) ?
        true : false );
}


//////////////////////////// 
//      GET     SERIAL       NUMBER
std::string ApogeeCam::GetSerialNumber( )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetSerialNumber" );
#endif

    return m_CamIo->GetSerialNumber();
}

//////////////////////////// 
//      STOP  EXPOSURE   MODE     NORM
void ApogeeCam::StopExposureModeNorm( const bool Digitize )
{
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::StopExposureModeNorm -> Digitize = %d", Digitize );
#endif

     //pre-conditions
     if( Apg::CameraMode_Normal != GetCameraMode() )
     {
         // we are not in the right mode throw an exception
        apgHelper::throwRuntimeException(m_fileName,
            "Error: StopExposureModeNorm camera is not in normal mode", 
             __LINE__, Apg::ErrorType_InvalidMode );
     }

    if( !m_ImageInProgress )
    {
        // we are not taking a picture so log a warning
        // and exit, ticket #42
         std::string vinfo = apgHelper::mkMsg( m_fileName, 
             "Exposure not in progress, thus exiting out of function without performing any operations", 
             __LINE__);
         ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        // if the user is expecting data we should throw, so they know
        // there is nothing to fetch
        if( Digitize )
        {
             apgHelper::throwRuntimeException(m_fileName,
                "Error exposure never started, thus no image to digitize", 
            __LINE__, Apg::ErrorType_InvalidMode );
        }

        //otherwise just exit
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::StopExposureModeNorm 0 END" );
#endif
        return;
    }

    //tell the fpga to stop imaging
    WriteReg(CameraRegs::CMD_B,
        CameraRegs::CMD_B_END_EXPOSURE_BIT );

    //if we are waiting on a trigger, we cannot save the
    //data, we MUST force a reset of the camera
    // and exit function
    if( Apg::Status_WaitingOnTrigger == GetImagingStatus() )
    {
        HardStopExposure( "Stopping exposure while waiting for external triggers" );

        #ifdef DEBUGGING_CAMERA
            apgHelper::DebugMsg( "ApogeeCam::StopExposureModeNorm 2 END" );
        #endif
        return;
    }
  
    //check if we are in image sequence mode
    //if so, do another hard stop
    if( GetImageCount() > 1)
     {
        HardStopExposure( "Hard stop of an exposure of image sequences" );

        #ifdef DEBUGGING_CAMERA
            apgHelper::DebugMsg( "ApogeeCam::StopExposureModeNorm 3 END" );
        #endif
        return;
    }

    if( !Digitize )
    {
        GrabImageAndThrowItAway();
    }

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::StopExposureModeNorm 4 END" );
#endif

 }

//////////////////////////// 
//      HARD      STOP     EXPOSURE
void ApogeeCam::HardStopExposure( const std::string & msg )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::HardStopExposure -> msg = %s", msg.c_str() );
#endif

    //log the hard stop
    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info",
        apgHelper::mkMsg( m_fileName, msg, __LINE__) );

    Reset( true );
    m_CamIo->CancelImgXfer();
    m_ImageInProgress = false;
      
}

//////////////////////////// 
//  GRAB      IMAGE        AND        THROW      IT    AWAY
void ApogeeCam::GrabImageAndThrowItAway()
{
#ifdef DEBUGGING_CAMERA
     apgHelper::DebugMsg( "ApogeeCam::GrabImageAndThrowItAway" );
#endif

    //the user doesn't want the data so we will wait for the camera
    //to finish and then fetch the data 
     //wait for the image
    int32_t WaitCounter = 0;
    const int32_t NUM_WAITS = 1000;
    const uint32_t SLEEP_IN_MSEC = 100;
	m_LastExposureTime = 0; // for aspen ethernet issue
    while( GetImagingStatus() != Apg::Status_ImageReady )
    {
        apgHelper::ApogeeSleep( SLEEP_IN_MSEC );

        ++WaitCounter;
    	
        if ( WaitCounter >= NUM_WAITS  )
        {
            // we have waited to long for the image data
            // throw here to break an inifinite loop
            std::stringstream ss;
            ss << "Stop image no digitize failed.  ";
            ss << "Camera has not freed image data in ";
            ss << ((SLEEP_IN_MSEC*0.001)*NUM_WAITS ) << " seconds.";
            apgHelper::throwRuntimeException(m_fileName, ss.str(), 
                __LINE__, Apg::ErrorType_Critical);
        }
    }

    //grab the data off the camera, so it is clear for the next image
    //the user doesn't want it so just leave it here to be destoryed on exit
    std::vector<uint16_t> data;
    GetImage( data );
}

//////////////////////////// 
//      GET    PIXEL      WIDTH
double ApogeeCam::GetPixelWidth()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetPixelWidth" );
#endif

    return m_CamCfgData->m_MetaData.PixelSizeX;
}

//////////////////////////// 
//      GET    PIXEL      HEIGHT
double ApogeeCam::GetPixelHeight()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetPixelHeight" );
#endif

    return m_CamCfgData->m_MetaData.PixelSizeY;
}

//////////////////////////// 
//      GET    MIN      EXPOSURE       TIME
double ApogeeCam::GetMinExposureTime()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMinExposureTime" );
#endif

    return m_CameraConsts->m_ExposureTimeMin;
}

//////////////////////////// 
//      GET    MAX      EXPOSURE       TIME
double ApogeeCam::GetMaxExposureTime()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetMaxExposureTime" );
#endif

    return m_CameraConsts->m_ExposureTimeMax;
}

//////////////////////////// 
//      IS     COLOR
bool ApogeeCam::IsColor()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsColor" );
#endif

    return m_CamCfgData->m_MetaData.Color;
}

//////////////////////////// 
//  IS     COOLING       SUPPORTED
bool ApogeeCam::IsCoolingSupported()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsCoolingSupported" );
#endif

    return m_CamCfgData->m_MetaData.CoolingSupported;
}

//////////////////////////// 
//  IS     COOLING           REGULATED
bool ApogeeCam::IsCoolingRegulated()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsCoolingRegulated" );
#endif

    return m_CamCfgData->m_MetaData.RegulatedCoolingSupported;
}

//////////////////////////// 
//  GET    INPUT     VOLTAGE
double ApogeeCam::GetInputVoltage()
{
    CameraStatusRegs StatusObj = GetStatus();

    const double voltage = ( StatusObj.GetInputVoltage() & CameraRegs::INPUT_VOLTAGE_MASK	) 
        * m_CameraConsts->m_VoltageResolution;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetInputVoltage -> voltage = %f", voltage );
#endif

    return voltage;
}

//////////////////////////// 
//      GET    INTERFACE    TYPE
CamModel::InterfaceType ApogeeCam::GetInterfaceType()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetInterfaceType" );
#endif
    return m_CamIo->GetInterfaceType();
}


//////////////////////////// 
//      GET    USB     VENDOR        INFO
void ApogeeCam::GetUsbVendorInfo( uint16_t & VendorId,
            uint16_t & ProductId, uint16_t  & DeviceId)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetUsbVendorInfo" );
#endif

    m_CamIo->GetUsbVendorInfo( VendorId, ProductId, DeviceId );
}

//////////////////////////// 
//      IS         CCD
bool ApogeeCam::IsCCD()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsCCD" );
#endif

    return m_CamCfgData->m_MetaData.SensorTypeCCD;
}


//////////////////////////// 
//          PAUSE    TIMER
void ApogeeCam::PauseTimer( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::PauseTimer -> TurnOn = %d", TurnOn );
#endif

    if( TurnOn )
    {
        m_CamIo->ReadOrWriteReg( CameraRegs::OP_A,	
        CameraRegs::OP_A_PAUSE_TIMER_BIT );
    }
    else
    {
        m_CamIo->ReadAndWriteReg( CameraRegs::OP_A,	
            static_cast<uint16_t>( (~CameraRegs::OP_A_PAUSE_TIMER_BIT) ) );
    }

}

//////////////////////////// 
//      IS     SERIAL   A     SUPPORTED
bool ApogeeCam::IsSerialASupported()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsSerialASupported" );
#endif

    return m_CamCfgData->m_MetaData.SupportsSerialA;
}

//////////////////////////// 
//      IS     SERIAL   B     SUPPORTED
bool ApogeeCam::IsSerialBSupported()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsSerialBSupported" );
#endif

    return m_CamCfgData->m_MetaData.SupportsSerialB;
}

//////////////////////////// 
//          SET         FLUSH        BINNING     ROWS
void ApogeeCam::SetFlushBinningRows( const uint16_t bin )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetFlushBinningRows -> bin = %d", bin );
#endif

    uint16_t actualBin = bin;

	// Do some bounds checking on our input parameter
	if ( 0 == bin ) 
    {
        std::stringstream msg;
        msg << "Changing input flush binning rows from " << bin;
        msg << " to " << 1;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

		actualBin = 1;
    }
	
    if(  GetMaxBinRows() < bin ) 
    {
        std::stringstream msg;
        msg << "Changing input flush binning rows from " << bin;
        msg << " to " << GetMaxBinRows();
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        // TODO: determine how video mode, which will result in a
        // v bin max of 1 affects this parameter.  in other words
        // if the current guard is ok.
		actualBin = GetMaxBinRows();
    }
	
	Reset( false );
    WriteReg( CameraRegs::VFLUSH_BINNING, actualBin );
	Reset( true );

}

//////////////////////////// 
//          GET         FLUSH        BINNING     ROWS
uint16_t ApogeeCam::GetFlushBinningRows()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetFlushBinningRows" );
#endif

    return ( m_CamIo->ReadMirrorReg( CameraRegs::VFLUSH_BINNING ) );
}

//////////////////////////// 
//   IS            OVERSCAN           DIGITIZED
bool ApogeeCam::IsOverscanDigitized()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsOverscanDigitized" );
#endif

    return m_CcdAcqSettings->IsOverscanDigitized();
}

//////////////////////////// 
//      SET         DIGITIZE        OVERSCAN
void ApogeeCam::SetDigitizeOverscan( const bool TurnOn )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetDigitizeOverscan -> TurnOn = %d", TurnOn );
#endif

    m_CcdAcqSettings->SetDigitizeOverscan( TurnOn );
}

//////////////////////////// 
//      SET     ADC        GAIN 
void ApogeeCam::SetAdcGain( const uint16_t gain, const int32_t ad, 
                           const int32_t channel )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetAdcGain -> gain = %d, ad = %d, channel = %d", 
        gain, ad, channel );
#endif

    //pre-condition check, throws on failure
   AdcParamCheck( ad, channel, "ApogeeCam::SetAdcGain" );

    m_CcdAcqSettings->SetAdcGain( gain, ad, channel );
}

//////////////////////////// 
//      GET     ADC        GAIN 
uint16_t ApogeeCam::GetAdcGain( int32_t ad, int32_t channel )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetAdcGain" );
#endif

    return m_CcdAcqSettings->GetAdcGain( ad, channel );
}

//////////////////////////// 
//      SET     ADC        OFFSET
void ApogeeCam::SetAdcOffset( uint16_t offset, int32_t ad, int32_t channel )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetAdcOffset -> offset = %d, ad = %d, channel = %d", 
        offset, ad, channel );
#endif

   //pre-condition check, throws on failure
   AdcParamCheck( ad, channel, "ApogeeCam::SetAdcOffset" );

    m_CcdAcqSettings->SetAdcOffset( offset, ad, channel );
}

//////////////////////////// 
//      GET     ADC        OFFSET
uint16_t ApogeeCam::GetAdcOffset( int32_t ad, int32_t channel )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetAdcOffset" );
#endif
    return m_CcdAcqSettings->GetAdcOffset( ad, channel );
}

//////////////////////////// 
//      ADC    PARAM       CHECK
void ApogeeCam::AdcParamCheck( const int32_t ad, 
                              const int32_t channel, const std::string & fxName )
{
    //minus one because counting starts a zero for adc and channel values
    if( ad > GetNumAds()-1  || ad < 0 )
    {
        std::stringstream ss;
        ss << "Invalid adc number, " << ad << ", passed to function = " << fxName;
        apgHelper::throwRuntimeException( m_fileName, ss.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    if( channel > GetNumAdChannels()-1 || channel < 0 )
    {
         std::stringstream ss;
        ss << "Invalid adc channel, " << ad << ", passed to function = " << fxName;
        apgHelper::throwRuntimeException( m_fileName, ss.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }
    
}

//////////////////////////// 
//      GET    FIRMWARE    REV
uint16_t ApogeeCam::GetFirmwareRev() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetFirmwareRev -> rev = %d", m_FirmwareVersion );
#endif
    return m_FirmwareVersion; 
}

//////////////////////////// 
//      GET    PLATFORM    TYPE
CamModel::PlatformType ApogeeCam::GetPlatformType() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetPlatformType -> type = %d", m_PlatformType );
#endif
    return m_PlatformType; 
}


//////////////////////////// 
//      SET     AD       SIM     MODE
void ApogeeCam::SetAdSimMode( const bool TurnOn )
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetAdSimMode -> turnOn = %d", TurnOn );
#endif

    m_CcdAcqSettings->SetAdsSimMode( TurnOn );
}

//////////////////////////// 
//     IS     AD       SIM     MODE   ON
bool ApogeeCam::IsAdSimModeOn()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::IsAdSimModeOn" );
#endif

    return m_CcdAcqSettings->IsAdsSimModeOn();
}

//////////////////////////// 
//      SET         LED         BRIGHTNESS
void ApogeeCam::SetLedBrightness( double PercentIntensity )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::SetLedBrightness -> PercentIntensity = %f", PercentIntensity );
#endif

    if( apgHelper::IsEqual( PercentIntensity, GetLedBrightness() ) )
    {
        //no change is required
        return;
    }

    //pausing cooler, i believe to get access to the DAC
    bool RestartCooler = false;
    SupsendCooler( RestartCooler );

    //calc reg value and write it to the led drive reg
    uint16_t value = static_cast<uint16_t>( 
        GetIlluminationMask() * (PercentIntensity/100.0) );

    WriteReg( CameraRegs::LED_DRIVE, value );

    // restarting the cooler if neccessary
    if( RestartCooler )
    {
        ResumeCooler();
    }
}

//////////////////////////// 
//      GET         LED         BRIGHTNESS
double ApogeeCam::GetLedBrightness()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetLedBrightness" );
#endif

    return( 100.0 * static_cast<double>(
        m_CamIo->ReadMirrorReg( CameraRegs::LED_DRIVE ) / GetIlluminationMask() ) );
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string ApogeeCam::GetDriverVersion()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetDriverVersion" );
#endif

    return m_CamIo->GetDriverVersion();
}

//////////////////////////// 
// GET      USB        FIRMWARE        VERSION
std::string ApogeeCam::GetUsbFirmwareVersion()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::GetUsbFirmwareVersion" );
#endif

    return m_CamIo->GetUsbFirmwareVersion();
}

//////////////////////////// 
//      CHECK       AND    WAIT       FOR     STATUS
bool ApogeeCam::CheckAndWaitForStatus( const Apg::Status desired, Apg::Status & acutal )
{
    // giving the camera a little time to get into the desired mode
    // found this code is needed while running the irregularRoi.py test
    // script.  especially rapidly going from getimage to startexposure
    // the camera will be in image_active mode.  giving it a couple of cycles
    // it gets into flushing mode.  every now and again the camera will
    // go from image_ready to image_active as well.  
    const uint32_t MAX_WAIT_COUNT = 300;
    const uint32_t SLEEP_IN_MSEC = 10;
    uint32_t waitCount = 0;
    acutal  = GetImagingStatus();
    while( desired != acutal  )
    {
        if( waitCount < MAX_WAIT_COUNT )
        {
            ++waitCount;
            apgHelper::ApogeeSleep( SLEEP_IN_MSEC );
            acutal = GetImagingStatus();
        }
        else
        {
            return false;
        }
    }

    return true;
}

//////////////////////////// 
//      CANCEL     EXPOSURE    NO      THROW
void ApogeeCam::CancelExposureNoThrow()
{

    if( m_ImageInProgress )
    {
        try
        {
            HardStopExposure(  "Stopping exposure in CancelExposureNoThrow()" );
        }
        catch( std::exception & err )
        {
            std::string msg ("Exception caught stopping exposure in CancelExposureNoThrow() msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~ApogeeCam()" );
        }
    }
}

//////////////////////////// 
//      DEFAULT    SET      FAN       MODE
void ApogeeCam::DefaultSetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "DefaultSetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif

    if( PreCondCheck )
    {
        if( GetFanMode() == mode )
        {
            //exit we are already in the fan mode
            return;
        }
    }

    uint16_t RegVal = 0;
    switch ( mode )
    {
        case Apg::FanMode_Off:
            RegVal = m_CameraConsts->m_FanSpeedOff;
        break;
        
        case Apg::FanMode_Low:
            RegVal = m_CameraConsts->m_FanSpeedLow;
        break;
        
        case Apg::FanMode_Medium:
            RegVal = m_CameraConsts->m_FanSpeedMedium;
        break;

        case Apg::FanMode_High:
            RegVal = m_CameraConsts->m_FanSpeedHigh;
        break;

        default:
        {
            std::stringstream msg;
            msg << "Invalid fan mode: " << mode;
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
        }
        break;
    }

    bool resumeCooling = false;

    //disable cooler to access the fan dac
    SupsendCooler( resumeCooling );

    WriteReg( CameraRegs::FAN_SPEED_CONTROL, RegVal );

     uint16_t opBVal = ReadReg( CameraRegs::OP_B );

     opBVal |= CameraRegs::OP_B_DAC_SELECT_ZERO_BIT;
     opBVal &= ~CameraRegs::OP_B_DAC_SELECT_ONE_BIT;
     
     WriteReg( CameraRegs::OP_B, opBVal );

     WriteReg( CameraRegs::CMD_B,
         CameraRegs::CMD_B_DAC_LOAD_BIT );

    if ( resumeCooling )
    {
        ResumeCooler();
    }

}

//////////////////////////// 
//      DEFAULT   GET      FAN       MODE
Apg::FanMode ApogeeCam::DefaultGetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::DefaultGetFanMode" );
#endif

    const uint16_t val = m_CamIo->ReadMirrorReg(
        CameraRegs::FAN_SPEED_CONTROL);

    if( val == m_CameraConsts->m_FanSpeedOff)
    {
            return Apg::FanMode_Off;
    }
    else if( val == m_CameraConsts->m_FanSpeedLow)
    {
        return Apg::FanMode_Low;
    }
    else if ( val == m_CameraConsts->m_FanSpeedMedium)
    {
        return Apg::FanMode_Medium;
    }
    else if( val == m_CameraConsts->m_FanSpeedHigh)
    {
        return Apg::FanMode_High;
    }
    else
    {
        std::stringstream msg;
        msg << "Unknow fan DAC value: " << val;
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }
    
    //fall through - should never reach here
    return Apg::FanMode_Off;
}


CamInfo::StrDb ApogeeCam::ReadStrDatabase()
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::ReadStrDatabase" );
#endif
    if ( (m_PlatformType == CamModel::ASCENT) || (m_PlatformType == CamModel::ALTAF) )
    {
        return std::dynamic_pointer_cast<AscentBasedIo>(m_CamIo)->ReadStrDatabase();
    }
    else
    {
        return std::dynamic_pointer_cast<AspenIo>(m_CamIo)->ReadStrDatabase();
    }
}

void ApogeeCam::WriteStrDatabase(CamInfo::StrDb &info)
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "ApogeeCam::WriteStrDatabase" );
#endif
    if ( (m_PlatformType == CamModel::ASCENT) || (m_PlatformType == CamModel::ALTAF) )
    {
        std::dynamic_pointer_cast<AscentBasedIo>(m_CamIo)->WriteStrDatabase(info);
    }
    else
    {
        std::dynamic_pointer_cast<AspenIo>(m_CamIo)->WriteStrDatabase(info);
    }
}


void ApogeeCam::UpdateAlta(const std::string FilenameCamCon, const std::string FilenameBufCon, const std::string FilenameFx2, const std::string FilenameGpifCamCon, const std::string FilenameGpifBufCon, const std::string FilenameGpifFifo)
{
	if ((CamModel::ALTAE != m_PlatformType) && (CamModel::ALTAU != m_PlatformType))
    {
        return;
    }

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->Program(
                    FilenameCamCon,
                    FilenameBufCon, 
                    FilenameFx2, 
                    FilenameGpifCamCon,
                    FilenameGpifBufCon, 
                    FilenameGpifFifo, 
                    false);
}


void ApogeeCam::UpdateAscentOrAltaF(const std::string FilenameFpga, const std::string FilenameFx2, const std::string FilenameDescriptor)
{
	if ((CamModel::ASCENT != m_PlatformType) && (CamModel::ALTAF != m_PlatformType))
    {
        return;
    }

    std::dynamic_pointer_cast<AscentBasedIo>(m_CamIo)->Program(
                    FilenameFpga,
                    FilenameFx2, 
                    FilenameDescriptor, 
                    false);
}

void ApogeeCam::UpdateAspen(const std::string FilenameFpga, const std::string FilenameFx2, const std::string FilenameDescriptor, const std::string FilenameWebPage, const std::string FilenameWebServer, const std::string FilenameWebCfg)
{
	if (CamModel::ASPEN != m_PlatformType)
    {
        return;
    }

    std::dynamic_pointer_cast<AspenIo>(m_CamIo)->Program(
                    FilenameFpga, 
                    FilenameFx2, 
                    FilenameDescriptor, 
                    FilenameWebPage, 
                    FilenameWebServer, 
                    FilenameWebCfg, 
                    false);
}

// END     OF       CLASS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
