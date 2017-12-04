/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \class GenOneLinuxUSB 
* \brief Usb interface for *nix systems. 
* 
*/ 

#include "GenOneLinuxUSB.h" 
#include "../ApnUsbSys.h"
#include "../apgHelper.h"
#include "../CamHelpers.h"  // for UsbFrmwr namespace
#include "../helpers.h"
#include "../ApgLogger.h" 
#include <cstring>
#include <sstream>

namespace
{
    // this sets the timeout to infinte on the libusb_bulk_transfer funtion
    const uint32_t BULK_XFER_TIMEOUT = 0;  
    const uint32_t TIMEOUT = 10000;  
    const int32_t INTERFACE_NUM = 0x0;

}

//////////////////////////// 
// CTOR
GenOneLinuxUSB::GenOneLinuxUSB(const uint16_t DeviceNum ) : m_Context( NULL ),
																  m_Device( NULL ),
																  m_fileName( __FILE__ ),
                                                                  m_ReadImgError( false ),
                                                                  m_IoError( false ),
                                                                  m_NumRegWriteRetries( 0 ),
                                                                  m_DeviceNum( DeviceNum )
{

	int32_t result = libusb_init( &m_Context );
	if( result )
	{
		std::stringstream ss;
		ss << "libusb_init failed with error = " << result;
		apgHelper::throwRuntimeException(m_fileName, ss.str(), 
            __LINE__, Apg::ErrorType_Connection );
	}

	std::string errMsg;

	if( !OpenDeviceHandle(DeviceNum, errMsg) )
	{
		//failed to find device clean up and throw
		libusb_exit( m_Context);

		apgHelper::throwRuntimeException( m_fileName,
				errMsg, __LINE__, Apg::ErrorType_Connection );
	}

    //claim the interface
    int32_t getInterface = libusb_claim_interface( m_Device, INTERFACE_NUM );

    if( 0 != getInterface )
    {
        //clean up
        libusb_close( m_Device );
        libusb_exit( m_Context );

        //die
        apgHelper::throwRuntimeException( m_fileName,
				"failed to claim usb interface", __LINE__, 
                Apg::ErrorType_Connection );
    }

    //log what device we have connected to
    std::stringstream ss;
    ss << "Connection to device " << m_DeviceNum << " is open.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      ss.str() ); 

}

////////////////////////////
// DTOR 
GenOneLinuxUSB::~GenOneLinuxUSB() 
{ 
    int32_t result = 0;
    // if we are in an error state call a reset on the 
    // the USB port
    if( m_ReadImgError ||  m_IoError )
    {
         apgHelper::LogErrorMsg( m_fileName, 
             "Exiting in an error state.  Calling reset device to attempt to clear the USB error.", __LINE__ );

        result = libusb_reset_device( m_Device );

        if( 0 != result )
        {
            std::stringstream ss;
			ss << "libusb_reset_device error = " << result;
            apgHelper::LogErrorMsg( m_fileName, ss.str(), __LINE__ );
        }
        else
        {
             apgHelper::LogErrorMsg( m_fileName, "libusb_reset_device successful.", __LINE__ );
        }

    }

    result = libusb_release_interface( m_Device, INTERFACE_NUM );

    if( 0 != result )
    {
        std::stringstream ss;
        ss << "libusb_release_interface error = " << result;
        apgHelper::LogErrorMsg( m_fileName, ss.str(), __LINE__ );
    }

	libusb_close( m_Device );

	libusb_exit( m_Context );

    //log what device we have disconnected from
    std::stringstream ss;
    ss << "Connection to device " << m_DeviceNum << " is closed.";
    ApgLogger::Instance().Write(ApgLogger::LEVEL_RELEASE,"info",
      ss.str() ); 

} 

