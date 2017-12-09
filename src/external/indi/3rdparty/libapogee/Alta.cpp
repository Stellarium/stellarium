/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class Alta 
* \brief Derived class for the alta apogee cameras 
* 
*/ 

#include "Alta.h" 
#include "AltaIo.h" 
#include "apgHelper.h" 
#include "AltaData.h" 
#include "CamHelpers.h" 
#include "AltaModeFsm.h" 
#include "ApgLogger.h" 
#include "AltaCcdAcqParams.h" 
#include "ApnCamData.h"
#include "helpers.h"
#include "PlatformData.h" 
#include "ImgFix.h" 

#include <sstream>
#include <cstring>  //for memset

namespace
{
    const uint16_t MIN_FIRMWARE_REV_4_ADVANCED_STATUS = 16; 
    const uint32_t ALTA_U_MEMORY = 32 * 1024;
    const uint32_t ALTA_E_MEMORY = 28 * 1024;
    
    const uint16_t SERIAL_PORT_A = 0;
    const uint16_t SERIAL_PORT_B = 1;
    const uint32_t DEFAULT_SERIAL_PORT_BAUD = 9600;
    const Apg::SerialParity DEFAULT_SERIAL_PORT_PARITY = Apg::SerialParity_None;
    const Apg::SerialFC DEFAULT_SERIAL_PORT_CF = Apg::SerialFC_Off;

}

//////////////////////////// 
// CTOR 
Alta::Alta() : ApogeeCam(CamModel::ALTAU),
                m_fileName( __FILE__ )
{ 
     //alloc and set the camera constants
    m_CameraConsts = std::shared_ptr<PlatformData>( new AltaData() );

    m_serialPortOpenStatus[SERIAL_PORT_A] = false;
    m_serialPortOpenStatus[SERIAL_PORT_B] = false;
}

//////////////////////////// 
// DTOR 
Alta::~Alta() 
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
            std::string msg ("Exception caught in ~Alta msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~Alta" );
        }
    }

     //log that we are deleting object
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
        "Deleting Alta object"); 
} 

//////////////////////////// 
// OPEN     CONNECTION
void Alta::OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::OpenConnection -> ioType= %s, DeviceAddr = %s, FW = %d, ID = %d", 
        ioType.c_str(), DeviceAddr.c_str(), FirmwareRev, Id  );
#endif

    CreateCamIo( ioType, DeviceAddr );

     // save the input data
    m_FirmwareVersion =  FirmwareRev;
    m_Id = Id;

    if( CamModel::ETHERNET == m_CamIo->GetInterfaceType() )
    {
        m_PlatformType = CamModel::ALTAE;

        // because the ethernet discover message does not 
        // contain the correct firmware version, we are going 
        // to fetch it and store here....kinda silly, but it make this 
        // interface backwards compatible
        m_FirmwareVersion = m_CamIo->GetFirmwareRev();
    }

    //make sure the input id and firmware rev
    //match the camera we just connected to
    VerifyFrmwrRev();
    VerifyCamId();

    //create the ccd specific object
    CfgCamFromId( m_Id );

    //set up the camera mode fsm
    m_CamMode = std::shared_ptr<ModeFsm>( new AltaModeFsm(m_CamIo,
        m_CamCfgData, m_FirmwareVersion) );

  
    //create the adc and pattern file handler object
    m_CcdAcqSettings = std::shared_ptr<CcdAcqParams>(
        new AltaCcdAcqParams(m_CamCfgData, m_CamIo, m_CameraConsts) );

    m_IsConnected = true;
    LogConnectAndDisconnect( true );
}

//////////////////////////// 
//      CLOSE        CONNECTION
void Alta::CloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::DefaultCloseConnection");
#endif

    // need to clear out the usb interface if an image ready 
    // on altau's
    if( m_ImageInProgress )
    {
       StopExposure( false );
    }

    DefaultCloseConnection();
}

//////////////////////////// 
// CREATE      CAMIO        AND    CONSTS 
void Alta::CreateCamIo(const std::string & ioType,
     const std::string & DeviceAddr)
{
    //create the camera interface
    const CamModel::InterfaceType type = InterfaceHelper::DetermineInterfaceType( ioType );

    m_CamIo = std::shared_ptr<CameraIo>( new AltaIo( type, DeviceAddr ) );

    if( !m_CamIo )
    {
        std::string errStr("failed to create a camera interface io object");
        apgHelper::throwRuntimeException( m_fileName, errStr, 
            __LINE__, Apg::ErrorType_Connection );
    }

}

