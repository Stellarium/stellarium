/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class GenTwoWinUSB 
* \brief USB IO class that communicates with the WinUSB driver 
* 
*/ 

#include <atlstr.h>
#include "GenTwoWinUSB.h" 
#include <sstream>

#include "apgHelper.h" 
#include "windozeHelpers.h" 
#include "ApogeeIoctl.h"
#include "ApnUsbSys.h"
#include "CamHelpers.h" 
#include "ApgLogger.h" 


namespace
{
    const UCHAR BMRT_VENDOR_DEVICE_OUT = 0x40;
    const UCHAR BMRT_VENDOR_DEVICE_IN = 0xC0;
}


//////////////////////////// 
// CTOR 
GenTwoWinUSB::GenTwoWinUSB( const uint16_t DeviceNum ) : m_device( NULL ),
                                                               m_DeviceName(""),
                                                               m_fileName( __FILE__ ),
                                                               m_ReadImgError( false ),
                                                               m_IoError( false ),
                                                               m_DeviceNum( DeviceNum ),
                                                               m_DriverVersion( "null" )
{

    CString Path;
    windozeHelpers::FetchUsbDevicePath( DeviceNum, Path );

    windozeHelpers::FetchUsbDriverVersion( DeviceNum, m_DriverVersion );

    // Open the driver
    m_device = CreateFile( Path,
        GENERIC_WRITE | GENERIC_READ,
        FILE_SHARE_WRITE | FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL );

    if (INVALID_HANDLE_VALUE == m_device )
    {
        std::stringstream numSs;
        numSs << DeviceNum;
        std::string errMsg = "Failed to create usb device handle to dev num " + numSs.str() + " with error "
            + windozeHelpers::GetLastWinError();
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection );
    }

    if( !WinUsb_Initialize( m_device, &m_winusb ) )
    {
        std::stringstream numSs;
        numSs << DeviceNum;
        std::string errMsg = "Failed to create WinUSB wrapper to dev num " + numSs.str() + " with error "
            + windozeHelpers::GetLastWinError();
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Connection );
    }

    unsigned long BytesReceived;
    WinUsb_GetDescriptor(
        m_winusb,
        USB_DEVICE_DESCRIPTOR_TYPE,
        0, 0,
        (PUCHAR)&m_Descriptor,
        sizeof(m_Descriptor),
        &BytesReceived
    );

    //log what device we have connected to
    std::stringstream ss;
    ss << "Connection to device " << m_DeviceNum << " is open.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      ss.str() ); 
}
 

//////////////////////////// 
// DTOR 
GenTwoWinUSB::~GenTwoWinUSB() 
{ 
    // log that we are exiting on a error
    if( m_ReadImgError ||  m_IoError )
    {
        std::stringstream err;
        err << "Exiting GenTwoWinUSB in an error state. ";
        err << "read error = " << m_ReadImgError;
        err << ", io error = " << m_IoError ;
         apgHelper::LogErrorMsg( m_fileName, 
             err.str() , __LINE__ );
    }

    // WinUsb does not support port reset
    // so if this is a read image error call abort
    // not sure if there is a way to clear the control transfer pipe....
    if( m_ReadImgError )
    {
        const BOOL result = WinUsb_AbortPipe( m_winusb, UsbFrmwr::END_POINT );

        if( !result )
        {
            std::string msg("WinUsb_AbortPipe falied with error ");
            msg.append( windozeHelpers::GetLastWinError() );
            apgHelper::LogErrorMsg( m_fileName, msg, __LINE__ );
        }
    }

    WinUsb_Free( m_winusb );
    CloseHandle( m_device );

    //log what device we have disconnected from
    std::stringstream ss;
    ss << "Connection to device " << m_DeviceNum << " is closed.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      ss.str() ); 
} 

//////////////////////////// 
//      VENDOR    REQUEST
void GenTwoWinUSB::VendorRequest( 
    Direction whichWay,
    unsigned char bRequest,
    unsigned short wValue,
    unsigned short wIndex,
    unsigned char * Buffer,
    size_t BufferSize
)
{
    unsigned long BytesReceived = 0;

    WINUSB_SETUP_PACKET wsp = {
        whichWay == In 
            ? BMRT_VENDOR_DEVICE_IN 
            : BMRT_VENDOR_DEVICE_OUT,
        bRequest,
        wValue,
        wIndex,
        apgHelper::SizeT2Uint16( BufferSize )
    };

    if( !WinUsb_ControlTransfer( m_winusb, wsp, Buffer, 
        apgHelper::SizeT2Uint32( BufferSize ), &BytesReceived, NULL ) )
    {
        m_IoError = true;
        std::stringstream err;
		err << "VendorRequest failed with error ";
		err <<  windozeHelpers::GetLastWinError().c_str();
        err << " : RequestCode = " << std::hex << static_cast<int32_t>(bRequest);
        err << " : Index = " << wIndex << " : Value = " << wValue << " : Direction = " << whichWay;
		apgHelper::throwRuntimeException( m_fileName, err.str(), 
            __LINE__, Apg::ErrorType_Critical );
    }

    if( BytesReceived != BufferSize ) 
    {
        m_IoError = true;
        std::stringstream errMsg;
        errMsg << "VendorRequest error - ";
        errMsg << BufferSize << " bytes requested, ";
        errMsg << BytesReceived << " bytes received.";
        errMsg << " : RequestCode = " << std::hex << static_cast<int32_t>(bRequest);
        errMsg <<  " : Index = " << wIndex << " : Value = " << wValue << " : Direction = " << whichWay;
        apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
            __LINE__, Apg::ErrorType_Critical );
    }

    m_IoError = false;
}

