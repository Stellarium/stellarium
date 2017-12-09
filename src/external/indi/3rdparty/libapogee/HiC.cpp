/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class HiC 
* \brief Camera object for the HiC 
*/ 

#include "HiC.h" 
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
HiC::HiC() : Quad(),
                    m_fileName( __FILE__ )

{
    SetPixelReorder( false );
    m_PlatformType = CamModel::HIC;
} 

//////////////////////////// 
// DTOR 
HiC::~HiC() 
{ 
} 

//////////////////////////// 
//      SET     SERIAL       NUMBER
void HiC::SetSerialNumber(const std::string & num)
{
    m_CamIo->SetSerialNumber( num );
}

//////////////////////////// 
//  GET        CAM        INFO 
CamInfo::StrDb HiC::GetCamInfo()
{
	return std::dynamic_pointer_cast<AscentBasedIo>(
        m_CamIo)->ReadStrDatabase();
}
       
//////////////////////////// 
//  SET        CAM        INFO 
void HiC::SetCamInfo( CamInfo::StrDb & info )
{
	std::dynamic_pointer_cast<AscentBasedIo>(
        m_CamIo)->WriteStrDatabase( info );
}

//////////////////////////// 
//      GET    4K        BY       4K        IMAGE
void HiC::Get4kby4kImage( std::vector<uint16_t> & out )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "HiC::Get4kby4kImage -> BEGIN" );
#endif

    ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Getting Image.");

    //pre-condition make sure the image is ready
    Apg::Status actualStatus = Apg::Status_Idle;

    if( !CheckAndWaitForStatus( Apg::Status_ImageReady, actualStatus ) )
    {
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

    const int32_t dataLen = GetRoiNumRows()*z;
    const int32_t numCols = GetRoiNumCols();
    
    try
    {
        m_CamIo->GetImageData( datafromCam );
    }
    catch(std::exception & err )
    {
        m_ImageInProgress = false;
        // logging and at a minimum removing the AD garbage pixels at the end of every row
        std::string msg( "Removing AD latency pixels in exception handler" );
        ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
        apgHelper::mkMsg( m_fileName, msg, __LINE__) );

        FixImgFromCamera( datafromCam, out, dataLen, numCols );
        throw;
    }
        
    ++m_NumImgsDownloaded;

    if( IsBulkDownloadOn() )
    {
        m_ImageInProgress = false;
    }
    else if( GetImageCount() == m_NumImgsDownloaded )
    {
        m_ImageInProgress = false;
    }
    
    // at a minimum removing the AD garbage pixels at the end of every row
    const uint16_t HIC_ROWS = 4096;
    const uint16_t HIC_COLS = 4096;
    if( HIC_ROWS*HIC_COLS != out.size() )
    {
        out.clear();
        out.resize( HIC_ROWS*HIC_COLS );
    }

    // first see if the buffer from the camera is a good size
    // and the number of columns is good.  if either of these conditions
    // fail then just get as much data out as you can and then throw
    // if not fall through to the happy path of removing the
    // latency pixels, which for NASA will be the only operation they
    // perform, for internal testing we will reorder which will have some
    // time and memory over head
    const int32_t LATENCY_PIXELS =  c - numCols;

    // if the data from the camera is bigger than the
    if( datafromCam.size() > out.size() )
    {
        // TODO - copy some data into the output buffer
        // before throw
        std::stringstream msg;
        msg << "Invalid buffer size from camera " << datafromCam.size();
        apgHelper::throwRuntimeException(m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    // if the number of columns is not 4096
    if( HIC_COLS != numCols )
    {
        // TODO - copy some data into the output buffer 
        // before throw
        std::stringstream msg;
        msg << "Invalid number of columns " << numCols;
        apgHelper::throwRuntimeException(m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    //we are good.  now deal with the data
    const int32_t OUTPUT_OFFSET =  
    ( (m_CamCfgData->m_MetaData.ImagingRows - r) / 2 ) * numCols;

    ImgFix::QuadOuputCopy( datafromCam, out, dataLen, 
        numCols, LATENCY_PIXELS, OUTPUT_OFFSET );

    if( IsPixelReorderOn() )
    {
        std::vector<uint16_t> temp = out;
        //already removed latency pixels above
        ImgFix::QuadOuputFix( temp, out, dataLen, numCols, 0 );
    }
   
   ApgLogger::Instance().Write(ApgLogger::LEVEL_DEBUG,"info","Get Image Completed.");

#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "HiC::Get4kby4kImage -> END" );
#endif

}