//////////////////////////// 
// VERIFY        CAM        ID 
void Alta::VerifyCamId()
{
    const uint16_t id = m_CamIo->GetId() & CamModel::ALTA_CAMERA_ID_MASK;

    if( id != m_Id )
    {
        std::string errStr = "id rev mis-match expected id =" + 
            help::uShort2Str(m_Id) + " received from camera id = " +
            help::uShort2Str(id);
        apgHelper::throwRuntimeException( m_fileName, errStr, 
            __LINE__, Apg::ErrorType_Connection );
    }

}

//////////////////////////// 
// CFG      CAM       FROM  ID
void Alta::CfgCamFromId( const uint16_t CameraId )
{
    //verify that this is an alta id
    if( !CamModel::IsAlta( m_FirmwareVersion ) )
    {
        std::stringstream ss;
        ss << "Invalid firmware version, " << m_FirmwareVersion << ", for Alta's." << std::endl;
        apgHelper::throwRuntimeException( m_fileName, ss.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //create and set the camera's cfg data
    DefaultCfgCamFromId( CameraId );
}

//////////////////////////// 
// GET       MAC      ADDRESS
std::string Alta::GetMacAddress( )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetMacAddress" );
#endif

    return std::dynamic_pointer_cast<AltaIo>(m_CamIo)->GetMacAddress();
}

//////////////////////////// 
// GET    STATUS
CameraStatusRegs Alta::GetStatus()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetStatus" );
#endif

    CameraStatusRegs status;

    if( MIN_FIRMWARE_REV_4_ADVANCED_STATUS > m_FirmwareVersion )
    {
        CameraStatusRegs::BasicStatus basic;
        memset( &basic,0,sizeof(basic) );
        m_CamIo->GetStatus( basic );

        status.Update( basic );  
    }
    else
    {
        CameraStatusRegs::AdvStatus adv;
        memset(&adv,0,sizeof(adv));
        m_CamIo->GetStatus(adv);

        status.Update( adv );
    }

    #ifdef DEBUGGING_CAMERA_STATUS
        apgHelper::DebugMsg( "Alta::GetStatus - CoolerDrive = 0x%X", status.GetCoolerDrive() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetCurrentFrame = 0x%X", status.GetCurrentFrame() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetDataAvailFlag = 0x%X", status.GetDataAvailFlag() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetFetchCount = 0x%X", status.GetFetchCount() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetInputVoltage = 0x%X", status.GetInputVoltage() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetMostRecentFrame = 0x%X", status.GetMostRecentFrame() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetReadyFrame = 0x%X", status.GetReadyFrame() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetSequenceCounter = 0x%X", status.GetSequenceCounter() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetStatus = 0x%X", status.GetStatus() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetTdiCounter = 0x%X", status.GetTdiCounter() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetTempCcd = 0x%X", status.GetTempCcd() );
        apgHelper::DebugMsg( "Alta::GetStatus - GetuFrame = 0x%X", status.GetuFrame() );
    #endif

    return status;
}

//////////////////////////// 
// START EXPOSURE  
void Alta::StartExposure( double Duration, bool IsLight )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::StartExposure BEGINNING -> Duration = %f, IsLight = %d",
         Duration, IsLight );
