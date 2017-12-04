/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class CamGen2Base 
* \brief This is the base class for the second generation apogee cameras (Ascent, Aspen, etc).  This is a derived class of the ApogeeCam, which contains the function common to both Alta and second generation cameras. 
* 
*/ 

#include "CamGen2Base.h" 
#include "apgHelper.h" 
#include "ApnCamData.h"
#include "CameraIo.h" 
#include "CamHelpers.h" 
#include "ModeFsm.h" 
#include "CcdAcqParams.h" 
#include "ApgLogger.h" 
#include "PlatformData.h" 

#include <sstream>
#include <cstring>  //for memset
#include <algorithm>

namespace
{
     const uint32_t MEMORY_SIZE = 32 * 1024;
     const int32_t NUM_ADC = 2;
}

//////////////////////////// 
// CTOR 
CamGen2Base::CamGen2Base(CamModel::PlatformType platform) : 
                ApogeeCam(platform),
                m_fileName( __FILE__ )
{ 

} 

//////////////////////////// 
// DTOR 
CamGen2Base::~CamGen2Base()
{ 

} 
 
//////////////////////////// 
// GET    STATUS
CameraStatusRegs CamGen2Base::GetStatus()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetStatus" );
#endif

    CameraStatusRegs::AdvStatus adv;
    memset(&adv,0,sizeof(adv));
    m_CamIo->GetStatus(adv);

    CameraStatusRegs status(adv);

     #ifdef DEBUGGING_CAMERA_STATUS
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - CoolerDrive = 0x%X", status.GetCoolerDrive() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetCurrentFrame = 0x%X", status.GetCurrentFrame() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetDataAvailFlag = 0x%X", status.GetDataAvailFlag() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetFetchCount = 0x%X", status.GetFetchCount() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetInputVoltage = 0x%X", status.GetInputVoltage() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetMostRecentFrame = 0x%X", status.GetMostRecentFrame() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetReadyFrame = 0x%X", status.GetReadyFrame() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetSequenceCounter = 0x%X", status.GetSequenceCounter() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetStatus = 0x%X", status.GetStatus() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetTdiCounter = 0x%X", status.GetTdiCounter() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetTempCcd = 0x%X", status.GetTempCcd() );
        apgHelper::DebugMsg( "CamGen2Base::GetStatus - GetuFrame = 0x%X", status.GetuFrame() );
    #endif

    return status;
}

//////////////////////////// 
// DEFAULT    START    EXPOSURE  
void CamGen2Base::DefaultStartExposure( double Duration, const bool IsLight, 
                                       const bool IssueReset )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( " CamGen2Base::StartExposure -> Duration = %f, IsLight = %d",
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
    
    // calling reset causes the camera to stop flushing
    // putting the Resets back into the StartExpose function, 
    // because without them the gen2 based cameras (ascent, altaF, altaG) 
    // will have a USB hang on GetImage without it when going from 
    // subframe -> full frame.  this is because without the reset call, which 
    // stops sensor flushing, the firmware is trying to read those registers 
    // at the end of every line flush to determine what to flush.  thus causing 
    // a read/write conflict that causes the GetImage function to hang.  
    // putting these back in re-introduces the "short dark phenomenon" 
    // reported in tickets #92 and #94 in the alta project.  see ticket #119 in alta
    // for all of the details on this error
    // 2012-05-03 have a firmware fix for this issue.  to maintain backwards
    // compatibility with old firmware only skipping reset when told it is ok
    if( IssueReset )
    {
        Reset( false );
    }

    // Set horizontal and vertical imaging roi registers
    m_CcdAcqSettings->SetImagingRegs( m_FirmwareVersion );

    if( IssueReset )
    {
        Reset( false );
    }

    //execute preflash
    if( m_IsPreFlashOn )
    {
         ExectuePreFlash();

        if( IssueReset )
        {
            Reset( false );
        }

        //reset horizontal and vertical imaging roi registers
        //for the next real image
        m_CcdAcqSettings->SetImagingRegs( m_FirmwareVersion );

        if( IssueReset )
        {
            Reset( false );
        }
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
}


//////////////////////////// 
// GET       IMAGING       STATUS  
Apg::Status CamGen2Base::GetImagingStatus()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetImagingStatus" );
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
void CamGen2Base::GetImage( std::vector<uint16_t> & out )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetImage -> BEGIN" );
#endif

    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Getting Image.");


    //pre-condition make sure the image is ready
    Apg::Status actualStatus = GetImagingStatus();

	switch (actualStatus)
	{
	case Apg::Status_Exposing:
	case Apg::Status_ImagingActive:
	case Apg::Status_ImageReady:
		break;
	default:
        std::stringstream msg;
        msg << "Invalid imaging status, " << actualStatus;
        msg << ", for getting image data.";
        apgHelper::throwRuntimeException(m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidMode );
    }


    // allocating the buffers for the image
    // doing this outside of the try / catch, so that
    // even if the GetImage function throws
    // we can try to copy whatever data we managed
    // to fetch from the camera into the user supplied
    // vector
    uint16_t r=0, c= 0;
    ExposureAndGetImgRC( r, c );
    const uint16_t z = GetImageZ();
    std::vector<uint16_t>  datafromCam( r*c*z );

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
        // logging and at a minimum removing the AD garbage pixels at the beginning of every row
        std::string msg( "Removing AD latency pixels in exception handler" );
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
        apgHelper::mkMsg( m_fileName, msg, __LINE__) );

        FixImgFromCamera( datafromCam, out, dataLen, numCols );
        throw;
    }
        
    ++m_NumImgsDownloaded;

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
    
    // at a minimum removing the AD garbage pixels at the beginning of every row
    FixImgFromCamera( datafromCam, out, dataLen, numCols );

   ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Get Image Completed.");

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetImage -> END" );
#endif

}