////////////////////////////
//	OPEN		DEVICE		HANDLE
bool GenOneLinuxUSB::OpenDeviceHandle(const uint16_t DeviceNum,
		std::string & err)
{
	//pointer to pointer of device, used to retrieve a list of devices
	libusb_device **devs = NULL;

	//get the list of devices
	const int32_t count = libusb_get_device_list(m_Context, &devs);

	bool result = false;
	int32_t i=0;
	for(; i < count; ++i)
	{
		libusb_device_descriptor desc;
		int32_t r = libusb_get_device_descriptor(devs[i], &desc);
		if (r < 0)
		{
			//ignore and go to the next device
			continue;
		}

		//are we an apogee device
		if( UsbFrmwr::IsApgDevice(desc.idVendor, desc.idProduct) )
		{
			const uint16_t num = libusb_get_device_address(devs[i]);
			if( num == DeviceNum)
			{
				//we found the device we want try to open
				//handle to it
				int32_t notOpened = libusb_open(devs[i], &m_Device);

				if( notOpened )
				{
					std::stringstream ss;
					ss << "libusb_open error = " << notOpened;
					err = ss.str();
					//bail out of for loop here
					break;
				}

				//save the descriptor information
				memcpy( &m_DeviceDescriptor, &desc, sizeof(libusb_device_descriptor) );

				//set the return value and break out of for loop
				result = true;
				break;

			}
		}
	}

	//see if we found a device
	if( count == i )
	{
		err.append("No device found");
	}

	libusb_free_device_list(devs, 1);


	return result;

}

////////////////////////////
// READ     REG
uint16_t GenOneLinuxUSB::ReadReg(const uint16_t FpgaReg)
{
	uint16_t FpgaData = 0;
	UsbRequestIn(
				VND_APOGEE_CAMCON_REG,
				FpgaReg,
				0,
				reinterpret_cast<uint8_t*>(&FpgaData),
				sizeof(uint16_t)
				);

	return FpgaData;
}

////////////////////////////
// WRITE        REG
void GenOneLinuxUSB::WriteReg(uint16_t FpgaReg,
	const uint16_t FpgaData )
{

    try
    {
	    UsbRequestOut(
			    VND_APOGEE_CAMCON_REG,
			    FpgaReg,
			    0,
			    reinterpret_cast<const uint8_t*>(&FpgaData),
			    sizeof(uint16_t)
			    );
    }
    catch( std::runtime_error & err)
    {
        // if this is a time out error retry to send the command
        // once
        const std::string timeoutErr("error -7");
        const std::string whatStr( err.what() );

        //this isn't a -7 error throw
        if( std::string::npos == whatStr.find( timeoutErr ) )
        {
            throw;
        }

        // if this is the first error, log and retry it, if not throw
        if( m_NumRegWriteRetries < 1 )
        {
             apgHelper::LogErrorMsg( m_fileName, "USB timeout error in reg write, retrying the cmd.", __LINE__ );
            ++m_NumRegWriteRetries;
            WriteReg( FpgaReg, FpgaData );
             m_NumRegWriteRetries = 0;
        }
        else
        {
            throw;
        }      
    }
    
}

////////////////////////////
// GET     VENDOR      INFO
void GenOneLinuxUSB::GetVendorInfo(uint16_t & VendorId,
	uint16_t & ProductId, uint16_t  & DeviceId)
{

	VendorId = m_DeviceDescriptor.idVendor;
	ProductId = m_DeviceDescriptor.idProduct;
	DeviceId = m_DeviceDescriptor.bcdDevice;

}

////////////////////////////
// SETUP     SINGLE          IMG         XFER
void GenOneLinuxUSB::SetupSingleImgXfer(
		const uint16_t Rows,
		const uint32_t Cols)
{
	const uint32_t ImageSize = Rows*Cols;

	if( ImageSize <= 0 )
	{
		std::string errMsg("Invalid input image parameters");
		apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
	}

	const uint16_t Index = help::GetLowWord( ImageSize );
	const uint16_t Value = help::GetHighWord( ImageSize );
	UsbRequestOut(
				VND_APOGEE_GET_IMAGE,
				Index,
				Value,
				NULL,
				0
				);
}

////////////////////////////
// SETUP         SEQUENCE       IMG         XFER
void GenOneLinuxUSB::SetupSequenceImgXfer(
		const uint16_t Rows,
		const uint16_t Cols,
		const uint16_t NumOfImages)
{
	const uint32_t ImageSize = Rows*Cols;

	if( ImageSize <= 0 )
	{
		std::string errMsg("Invalid input image parameters");
		apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
	}

	if( NumOfImages <= 0 )
	{
		std::string errMsg("Invalid number of images");
		apgHelper::throwRuntimeException( m_fileName, errMsg, 
            __LINE__, Apg::ErrorType_InvalidUsage );
	}


	const uint8_t DeviceData[3] = { help::GetLowByte(NumOfImages),
			help::GetHighByte(NumOfImages), 0 };

	UsbRequestOut(
				VND_APOGEE_GET_IMAGE,  		//request code
				help::GetLowWord( ImageSize ),	//index
				help::GetHighWord( ImageSize ),	//value
				&DeviceData[0], 			//iobuffer
				sizeof( DeviceData )		//buffer size in bytes
				);

}