#endif

    Apg::Status actualStatus = Apg::Status_Idle;

    if( !CheckAndWaitForStatus( Apg::Status_Flushing, actualStatus ) )
    {
        std::stringstream msg;
        msg << "Invalid image status, " << actualStatus << ", for starting an exposure";
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidMode );
    }

    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Starting Exposure");

    //tell the camera interface how large of an image
    //and what transfer mode to use
    uint16_t r=0, c = 0;
    ExposureAndGetImgRC(r,c);

    uint16_t z = ExposureZ();

    //priming the camera interface for the upcoming data transfer via GetImage
    m_CamIo->SetupImgXfer( r, c, z, m_CamMode->IsBulkDownloadOn() );

    //the alta needs this resets in order to
    //image for 1 sec or less
    Reset( false );

    // Set horizontal and vertical imaging roi registers
    m_CcdAcqSettings->SetImagingRegs( m_FirmwareVersion );

    Reset( false );

    //execute preflash
    if( m_IsPreFlashOn )
    {
         ExectuePreFlash();

         Reset( false );

        //reset horizontal and vertical imaging roi registers
        //for the next real image
        m_CcdAcqSettings->SetImagingRegs( m_FirmwareVersion );

        Reset( false );
    }

     //boundary checking
    if( Duration < m_CameraConsts->m_ExposureTimeMin )
    {
        std::stringstream msg;
        msg << "Changing input exposure duration from " << Duration << " to " << m_CameraConsts->m_ExposureTimeMin;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        Duration = m_CameraConsts->m_ExposureTimeMin ;
    }

     if( Duration > m_CameraConsts->m_ExposureTimeMax )
    {
        std::stringstream msg;
        msg << "Changing input exposure duration from " << Duration << " to " << m_CameraConsts->m_ExposureTimeMax;
        std::string vinfo = apgHelper::mkMsg( m_fileName, msg.str(), __LINE__);
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"warn",vinfo);

        Duration = m_CameraConsts->m_ExposureTimeMax;
    }

    // Calculate and set the exposure time in the camera
    SetExpsoureTime( Duration );

    // Issue the flush for interlines, or if using the external shutter
    bool IssueFlush = false;
    if( m_CamCfgData->m_MetaData.InterlineCCD && m_CamMode->IsFastSequenceOn() )
    {
        IssueFlush = true;
    }
    else if( m_CamMode->IsTriggerExternalShutterOn() )
    {
        IssueFlush = true;
    }
    else if( m_CamMode->IsTriggerTdiKinEachOn() || 
        m_CamMode->IsTriggerTdiKinGroupOn() )
    {
        IssueFlush = true;
    }

    if( IssueFlush )
    {
        m_CamIo->WriteReg( CameraRegs::CMD_A,
            CameraRegs::CMD_A_FLUSH_BIT);
        
        m_CamIo->WriteReg( CameraRegs::SCRATCH, 0x8086 );
	    m_CamIo->WriteReg( CameraRegs::SCRATCH, 0x8088 );
	    m_CamIo->WriteReg( CameraRegs::SCRATCH, 0x8086 );
	    m_CamIo->WriteReg( CameraRegs::SCRATCH, 0x8088 );
    }

    // tell the camera to start taking the image
    IssueExposeCmd( IsLight );

    m_ImageInProgress = true;
    m_NumImgsDownloaded = 0;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::StartExposure END" );
#endif
}

//////////////////////////// 
// GET       IMAGING       STATUS  
Apg::Status Alta::GetImagingStatus()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImagingStatus" );
#endif

    CameraStatusRegs StatusObj = GetStatus();
    const uint16_t statusReg = StatusObj.GetStatus();

    //throw if there is an error
    IsThereAStatusError(statusReg);

    //see if we are waiting on hardware triggers
    if( statusReg & CameraRegs::STATUS_WAITING_TRIGGER_BIT )
    {
        //the user is externally controlling the exposure time
        if( m_CamMode->IsTriggerExternalShutterOn() && 
            (statusReg & CameraRegs::STATUS_IMAGING_ACTIVE_BIT) && 
            (statusReg & CameraRegs::STATUS_IMAGE_EXPOSING_BIT) )
        {
            return LogAndReturnStatus( Apg::Status_Exposing, StatusObj );
        }
        else
        {
             return LogAndReturnStatus(  Apg::Status_WaitingOnTrigger, StatusObj );
        }
    }

    
    if( m_ImageInProgress )
    {
        const bool ImgDone = IsImgDone( StatusObj );

        //see if we are in tdi mode
        if( Apg::CameraMode_TDI != GetCameraMode() ) 
        {
            //see if we are doing a sequence
            if( GetImageCount() > 1 )
            {
                if( m_CamMode->IsBulkDownloadOn() )
                {               
                    if( ImgDone )
                    {
                         return LogAndReturnStatus( Apg::Status_ImageReady, StatusObj );
                    }
                }
                else if( GetImgSequenceCount() > m_NumImgsDownloaded )
                {
                    return LogAndReturnStatus( Apg::Status_ImageReady, StatusObj );
                }
            }
            else
            {
                //use this logic for a single image
                if( ImgDone )
                {
                    return LogAndReturnStatus( Apg::Status_ImageReady, StatusObj );
                }
            }
        }

        //made up mode for TDI
        if( Apg::CameraMode_TDI == GetCameraMode() ) 
        {
                if( IsBulkDownloadOn() && ImgDone )
                {
                     return LogAndReturnStatus( Apg::Status_ImageReady, StatusObj );
                }
                else
                {
                    return LogAndReturnStatus( Apg::Status_ImagingActive, StatusObj );
                }
        }
    }

    //exposing OR active
    if( statusReg & CameraRegs::STATUS_IMAGING_ACTIVE_BIT )
    {
        if( statusReg & CameraRegs::STATUS_IMAGE_EXPOSING_BIT )
        {
            return LogAndReturnStatus( Apg::Status_Exposing, StatusObj );
        }
        else
        {
            return LogAndReturnStatus( Apg::Status_ImagingActive, StatusObj );
        }
    }

    //flushing
    if( statusReg & CameraRegs::STATUS_FLUSHING_BIT )
    {
         return LogAndReturnStatus( Apg::Status_Flushing, StatusObj );
    }

   //fall through
   return LogAndReturnStatus( Apg::Status_Idle, StatusObj );
}