//////////////////////////// 
// READ     REG 
uint16_t GenTwoWinUSB::ReadReg(uint16_t FpgaReg)
{
    uint16_t FpgaData = 0;

    VendorRequest( In,                                         // direction
        VND_APOGEE_CAMCON_REG,             // request
        0,                                                                 // value
        FpgaReg,                                                    // index
        (PUCHAR)&FpgaData, sizeof(uint16_t)     // buffer
    );

    return FpgaData;
}

//////////////////////////// 
// WRITE        REG 
void GenTwoWinUSB::WriteReg(const uint16_t FpgaReg, 
                            const uint16_t FpgaData )
{
    VendorRequest( Out,                                          // direction
        VND_APOGEE_CAMCON_REG,                 // request
        0,                                                                     // value
        FpgaReg,                                                        // index
        (PUCHAR)&FpgaData, sizeof(uint16_t)         // buffer
    );

}

//////////////////////////// 
// GET     VENDOR      INFO 
void GenTwoWinUSB::GetVendorInfo(
    uint16_t & VendorId,
    uint16_t & ProductId,
    uint16_t & DeviceId
)
{
    VendorId	= m_Descriptor.idVendor;
    ProductId	= m_Descriptor.idProduct;
    DeviceId	= m_Descriptor.bcdDevice;
}

//////////////////////////// 
// SETUP     SEQUENCE       IMG     XFER
void GenTwoWinUSB::SetupSingleImgXfer( const uint16_t Rows,
                                        const uint32_t Cols)
{
    ULONG ImageSize =Rows*Cols;

    if( ImageSize <= 0 )
    {
        std::string errMsg("Invalid input image parameters");
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    // Does Alta bcdDevice < 0x0010 really need this marked In?

    VendorRequest( Out,
        VND_APOGEE_GET_IMAGE,
        HIWORD(ImageSize), 
        LOWORD(ImageSize),
        NULL, 0
        // (PUCHAR)&ImageSize, sizeof(ULONG)
    );
}

//////////////////////////// 
// SETUP     SEQUENCE       IMG     XFER
void GenTwoWinUSB::SetupSequenceImgXfer( const uint16_t Rows,
                                          const uint16_t Cols,
                                          const uint16_t NumOfImages)
{
    ULONG ImageSize = Rows*Cols;

    // You know, an unsigned long cannot be < 0.

    if( ImageSize <= 0 )
    {
        std::string errMsg("Invalid input image parameters");
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
    }

    uint8_t Data[3] = {
        LOBYTE(NumOfImages),
        HIBYTE(NumOfImages),
        0
    };

    VendorRequest( Out, 
        VND_APOGEE_GET_IMAGE,
        HIWORD(ImageSize),
        LOWORD(ImageSize),
        Data, 3
    );
}


////////////////////////////
// CANCEL      IMG         XFER
void GenTwoWinUSB::CancelImgXfer()
{
    UsbRequestOut( VND_APOGEE_STOP_IMAGE,
        0, 0, NULL, 0 );
}


//////////////////////////// 
// READ    IMAGE
void GenTwoWinUSB::ReadImage( uint16_t * ImageData, 
                               const uint32_t InSizeInBytes,
                               uint32_t &OutSizeInBytes)
{
#if 1
    ULONG Yes = 1;
    WinUsb_SetPipePolicy( m_winusb, UsbFrmwr::END_POINT, 
        ALLOW_PARTIAL_READS, sizeof(ULONG), &Yes );
    WinUsb_SetPipePolicy( m_winusb, UsbFrmwr::END_POINT,
        AUTO_FLUSH, sizeof(ULONG), &Yes );
    WinUsb_SetPipePolicy( m_winusb, UsbFrmwr::END_POINT,
        AUTO_CLEAR_STALL, sizeof(ULONG), &Yes );
#endif

    const BOOL Success = WinUsb_ReadPipe( 
        m_winusb, UsbFrmwr::END_POINT, 
        (PUCHAR)ImageData, InSizeInBytes, 
        (PULONG)&OutSizeInBytes,      //is this safe 64 v. 32 bit?
        NULL
    );


    if( !Success  )
    {
        m_ReadImgError = true;
        std::string errMsg = "WinUsb_ReadPipe failed with error "
            + windozeHelpers::GetLastWinError() + ".";
        apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_Critical );
    }

    m_ReadImgError = false;
}

//////////////////////////// 
// GET  STATUS
void GenTwoWinUSB::GetStatus(uint8_t * status, uint32_t NumBytes)
{
    VendorRequest( In, 
        VND_APOGEE_STATUS,
        0,
        0,
        status, NumBytes
    );
}


