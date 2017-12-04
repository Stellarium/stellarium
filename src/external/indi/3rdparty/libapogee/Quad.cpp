/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2011 Apogee Imaging Systems, Inc. 
* \class Quad
* \brief camera class for f4320 for Quad
* 
*/ 

#include "Quad.h" 
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
Quad::Quad( ) :   CamGen2Base( CamModel::QUAD ),
                            m_fileName( __FILE__ ),
                            m_DoPixelReorder( true )
{   
    //alloc and set the camera constants
    m_CameraConsts = std::shared_ptr<PlatformData>( new AscentData() );
}


//////////////////////////// 
// DTOR 
Quad::~Quad() 
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
            std::string msg ("Exception caught in ~Quad msg = " );
            msg.append( err.what() );
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
                msg);
        }
        catch( ... )
        {
            ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"error",
            "Unknown exception caught stopping exposure in ~Quad" );
        }
    }

} 

//////////////////////////// 
// OPEN     CONNECTION
void Quad::OpenConnection( const std::string & ioType,
             const std::string & DeviceAddr,
             const uint16_t FirmwareRev,
             const uint16_t Id )
{
#ifdef DEBUGGING_CAMERA
   apgHelper::DebugMsg( "Quad::OpenConnection -> ioType= %s, DeviceAddr = %s, FW = %d, ID = %d", 
       ioType.c_str(), DeviceAddr.c_str(), FirmwareRev, Id  );
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
//      CLOSE        CONNECTION
void Quad::CloseConnection()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::DefaultCloseConnection");
#endif

    DefaultCloseConnection();
}

//////////////////////////// 
//      UPDATE     CFG        WITH       STR    DB       INFO
void Quad::UpdateCfgWithStrDbInfo()
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
// CFG      CAM       FROM  ID
void Quad::CfgCamFromId( const uint16_t CameraId )
{
     //create and set the camera's cfg data
    DefaultCfgCamFromId( CameraId );
}


//////////////////////////// 
// CREATE      CAMIO        
void Quad::CreateCamIo(const std::string & ioType,
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
// VERIFY        CAM        ID 
void Quad::VerifyCamId()
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
void Quad::FixImgFromCamera( const std::vector<uint16_t> & data,
                                            std::vector<uint16_t> & out,  const int32_t rows, 
                                            const int32_t cols)
{
    int32_t offset = 0; 

    switch( m_CamCfgData->m_MetaData.NumAdOutputs )
    {
        case 1:
            offset = m_CcdAcqSettings->GetPixelShift();
            ImgFix::SingleOuputCopy( data, out, rows, cols, offset );
        break;

        case 4:
        {
            uint16_t r =0, c=0;
            ExposureAndGetImgRC( r, c );
            offset = c - cols;
            if( m_DoPixelReorder )
            {
                ImgFix::QuadOuputFix( data, out, rows, cols, offset );
            }
            else
            {
                ImgFix::QuadOuputCopy( data, out, rows, cols, offset );
            }
        }
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
void Quad::StartExposure( const double Duration, const bool IsLight )
{
    //verify the roi is valid
    const uint16_t maxImgRows =  GetMaxImgRows() / GetRoiBinRow();
    if( !IsRoiCenteredAndSymmetric( maxImgRows, 
                  GetRoiStartRow(),  
                  GetRoiNumRows() ) )
    {

        std::stringstream msg;
        msg << "Invaild roi: ";
        msg << "ccd imaging rows = " << maxImgRows;
        msg << "; start row = " << GetRoiStartRow();
        msg << "; # roi rows = " << GetRoiNumRows();
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );

    }

    const uint16_t maxImgCols =  GetMaxImgCols() / GetRoiBinCol();
     if( !IsRoiCenteredAndSymmetric( maxImgCols , 
                  GetRoiStartCol(),  
                  GetRoiNumCols() ) )
    {

        std::stringstream msg;
        msg << "Invaild roi: ";
        msg << "ccd imaging cols = " << maxImgCols;
        msg << "; start col = " << GetRoiStartCol();
        msg << "; # roi cols = " << GetRoiNumCols();
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    // start expsoure
    DefaultStartExposure( Duration, IsLight, true );
}


//////////////////////////// 
//      IS     ROI     CENTERED        AND        SYMMETRIC
bool Quad::IsRoiCenteredAndSymmetric( const uint16_t ccdLen, 
                      const uint16_t startingPos,  
                      const uint16_t roiLen )
{
    const int32_t CENTER = ccdLen / 2;

    const int32_t START_DIFF = startingPos - CENTER;

    if( START_DIFF >= 0 )
    {
        return false;
    }

    const int32_t END_POS = startingPos + roiLen;

    const int32_t END_DIFF = CENTER - END_POS;

    if( END_DIFF >= 0 )
    {
        return false;
    }

    if( END_DIFF != START_DIFF )
    {
        return false;
    }

    return true;
}

//////////////////////////// 
// EXPOSURE    AND     GET        IMG     RC
void Quad::ExposureAndGetImgRC(uint16_t & r, uint16_t & c)
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::ExposureAndGetImgRC" );
#endif

    //detemine the exposure height
    r = m_CcdAcqSettings->GetRoiNumRows();

    //detemine the exposure width
    c = m_CcdAcqSettings->GetRoiNumCols() + (m_CcdAcqSettings->GetPixelShift()*2 );

}

//////////////////////////// 
//  GET    NUM        ADC        CHANNELS
int32_t Quad::GetNumAdChannels() 
{ 
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::GetNumAdChannels" );
#endif
    return 2; 
}  

//////////////////////////// 
//      SET     IS    QUAD     BIT
void Quad::SetIsQuadBit()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::SetIsQuadBit" );
#endif
    //set high this is a quad camera
    m_CamIo->ReadOrWriteReg( CameraRegs::OP_C,
        CameraRegs::OP_C_IS_QUAD_BIT );
}

//////////////////////////// 
//      INIT
void Quad::Init()
{
 #ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::Init" );
#endif

    DefaultInit();
	SetIsQuadBit();
}

//////////////////////////// 
//  SET      FAN       MODE
void Quad::SetFanMode( const Apg::FanMode mode, const bool PreCondCheck )
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::SetFanMode -> mode = %d, PreCondCheck =%d ", 
        mode, PreCondCheck );
#endif
    // no op on purpose, no fan control for v108 firmware and hic cameras
    // just ignore request...

}

//////////////////////////// 
//  GET      FAN       MODE
Apg::FanMode Quad::GetFanMode()
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "Quad::GetFanMode" );
#endif

    // no fan control on ascent based cameras
    return Apg::FanMode_Off;
}