//////////////////////////// 
// GET  IMAGE 
void Alta::GetImage( std::vector<uint16_t> & out )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImage -> BEGINNING" );
#endif

    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Getting Image.");


    //pre-condition make sure the image is ready
    if( Apg::CameraMode_TDI == GetCameraMode() && !IsBulkDownloadOn() ) 
    {
        if( Apg::Status_ImagingActive != GetImagingStatus() )
        {
            std::stringstream msg;
            msg << "Invalid imaging status, " << GetImagingStatus();
            msg << ", for getting TDI image data.";
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidMode );
        }
    }
    else
    {
        Apg::Status actualStatus = Apg::Status_Idle;

        if( !CheckAndWaitForStatus( Apg::Status_ImageReady, actualStatus ) )
        {
            std::stringstream msg;
            msg << "Invalid imaging status, " << actualStatus;
            msg << ", for getting image data.";
            apgHelper::throwRuntimeException( m_fileName, msg.str(), 
                __LINE__, Apg::ErrorType_InvalidMode );
        }
    }

    // allocating the buffers for the image
    // doing this outside of the try / catch, so that
    // even if the GetImage function throws
    // we can try to copy whatever data we managed
    // to fetch from the camera into the user supplied
    // vector
    uint16_t r=0, c = 0;
    ExposureAndGetImgRC( r, c );
    const uint16_t z = GetImageZ();
    std::vector<uint16_t> datafromCam( r*c*z, 0 ); 

    const int32_t dataLen = r*z;
    const int32_t numCols = GetRoiNumCols();  

    if( dataLen*numCols != apgHelper::SizeT2Int32( out.size() ) )
    {
        out.clear();
        out.resize( dataLen*numCols );
    }

    try
    {
        m_CamIo->GetImageData( datafromCam );
    }
    catch(std::exception & err )
    {
        m_ImageInProgress = false;
        
        // log and remove the AD garbage pixels at the beginning of every row
        std::string msg( "Removing AD latency pixels in exception handler" );
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
        apgHelper::mkMsg( m_fileName, msg, __LINE__) );

        FixImgFromCamera( datafromCam, out, dataLen, numCols );
        throw;
    }
    
    ++m_NumImgsDownloaded;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImage -> m_NumImgsDownloaded = %d", m_NumImgsDownloaded );
#endif

    //determine if we are done imaging     
    if( m_CamMode->GetMode() == Apg::CameraMode_TDI )
    {
        if( GetTdiRows() == m_NumImgsDownloaded ||
            IsBulkDownloadOn() )
        {
            m_ImageInProgress = false;
            Reset( true );
        }
    }
    else
    {
        if( IsBulkDownloadOn() ||
             GetImageCount() == m_NumImgsDownloaded )
        {
            m_ImageInProgress = false;
        }
    
    }

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImage -> m_ImageInProgress = %d", m_ImageInProgress );
#endif

    // removing the AD garbage pixels at the beginning of every row
    FixImgFromCamera( datafromCam, out, dataLen, numCols );
  
    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Get Image Completed.");

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImage -> END" );
#endif

}