////////////////////////////
// CANCEL      IMG         XFER
void GenOneLinuxUSB::CancelImgXfer()
{
    UsbRequestOut( VND_APOGEE_STOP_IMAGE,
        0, 0, NULL, 0 );

    // if there was an error on the read image
    // try to clear an halts on the ep.  no automatic
    // way to do this like in windows.
    if( m_ReadImgError )
    {
        const int32_t result = libusb_clear_halt(
			m_Device, 		                        //device handle
			UsbFrmwr::END_POINT); 		// end point

        if( result < 0 )
        {
            std::stringstream err;
            err << "libusb_clear_halt failed with error ";
            err << result << ".  ";

            m_IoError = true;
            apgHelper::throwRuntimeException( 
                m_fileName, err.str(), __LINE__, 
                Apg::ErrorType_Critical );
        }
    }
}

////////////////////////////
// READ    IMAGE
void GenOneLinuxUSB::ReadImage(uint16_t * ImageData,
			   const uint32_t InSizeInBytes,
			   uint32_t &OutSizeInBytes)
{
	const int32_t result = libusb_bulk_transfer(
			m_Device, 		//device handle
			UsbFrmwr::END_POINT, 		// end point
			reinterpret_cast<uint8_t*>(ImageData),  	// data pointer
			InSizeInBytes,  // length
			reinterpret_cast<int32_t*>(&OutSizeInBytes), // number of bytes transfered
			BULK_XFER_TIMEOUT );		// time out

	if( result < 0 )
	{
		std::stringstream err;
		err << "ReadImage failed with error ";
		err << result << ".  ";

        //from http://libusb.sourceforge.net/api-1.0/group__syncio.html
        // "Also check transferred when dealing with a timeout error code. 
        // libusb may have to split your transfer into a number of chunks to 
        // satisfy underlying O/S requirements, meaning that the timeout 
        // may expire after the first few chunks have completed. libusb is 
        // careful not to lose any data that may have been transferred; do not 
        // assume that timeout conditions indicate a complete lack of I/O."
        if( -7 == result )
        {
            err << "Number bytes transfered on time out = " << OutSizeInBytes << ".";
        }

        m_ReadImgError = true;
		apgHelper::throwRuntimeException( m_fileName, err.str(), 
            __LINE__, Apg::ErrorType_Critical );
	}

	#ifdef ALT_BULKIO_CHECK
		if( InSizeInBytes != OutSizeInBytes )
		{
			const int32_t diff = InSizeInBytes - OutSizeInBytes;
			
			// if the difference in size is greater than 512 bytes
			// throw an exception
			if( diff > 512 )
			{
				m_ReadImgError = true;
				std::stringstream errMsg;
				errMsg << "libusb_bulk_transfer error - number bytes expected = ";
				errMsg << InSizeInBytes << ", number of bytes received = " << OutSizeInBytes;
				apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
					__LINE__, Apg::ErrorType_Critical );
			}
			
			//log that we are missing bytes from the transfer
			//and let the transfer proceed
			std::stringstream warnMsg;
			warnMsg << "libusb_bulk_transfer warning - number bytes expected = ";
			warnMsg << InSizeInBytes << ", number of bytes received = " << OutSizeInBytes;
							
			apgHelper::LogWarningMsg( m_fileName, warnMsg.str() , __LINE__ );
		}
	#else
		if( InSizeInBytes != OutSizeInBytes )
		{
			m_ReadImgError = true;
			std::stringstream errMsg;
			errMsg << "libusb_bulk_transfer error - number bytes expected = ";
			errMsg << InSizeInBytes << ", number of bytes received = " << OutSizeInBytes;
			apgHelper::throwRuntimeException( m_fileName, errMsg.str(), 
				__LINE__, Apg::ErrorType_Critical );
		}
	#endif
	
    m_ReadImgError = false;
}


////////////////////////////
// GET  STATUS
void GenOneLinuxUSB::GetStatus(uint8_t * status, uint32_t NumBytes)
{
	UsbRequestIn( VND_APOGEE_STATUS, 0, 0, status, NumBytes );
}


