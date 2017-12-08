/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2009 Apogee Instruments, Inc. 
* \class CamUsbIo 
* \brief Usb implementation of the ICamIo interface
* 
*/ 

#include "CamUsbIo.h" 

#include <sstream>
#include <algorithm>


#include <iostream>

#include "apgHelper.h" 
#include "IUsb.h"
#include "helpers.h"
#include "ApnUsbSys.h"


#ifdef WIN_OS
   #include "GenTwoWinUSB.h" 
#else
    #include "linux/GenOneLinuxUSB.h"
#endif



//////////////////////////// 
// CTOR 
CamUsbIo::CamUsbIo( const std::string & DeviceEnum, const uint32_t MaxBufSize,
                                       const bool ApplyPad ) : 
                                                   m_fileName( __FILE__ ),
                                                   m_ApplyPadding( ApplyPad ),
                                                   m_MaxBufSize( MaxBufSize )

{ 

    const uint16_t deviceNum = help::Str2uShort( DeviceEnum );

#ifdef WIN_OS
        m_Usb =  std::shared_ptr<IUsb>(new GenTwoWinUSB( deviceNum ) );
#else
    m_Usb =  std::shared_ptr<IUsb>(new GenOneLinuxUSB( deviceNum ) );
#endif

  
}

//////////////////////////// 
// DTOR 
CamUsbIo::~CamUsbIo() 
{ 

} 

//////////////////////////// 
// READ     REG 
uint16_t CamUsbIo::ReadReg( uint16_t reg ) const 
{
 #ifdef DEBUGGING_CAMERA
    const uint16_t val = m_Usb->ReadReg( reg );
    apgHelper::DebugMsg( "CamUsbIo::ReadReg -> reg = 0x%X, val = 0x%X", reg, val );
    return val;
#else
    return m_Usb->ReadReg( reg );
#endif    

    
}

//////////////////////////// 
// WRITE        REG 
void CamUsbIo::WriteReg( uint16_t reg, uint16_t val ) 
{
#ifdef DEBUGGING_CAMERA
    apgHelper::DebugMsg( "CamUsbIo::WriteReg -> reg = 0x%X, val = 0x%X", reg, val );
#endif

    m_Usb->WriteReg( reg, val );
}

//////////////////////////// 
// WRITE        SRMD 
void CamUsbIo::WriteSRMD( const uint16_t reg, const std::vector<uint16_t> & data )
{
    std::vector<uint16_t>::const_iterator iter;
    for( iter = data.begin(); iter != data.end(); ++iter )
    {
        WriteReg( reg, *iter );
    }
}

//////////////////////////// 
// WRITE        MRMD 
void CamUsbIo::WriteMRMD(const uint16_t reg, const std::vector<uint16_t> & data )
{
    std::vector<uint16_t>::const_iterator iter;
    uint16_t offset = 0;
    for( iter = data.begin(); iter != data.end(); ++iter, ++offset )
    {
        WriteReg( reg+offset, *iter );
    }
}

//////////////////////////// 
// GET      USB     VENDOR      INFO
void CamUsbIo::GetUsbVendorInfo( uint16_t & VendorId,
            uint16_t & ProductId, uint16_t  & DeviceId)
{
    m_Usb->GetVendorInfo(VendorId, ProductId, DeviceId);
}


//////////////////////////// 
// SETUP     IMG     XFER
void CamUsbIo::SetupImgXfer(const uint16_t Rows, 
            const uint16_t Cols,
            const uint16_t NumOfImages, 
            const bool IsBulkSeq)
{
    if( IsBulkSeq )
    {
        //we are going to fake out the hardware by "rolling"
        //the number of images we want into the image height
        //thus the hw will wait for one really big image
        m_Usb->SetupSingleImgXfer(Rows, Cols*NumOfImages );

    }
    else if( NumOfImages > 1 )
    {
        //request a set of images
        m_Usb->SetupSequenceImgXfer(Rows, Cols, NumOfImages );
    }
    else
    {
        //just get one image with this h and w
        m_Usb->SetupSingleImgXfer( Rows, Cols );
    }
    
}

//////////////////////////// 
// CANCEL      IMG         XFER
void CamUsbIo::CancelImgXfer()
{
    m_Usb->CancelImgXfer();

}

//////////////////////////// 
// GET  IMAGE   DATA
void CamUsbIo::GetImageData( std::vector<uint16_t> & data )
{
     //allocate the proper buffer for the camera type
    const int32_t PadSize = GetPadding( apgHelper::SizeT2Int32(data.size()) );

    if( PadSize )
    {
        data.resize( data.size() + PadSize );
    }
   
    uint32_t NumBytesExpected = 
        apgHelper::SizeT2Uint32( data.size() ) * sizeof(uint16_t);
    std::vector<uint16_t>::iterator iter = data.begin();

    while( NumBytesExpected > 0 )
    {
        uint32_t SizeToRead = std::min<uint32_t>(NumBytesExpected,
            m_MaxBufSize );

        uint32_t ReceivedSize = 0;

        m_Usb->ReadImage(&(*iter),SizeToRead,ReceivedSize);

        NumBytesExpected -= ReceivedSize;
        
        if( ReceivedSize != SizeToRead )
        {
            break;
        }
        
        iter += ReceivedSize / sizeof(uint16_t);
    }

    if( NumBytesExpected )
    {
        const uint32_t TotalBytes = 
             apgHelper::SizeT2Uint32( data.size() ) * sizeof(uint16_t);
        const uint32_t  DownloadedBytes = TotalBytes - NumBytesExpected;
        std::stringstream msg;
        msg << "GetImageData error - Expected " << data.size()*sizeof(uint16_t) << " bytes.";
        msg << "  Downloaded " <<  DownloadedBytes << " bytes.";
        msg << "  " << NumBytesExpected << " bytes remaining.";
        
        apgHelper::throwRuntimeException( m_fileName, msg.str(), 
            __LINE__, Apg::ErrorType_Critical );
    }

    //remove the padded data if it exists
    if( 0 < PadSize )
    {
        int32_t offset =  
            apgHelper::SizeT2Int32( data.size() ) - PadSize;
        std::vector<uint16_t>::iterator iter = data.begin()+offset;
        data.erase( iter, data.end() );
    }
    
}