//////////////////////////// 
// EXPOSURE      Z
uint16_t Alta::ExposureZ()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::ExposureZ" );
#endif

    //detemine number of images
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() )
    {
            return GetTdiRows();
    }

    return GetImageCount();  
}

//////////////////////////// 
// GET  IMAGE    Z
uint16_t Alta::GetImageZ()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetImageZ" );
#endif
    
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() &&
        m_CamMode->IsBulkDownloadOn() )
    {
            return GetTdiRows();
    }
    

    return (m_CamMode->IsBulkDownloadOn() ? GetImageCount() : 1 );
}


//////////////////////////// 
// EXPOSURE    AND     GET        IMG     RC
void Alta::ExposureAndGetImgRC(uint16_t & r, uint16_t & c)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::ExposureAndGetImgRC" );
#endif

    //detemine the exposure width
    c = m_CcdAcqSettings->GetRoiNumCols() + m_CcdAcqSettings->GetPixelShift();

     //detemine the exposure height
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() )
    {
        r =  1;
    }
    else
    {
        r = m_CcdAcqSettings->GetRoiNumRows();
    }
      
}

//////////////////////////// 
// STOP EXPOSE  
void Alta::StopExposure( const bool Digitize )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::StopExposure -> Digitize =%d", Digitize );
#endif

    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Stoping Exposure.");

    switch( GetCameraMode() )
    {
        case Apg::CameraMode_Normal:
            // check if the image is ready - AltaU's register IO
            // appears blocked when data is ready for transfer
            // this is probably b/c there is one IO interface between
            // the fx2 chip and the FPGA.  Ascent based cameras have
            // two interfaces, one for registers and one for images.
            if( Apg::Status_ImageReady == GetImagingStatus() )
            {
                StopExposureImageReady( Digitize );
                // exit here b/c the exposure is over
                #ifdef DEBUGGING_CAMERA
                    apgHelper::DebugMsg( "Alta::StopExposure END" );
                #endif
                return;
            }

            // the image is not ready, follow the regular stop image
            // path
            StopExposureModeNorm( Digitize );
        break;

        case Apg::CameraMode_TDI:
        case Apg::CameraMode_Kinetics:
            StopExposureModeTdiKinetics( Digitize );
        break;

        default:
          apgHelper::throwRuntimeException(m_fileName,
            "Error: Unknown camera mode.", 
                __LINE__, Apg::ErrorType_InvalidMode );
        break;
    }

}

//////////////////////////// 
//  STOP            EXPOSURE      IMAGE      READY        
void Alta::StopExposureImageReady(const bool Digitize )
{
     if( GetImageCount() > 1)
     {
         // cancel the USB transfer, which should clear the IO interface
         // which should allow register access
         m_CamIo->CancelImgXfer();
         
         //tell the fpga to stop imaging
        WriteReg(CameraRegs::CMD_B,
                CameraRegs::CMD_B_END_EXPOSURE_BIT );

        // calling this again to make sure the CAMCON and BUFCON internals
        // are ok and not mucked up if data was transfered between
        // the first hard stop and the register call
        HardStopExposure( "Hard stop 1 of an exposure of image sequences" );
     }
     else
     {
            // check if the user wants single the image
            if( !Digitize )
            {
                GrabImageAndThrowItAway();
            }
     }

}

//////////////////////////// 
// STOP EXPOSE  
void Alta::StopExposureModeTdiKinetics( const bool Digitize )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::StopExposureModeTdiKinetics -> Digitize =%d", Digitize );
#endif

    //pre-conditions
    if( Apg::CameraMode_TDI != GetCameraMode() &&
        Apg::CameraMode_Kinetics != GetCameraMode() )
    {
     // we are not in the right mode throw an exception
    apgHelper::throwRuntimeException( m_fileName,
        "Error: Invalid camera mode for StopExposureModeTdiKinetics.", 
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
        return;
    }

    //tell the fpga to stop imaging
    WriteReg(CameraRegs::CMD_B,
        CameraRegs::CMD_B_END_EXPOSURE_BIT );

    //TODO - verify this is correct...seems like we should be fetching data
    //or checking for triggers???
    if( !Digitize )
    {
        m_CamIo->CancelImgXfer();
    }

    m_ImageInProgress = false;
    Reset( true );

}