//////////////////////////// 
// EXPOSURE      Z
uint16_t CamGen2Base::ExposureZ()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::ExposureZ" );
#endif

    //detemine number of images
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() )
    {
            return GetTdiRows();
    }

    //detemine number of images
    return GetImageCount();  
}

//////////////////////////// 
// GET  IMAGE    Z
uint16_t CamGen2Base::GetImageZ()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetImageZ" );
#endif
    
    if( Apg::CameraMode_TDI == m_CamMode->GetMode() &&
        m_CamMode->IsBulkDownloadOn() )
    {
            return GetTdiRows();
    }
    

    return( m_CamMode->IsBulkDownloadOn() ? GetImageCount() : 1 );
}


//////////////////////////// 
// STOP EXPOSE  
void CamGen2Base::StopExposure( const bool Digitize )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::StopExposure -> Digitize = %d", Digitize );
#endif

    StopExposureModeNorm( Digitize );
}

//////////////////////////// 
//      GET    AVAILABLE    MEMORY
uint32_t CamGen2Base::GetAvailableMemory()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetAvailableMemory" );
#endif

    return MEMORY_SIZE;
}

//////////////////////////// 
//  GET    ILLUMINATION       MASK
uint16_t CamGen2Base::GetIlluminationMask()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetIlluminationMask" );
#endif

    return CameraRegs::MASK_LED_ILLUMINATION_ASCENT;
}

//////////////////////////// 
//  GET    NUM        ADS
int32_t CamGen2Base::GetNumAds()
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetNumAds" );
#endif
    return NUM_ADC; 
}

//////////////////////////// 
//      GET        COOLER     DRIVE
double CamGen2Base::GetCoolerDrive()
{
     CameraStatusRegs curStatus = GetStatus();

     if( curStatus.GetCoolerDrive() > m_CameraConsts->m_CoolerDriveMax)
     {
         //we are maxed out
         return 100.0;
     }

     if( curStatus.GetCoolerDrive() <  m_CameraConsts->m_CoolerDriveOffset)
     {
         return 0.0;
     }

     const double result = ( 
         (static_cast<double>(curStatus.GetCoolerDrive() ) - m_CameraConsts->m_CoolerDriveOffset) 
         / m_CameraConsts->m_CoolerDriveDivisor ) * 100.0;

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetCoolerDrive -> drive = %f", result );
#endif

     return result;
}

//////////////////////////// 
//  GET    TEMP      HEATSINK
double CamGen2Base::GetTempHeatsink()
{
    // there was no heatsink temp on all gen2 camera, ascent, altaf, aspen(altag)
	// but newer revisions of the altaf have a heatsink temp, and future ascents might
	// return the temperature as if there is a heatsink, just in case there is
    double const temp = DefaultGetTempHeatsink();

 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamGen2Base::GetTempHeatsink -> temp = %f", temp );
#endif

     return temp;
}