//////////////////////////// 
// APN      USB        REQUEST      IN
void GenTwoWinUSB::UsbRequestIn(const uint8_t RequestCode, 
                                 const uint16_t Index, const uint16_t Value, 
                                 uint8_t * ioBuf, uint32_t BufSzInBytes) 
{    
    VendorRequest( In,
        RequestCode,
        Value,
        Index,
        ioBuf, BufSzInBytes
    );
}

//////////////////////////// 
// APN      USB        REQUEST      OUT
void GenTwoWinUSB::UsbRequestOut(const uint8_t RequestCode, 
                                 const uint16_t Index, const uint16_t Value, 
                                 const uint8_t * ioBuf, const uint32_t BufSzInBytes) 
{
    VendorRequest( Out,
        RequestCode,
        Value,
        Index,
        const_cast<PUCHAR>(ioBuf), BufSzInBytes
    );

}

//////////////////////////// 
// GET     SERIAL   NUMBER
void GenTwoWinUSB::GetSerialNumber(int8_t * ioBuf, const uint32_t BufSzInBytes)
{
    // Not supported for ALTA below APUSB_CUSTOM_SN_DID_SUPPORT.....

    if( 
        m_Descriptor.idProduct == UsbFrmwr::ALTA_USB_PID &&
        m_Descriptor.bcdDevice < APUSB_CUSTOM_SN_DID_SUPPORT
    )
    {
        ZeroMemory( ioBuf, BufSzInBytes );
        return;
    }

    VendorRequest( In,
        VND_APOGEE_CAMCON_REG,
        1,
        0,
        (PUCHAR)ioBuf,
        min( BufSzInBytes, Eeprom::MAX_SERIAL_NUM_BYTES)
    );
}


//////////////////////////// 
// GET  USB        FIRMWARE        VERSION
void GenTwoWinUSB::GetUsbFirmwareVersion(int8_t  * ioBuf, const uint32_t BufSzInBytes)
{
    // Not supported for ALTA below APUSB_8051_REV_DID_SUPPORT.....

    if( 
        m_Descriptor.idProduct == UsbFrmwr::ALTA_USB_PID &&
        m_Descriptor.bcdDevice < APUSB_8051_REV_DID_SUPPORT
    )
    {
        ZeroMemory( ioBuf, BufSzInBytes );
        return;
    }

   VendorRequest( In,
        VND_APOGEE_CAMCON_REG,
        2,
        0,
        (PUCHAR)ioBuf,
        min( BufSzInBytes, APUSB_8051_REV_BYTE_COUNT)
    );
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string GenTwoWinUSB::GetDriverVersion()
{
    return m_DriverVersion;
}

//////////////////////////// 
//      IS     ERROR
bool GenTwoWinUSB::IsError()
{
    // only check the ctrl xfer error, because
    // other error handling *should* clear the
    // m_ReadImgError
    return ( m_IoError ? true : false );
}

//////////////////////////// 
//      USB    REQ    OUT     WITH      EXTENDED        TIMEOUT
void GenTwoWinUSB::UsbReqOutWithExtendedTimeout(uint8_t RequestCode,
            uint16_t Index, uint16_t	Value,
            const uint8_t * ioBuf, uint32_t BufSzInBytes)
{

    //30 sec works with the clear flash cmd for alta g cameras
    ULONG TimeoutInMs = 30000;
    WinUsb_SetPipePolicy( m_winusb, 
        0x0, 
        PIPE_TRANSFER_TIMEOUT, 
        sizeof(ULONG), 
        &TimeoutInMs );

    UsbRequestOut( RequestCode, Index, Value, ioBuf, BufSzInBytes );

    // reseting to the default of 5 secs
    TimeoutInMs = 5000;
    WinUsb_SetPipePolicy( m_winusb, 
        0x0, 
        PIPE_TRANSFER_TIMEOUT, 
        sizeof(ULONG), 
        &TimeoutInMs );

}


//////////////////////////// 
//      READ      SERIAL
void GenTwoWinUSB::ReadSerialPort( const uint16_t PortId, 
                                  uint8_t * ioBuf, const uint16_t BufSzInBytes )
{
    
    unsigned long BytesReceived = 0;

    WINUSB_SETUP_PACKET wsp = {
        BMRT_VENDOR_DEVICE_IN,
        VND_APOGEE_SERIAL,
        0,
        PortId,
        BufSzInBytes
    };

    if( !WinUsb_ControlTransfer( m_winusb, 
        wsp, 
        const_cast<PUCHAR>(ioBuf), 
        BufSzInBytes, 
        &BytesReceived, 
        NULL ) )
    {
        m_IoError = true;
        std::stringstream err;
		err << "ReadSerialPort failed with error ";
		err <<  windozeHelpers::GetLastWinError().c_str();
		apgHelper::throwRuntimeException( m_fileName, err.str(), 
            __LINE__, Apg::ErrorType_Critical );
    }

    //the buffer size may be different, so no checking in this function
}