//////////////////////////// 
//      GET    AVAILABLE    MEMORY
uint32_t Alta::GetAvailableMemory()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetAvailableMemory" );
#endif

    uint32_t result = 0;
    switch( m_CamIo->GetInterfaceType() )
    {
        case CamModel::USB:
            result = ALTA_U_MEMORY;
        break;

        case CamModel::ETHERNET:
            result = ALTA_E_MEMORY;
        break;

        default:
            apgHelper::throwRuntimeException( m_fileName,
            "Error: Cannot get memory size invalid interface type.", 
                __LINE__, Apg::ErrorType_InvalidUsage );
        break;
    }

    return result;

}


//////////////////////////// 
// SET    CCD      ADC      12BIT    GAIN
void Alta::SetCcdAdc12BitGain( const uint16_t gain )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::SetCcdAdc12BitGain -> gain =%d", gain );
#endif

    std::dynamic_pointer_cast<AltaCcdAcqParams>(m_CcdAcqSettings)->Set12BitGain( gain );
}

//////////////////////////// 
// SET    CCD      ADC      12BIT    OFFSET
void Alta::SetCcdAdc12BitOffset( const uint16_t offset )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::SetCcdAdc12BitOffset -> offset =%d", offset );
#endif

    std::dynamic_pointer_cast<AltaCcdAcqParams>(m_CcdAcqSettings)->Set12BitOffset( offset );
}

//////////////////////////// 
// GET    CCD      ADC      12BIT    GAIN
uint16_t Alta::GetCcdAdc12BitGain()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetCcdAdc12BitGain" );
#endif

    return std::dynamic_pointer_cast<AltaCcdAcqParams>(m_CcdAcqSettings)->Get12BitGain();
}

//////////////////////////// 
// GET    CCD      ADC      12BIT    OFFSET
uint16_t Alta::GetCcdAdc12BitOffset()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetCcdAdc12BitOffset" );
#endif

    return std::dynamic_pointer_cast<AltaCcdAcqParams>(m_CcdAcqSettings)->Get12BitOffset();
}

//////////////////////////// 
// GET    CCD      ADC      16BIT    GAIN
double Alta::GetCcdAdc16BitGain()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetCcdAdc16BitGain" );
#endif

    return std::dynamic_pointer_cast<AltaCcdAcqParams>(m_CcdAcqSettings)->Get16bitGain();
}

//////////////////////////// 
//  GET    ILLUMINATION       MASK
uint16_t Alta::GetIlluminationMask()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetIlluminationMask" );
#endif
    return CameraRegs::MASK_LED_ILLUMINATION_ALTA;
}

//////////////////////////// 
//  GET    NUM        ADS
int32_t Alta::GetNumAds()
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetNumAds" );
#endif
    return 2; 
}

//////////////////////////// 
//  GET    NUM        ADC        CHANNELS
int32_t Alta::GetNumAdChannels() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetNumAdChannels" );
#endif
    return 1; 
}  

//////////////////////////// 
//      GET        COOLER     DRIVE
double Alta::GetCoolerDrive()
{
     CameraStatusRegs curStatus = GetStatus();

     const uint16_t Reg = curStatus.GetCoolerDrive() & CameraRegs::MASK_TEMP_PARAMS;

     if( Reg > m_CameraConsts->m_CoolerDriveMax)
     {
         //we are maxed out
         //see firmware notes on register 95
         return 100.0;
     }

     if( Reg <  m_CameraConsts->m_CoolerDriveOffset)
     {
         //we are bottomed out
         //see firmware notes on register 95
         return 0.0;
     }

     const double result = ( 
         (static_cast<double>(Reg) - m_CameraConsts->m_CoolerDriveOffset) / m_CameraConsts->m_CoolerDriveDivisor ) * 100.0;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetCoolerDrive -> drive = %f", result );
#endif

     return result;
}

//////////////////////////// 
//  SET      FAN       MODE
void Alta::SetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::SetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif

   DefaultSetFanMode( mode, PreCondCheck );
}

