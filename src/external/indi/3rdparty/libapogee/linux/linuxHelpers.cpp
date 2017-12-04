/*! 
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this file,
* You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Copyright(c) 2010 Apogee Instruments, Inc. 
* \namespace linuxHelpers
* \brief helper class for Linux OSs
* 
*/ 

#include "linuxHelpers.h"
#include "../CamHelpers.h" // for UsbFrmwr namespace
#include "../apgHelper.h"
#include <libusb-1.0/libusb.h>
#include <sstream>

////////////////////////////
// GET		DEVICES		LINUX
std::vector< std::vector<uint16_t> > linuxHelpers::GetDevicesLinux()
{
	libusb_context * ctxt = NULL;

	int32_t result = libusb_init( &ctxt );

	if( result )
	{
		std::stringstream ss;
		ss << "libusb_init failed with error = " << result;
		apgHelper::throwRuntimeException(__FILE__, ss.str(), 
            __LINE__, Apg::ErrorType_Connection );
	}

	try
	{
		libusb_device **devs = NULL;

		//get the list of usb devices
		const int32_t cnt = libusb_get_device_list(ctxt, &devs);

		std::vector< std::vector<uint16_t> > deviceVect;

		//search for and save apogee devices
		for(int32_t i=0; i < cnt; ++i)
		{
			libusb_device_descriptor desc;
			result = libusb_get_device_descriptor(devs[i], &desc);
			if (result < 0)
			{
				//error, ignore and go to next device
				continue;
			}

			if( UsbFrmwr::IsApgDevice(desc.idVendor, desc.idProduct) )
			{
				//save the values
                std::vector<uint16_t> temp; 
                temp.push_back( libusb_get_device_address(devs[i]) );
                temp.push_back( desc.idVendor );
                temp.push_back( desc.idProduct );

				deviceVect.push_back( temp );
			}
		}


		libusb_free_device_list( devs, 1 );
		libusb_exit( ctxt );

		return deviceVect;
	}
	catch(std::exception & err)
	{
		//clean up before exiting
		//wish c++ had a final like python...
		libusb_exit( ctxt );
		std::vector< std::vector<uint16_t> > nothing;
		return nothing;
	}

}