//////////////////////////// 
// GET  STATUS
void CamUsbIo::GetStatus(CameraStatusRegs::BasicStatus & status)
{
    m_Usb->GetStatus( reinterpret_cast<uint8_t*>(&status),
        sizeof(status) );
}
        
//////////////////////////// 
// GET  STATUS
void CamUsbIo::GetStatus(CameraStatusRegs::AdvStatus & status)
{
	int MaxRetries = 2;
    for (int RetryNumber = 0; RetryNumber <= MaxRetries; RetryNumber++)
	{
		try
		{
			m_Usb->GetStatus( reinterpret_cast<uint8_t*>(&status),
				sizeof(status) );
			break;
		}
		catch(std::exception & err )
		{
			if ( RetryNumber == MaxRetries )
			{
                std::stringstream msg;
				msg << "CamUsbIo::GetStatus (adv) failed after ";
				msg << RetryNumber << " retries with error ";
				msg <<  err.what();
				apgHelper::throwRuntimeException( m_fileName, msg.str(), 
					__LINE__, Apg::ErrorType_Critical );
			}
		}
	}
}

//////////////////////////// 
// GET       FIRMWARE    REV
uint16_t CamUsbIo::GetFirmwareRev()
{
    return ReadReg( CameraRegs::FIRMWARE_REV );
}

//////////////////////////// 
// GET  USB        FIRMWARE        VERSION
std::string CamUsbIo::GetUsbFirmwareVersion()
 {
     std::vector<char> data(UsbFrmwr::REV_LENGTH+1, 0);

     m_Usb->GetUsbFirmwareVersion( reinterpret_cast<int8_t*>(&data.at(0)), UsbFrmwr::REV_LENGTH );

    std::string version( &data.at(0) );

    return version;
 }

//////////////////////////// 
//  GET  INFO
std::string CamUsbIo::GetInfo()
{
     uint16_t vid, pid, did = 0;
    GetUsbVendorInfo( vid, pid, did );

    std::stringstream result;

    result << "Interface: USB\n";
    result << "Camera Firmware: " << GetFirmwareRev() << "\n";
    result << "USB Firmware: " << GetUsbFirmwareVersion() << "\n";
    result << "USB Vendor ID: " << vid << "\n";
    result << "USB Prodcut ID: " << pid << "\n";
    result << "USB Device ID: " << did << "\n";
    result << "USB Driver Version: " << GetDriverVersion() << "\n";
    result << "USB Device Number: " << m_Usb->GetDeviceNum() << "\n";
      
    return result.str();
}

//////////////////////////// 
// READ       BUFCON          REG 
uint8_t CamUsbIo::ReadBufConReg( uint16_t reg )
{
    uint8_t value = 0;
    m_Usb->UsbRequestIn( VND_APOGEE_BUFCON_REG, reg, 0,
        &value, sizeof(uint8_t) );
    return value;
}
	    

//////////////////////////// 
// WRITE       BUFCON          REG 
void CamUsbIo::WriteBufConReg( const uint16_t reg, 
                                uint8_t val )
{
    m_Usb->UsbRequestOut( VND_APOGEE_BUFCON_REG, reg, 0, 
        reinterpret_cast<uint8_t*>(&val), sizeof(uint8_t) );
}
        
//////////////////////////// 
// READ       FX2          REG 
uint8_t CamUsbIo::ReadFx2Reg( uint16_t reg )
{
     uint8_t value = 0;
    m_Usb->UsbRequestIn( VND_APOGEE_MEMRW, reg, 0,
        &value, sizeof(uint8_t) );
    return value;
}

//////////////////////////// 
// WRITE       FX2         REG 
void CamUsbIo::WriteFx2Reg( uint16_t reg, uint8_t val )
{
    m_Usb->UsbRequestOut( VND_APOGEE_MEMRW, reg, 0, 
        &val, sizeof(uint8_t) );

}


//////////////////////////// 
//  GET    PADDING
int32_t CamUsbIo::GetPadding( const int32_t Num )
{

    // if we don't need to pad the data
    // bail here
    if( !m_ApplyPadding )
    {
        return 0;
    }

    //if the output buffer is not divisible by
    //8 we can get an error in the usb data
    //so we pad the buffer
    int32_t PadSize = 8 - (Num % 8);

    if( 8 == PadSize )
    {
        PadSize = 0;
    }

    return PadSize;
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string CamUsbIo::GetDriverVersion()
{
    return m_Usb->GetDriverVersion();
}

//////////////////////////// 
// IS      ERROR
bool CamUsbIo::IsError()
{
    return m_Usb->IsError();
}


//////////////////////////// 
// PROGRESS        2          STDOUT
void CamUsbIo::Progress2StdOut(const int32_t percentComplete)
{
    if( m_Print2StdOut )
    {
        std::cout << "Precent:" << percentComplete << std::endl;
    }
}