//////////////////////////// 
//  GET      FAN       MODE
Apg::FanMode Alta::GetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::GetFanMode" );
#endif
    return DefaultGetFanMode();
}

//////////////////////////// 
//      FIX      IMG        FROM          CAMERA
void Alta::FixImgFromCamera( const std::vector<uint16_t> & data,
                              std::vector<uint16_t> & out,  const int32_t rows, 
                              const int32_t cols )
{
    const int32_t offset = m_CcdAcqSettings->GetPixelShift();
    ImgFix::SingleOuputCopy( data, out, rows, cols, offset );
}

//////////////////////////// 
//  GET    TEMP      HEATSINK
double Alta::GetTempHeatsink()
{
    return DefaultGetTempHeatsink();
}

//////////////////////////// 
//      INIT
void Alta::Init()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Alta::Init" );
#endif
    DefaultInit();
}


//////////////////////////// 
//      OPEN      SERIAL
void Alta::OpenSerial( const uint16_t PortId )
{
    if( IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " already open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //init the serial port
    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialBaudRate(
            PortId , DEFAULT_SERIAL_PORT_BAUD );
    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialFlowControl(
            PortId, DEFAULT_SERIAL_PORT_CF );
    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialParity(
            PortId, DEFAULT_SERIAL_PORT_PARITY );

    m_serialPortOpenStatus[PortId] = true;
}

//////////////////////////// 
//      CLOSE        SERIAL
void Alta::CloseSerial( const uint16_t PortId )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    m_serialPortOpenStatus[PortId] = false;
}

//////////////////////////// 
//      SET     SERIAL       BAUD     RATE
void Alta::SetSerialBaudRate( const uint16_t PortId , const uint32_t BaudRate )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialBaudRate(
        PortId , BaudRate );
}

//////////////////////////// 
//      GET     SERIAL       BAUD     RATE
uint32_t Alta::GetSerialBaudRate( const uint16_t PortId  )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    return std::dynamic_pointer_cast<AltaIo>(m_CamIo)->GetSerialBaudRate( PortId );
}

//////////////////////////// 
//      GET    SERIAL    FLOW         CONTROL
Apg::SerialFC Alta::GetSerialFlowControl( const uint16_t PortId )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    return std::dynamic_pointer_cast<AltaIo>(m_CamIo)->GetSerialFlowControl( PortId );
}

//////////////////////////// 
//      SET    SERIAL    FLOW         CONTROL
void Alta::SetSerialFlowControl( const uint16_t PortId, 
            const Apg::SerialFC FlowControl )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialFlowControl(
        PortId, FlowControl );
}

//////////////////////////// 
//   GET    SERIAL      PARITY
Apg::SerialParity Alta::GetSerialParity( const uint16_t PortId )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    return std::dynamic_pointer_cast<AltaIo>(m_CamIo)->GetSerialParity( PortId );
}
     
//////////////////////////// 
//   SET        SERIAL           PARITY
void Alta::SetSerialParity( const uint16_t PortId, const Apg::SerialParity Parity )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->SetSerialParity(
        PortId, Parity );
}
     
//////////////////////////// 
//   READ     SERIAL
std::string Alta::ReadSerial( const uint16_t PortId )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::string buffer;

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->ReadSerial(
        PortId, buffer );

    return buffer;
}

//////////////////////////// 
//   WRITE           SERIAL
void Alta::WriteSerial( const uint16_t PortId, const std::string & buffer )
{
    if( !IsSerialPortOpen( PortId ) )
    {
        std::stringstream errMsg;
        errMsg << "Serial port " << PortId << " is not open";
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
                __LINE__, Apg::ErrorType_InvalidUsage );
    }

    std::dynamic_pointer_cast<AltaIo>(m_CamIo)->WriteSerial(
        PortId, buffer );
}

//////////////////////////// 
//  IS     SERIAL   PORT     OPEN
bool Alta::IsSerialPortOpen( const uint16_t PortId )
{
    std::map<uint16_t , bool>::iterator iter = 
        m_serialPortOpenStatus.find( PortId );

    if( iter != m_serialPortOpenStatus.end() )
    {
        return (*iter).second;
    }

    std::stringstream errMsg;
    errMsg << "Invalid serial port " << PortId;
    apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );

    //should never reach here
    return false;

}