////////////////////////////
// APN      USB        REQUEST      IN
void GenOneLinuxUSB::UsbRequestIn(uint8_t RequestCode,
		uint16_t	Index, uint16_t	Value,
		uint8_t * ioBuf, uint32_t BufSzInBytes)
{
	const int32_t result = libusb_control_transfer(
				m_Device,
				LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
				RequestCode,
				Value,
				Index,
				ioBuf,
				BufSzInBytes,
				TIMEOUT);

	if( result < 0 )
	{
        m_IoError = true;
		std::stringstream err;
		err << "UsbRequestIn failed with error ";
		err << result << ".  ";
		err << "RequestCode = " << std::hex << static_cast<int32_t>(RequestCode);
		err << " : Index = " << Index << " : Value = " << Value;
		apgHelper::throwRuntimeException( m_fileName, err.str(), 
            __LINE__, Apg::ErrorType_Critical );
	}

    m_IoError = false;
}

////////////////////////////
// APN      USB        REQUEST      OUT
void GenOneLinuxUSB::UsbRequestOut(uint8_t RequestCode,
		uint16_t Index, uint16_t Value,
		const uint8_t * ioBuf, uint32_t BufSzInBytes)
{
	const int32_t result = libusb_control_transfer(
					m_Device,
					LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
					RequestCode,
					Value,
					Index,
					const_cast<uint8_t*>(ioBuf),
					BufSzInBytes,
					TIMEOUT);

	if( result < 0 )
	{
            m_IoError = true;
		    std::stringstream err;
		    err << "UsbRequestOut failed with error ";
		    err << result << ".  ";
		    err << "RequestCode = " << std::hex << static_cast<int32_t>(RequestCode);
		    err << " : Index = " << Index << " : Value = " << Value;
		    apgHelper::throwRuntimeException( m_fileName, err.str(), 
                __LINE__, Apg::ErrorType_Critical );
	}

    m_IoError = false;
}

////////////////////////////
// GET     SERIAL   NUMBER
void GenOneLinuxUSB::GetSerialNumber(int8_t * ioBuf, uint32_t BufSzInBytes)
{

	UsbRequestIn(VND_APOGEE_CAMCON_REG, 0, 1,
			reinterpret_cast<uint8_t*>(ioBuf), BufSzInBytes);

}

////////////////////////////
// GET  USB        FIRMWARE        VERSION
void GenOneLinuxUSB::GetUsbFirmwareVersion(int8_t * ioBuf, uint32_t BufSzInBytes)
{
	UsbRequestIn(VND_APOGEE_CAMCON_REG, 0, 2,
			reinterpret_cast<uint8_t*>(ioBuf), BufSzInBytes);
}

//////////////////////////// 
//      GET    DRIVER   VERSION
std::string GenOneLinuxUSB::GetDriverVersion()
{
    //TODO - what to for linux???
    std::string value( "libusb-1.0" );
    return value;
}

//////////////////////////// 
//      IS     ERROR
bool GenOneLinuxUSB::IsError()
{
    // only check the ctrl xfer error, because
    // other error handling *should* clear the
    // m_ReadImgError
    return ( m_IoError ? true : false );
}

//////////////////////////// 
//      USB    REQ    OUT     WITH      EXTENDED        TIMEOUT
void GenOneLinuxUSB::UsbReqOutWithExtendedTimeout(uint8_t RequestCode,
            uint16_t Index, uint16_t	Value,
            const uint8_t * ioBuf, uint32_t BufSzInBytes)
{

    const uint32_t EXTENDED_TIMEOUT = 30000;  
	const int32_t result = libusb_control_transfer(
					m_Device,
					LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE,
					RequestCode,
					Value,
					Index,
					const_cast<uint8_t*>(ioBuf),
					BufSzInBytes,
					EXTENDED_TIMEOUT );

	if( result < 0 )
	{
        m_IoError = true;
		std::stringstream err;
		err << "UsbRequestOut failed with error ";
		err << result << ".  ";
		err << "RequestCode = " << std::hex << static_cast<int32_t>(RequestCode);
		err << " : Index = " << Index << " : Value = " << Value;
		apgHelper::throwRuntimeException( m_fileName, err.str(), 
            __LINE__, Apg::ErrorType_Critical );
	}

    m_IoError = false;
}

//////////////////////////// 
//      READ      SERIAL
void GenOneLinuxUSB::ReadSerialPort( const uint16_t PortId, uint8_t * ioBuf, 
                                    const uint16_t BufSzInBytes )
{
    UsbRequestIn( VND_APOGEE_SERIAL, PortId, 0,
			reinterpret_cast<uint8_t*>(ioBuf), BufSzInBytes );
}
