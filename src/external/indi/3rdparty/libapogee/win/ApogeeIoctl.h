// ApogeeIoctl.h    Include file for I/O 
//
// Copyright (c) 2000-2004 Apogee Instruments, Inc.
//
// Define the IOCTL codes we will use.  The IOCTL code contains a command
// identifier, plus other information about the device, the type of access
// with which the file must have been opened, and the type of buffering.
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at http://mozilla.org/MPL/2.0/.
//////////////////////////////////////////////////////////////////////


// Device type           -- in the "User Defined" range."
#define GPD_TYPE		40000
#define WDM_TYPE		41000
#define PCI_SYS_TYPE	42000
#define USB_WDM_TYPE	43000

// The IOCTL function codes from 0x800 to 0xFFF are for customer use.


// IOCTL codes for an ISA device

// Read single word
#define IOCTL_GPD_READ_ISA_USHORT \
    CTL_CODE(GPD_TYPE, 0x901, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write single word
#define IOCTL_GPD_WRITE_ISA_USHORT \
    CTL_CODE(GPD_TYPE, 0x902, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Read line from camera
#define IOCTL_GPD_READ_ISA_LINE \
    CTL_CODE(GPD_TYPE, 0x903, METHOD_BUFFERED, FILE_READ_ACCESS)


// IOCTL codes for a PPI device

// Read single word
#define IOCTL_GPD_READ_PPI_USHORT \
    CTL_CODE(GPD_TYPE, 0x904, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write single word
#define IOCTL_GPD_WRITE_PPI_USHORT \
    CTL_CODE(GPD_TYPE, 0x905, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Read line from camera
#define IOCTL_GPD_READ_PPI_LINE \
    CTL_CODE(GPD_TYPE, 0x906, METHOD_BUFFERED, FILE_READ_ACCESS)


// IOCTL codes for a PCI device

// Read single word
#define IOCTL_GPD_READ_PCI_USHORT \
	CTL_CODE(GPD_TYPE, 0x907, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write single word
#define IOCTL_GPD_WRITE_PCI_USHORT \
	CTL_CODE(GPD_TYPE, 0x908, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Read line from camera
#define IOCTL_GPD_READ_PCI_LINE \
	CTL_CODE(GPD_TYPE, 0x909, METHOD_BUFFERED, FILE_READ_ACCESS)


/////////////////////////////////////////////////////////////////

// IOCTL codes for a PCI device using the WDM driver

// Read single word
#define IOCTL_WDM_READ_PCI_USHORT \
	CTL_CODE(WDM_TYPE, 0xA00, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write single word
#define IOCTL_WDM_WRITE_PCI_USHORT \
	CTL_CODE(WDM_TYPE, 0xA01, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Read line from camera
#define IOCTL_WDM_READ_PCI_LINE \
	CTL_CODE(WDM_TYPE, 0xA02, METHOD_BUFFERED, FILE_READ_ACCESS)


/////////////////////////////////////////////////////////////////

// IOCTL code for PCI bus scan
#define IOCTL_PCI_BUS_SCAN \
	CTL_CODE(PCI_SYS_TYPE, 0xB00, METHOD_BUFFERED, FILE_READ_ACCESS)


/////////////////////////////////////////////////////////////////

// IOCTL codes for a USB 2.0 device using the WDM driver (Alta)

// Read single word from camera controller
#define IOCTL_WDM_READ_USB_USHORT \
	CTL_CODE(USB_WDM_TYPE, 0xC00, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write single word from camera controller
#define IOCTL_WDM_WRITE_USB_USHORT \
	CTL_CODE(USB_WDM_TYPE, 0xC01, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Prime the image download process (Prior to actual exposure start)
#define IOCTL_WDM_PRIME_USB_DOWNLOAD \
	CTL_CODE(USB_WDM_TYPE, 0xC02, METHOD_BUFFERED, FILE_READ_ACCESS)

// Request the image data
#define IOCTL_WDM_READ_USB_IMAGE \
	CTL_CODE(USB_WDM_TYPE, 0xC03, METHOD_BUFFERED, FILE_READ_ACCESS)

// Get camera status information
#define IOCTL_WDM_USB_STATUS \
	CTL_CODE(USB_WDM_TYPE, 0xC04, METHOD_BUFFERED, FILE_READ_ACCESS)

// Stop an image in progress
#define IOCTL_WDM_STOP_USB_IMAGE \
	CTL_CODE(USB_WDM_TYPE, 0xC05, METHOD_BUFFERED, FILE_READ_ACCESS)

// Reset the USB interface control
#define IOCTL_WDM_USB_RESET \
	CTL_CODE(USB_WDM_TYPE, 0xC06, METHOD_BUFFERED, FILE_READ_ACCESS)

// Serial port read 
#define IOCTL_WDM_READ_USB_SERIAL \
	CTL_CODE(USB_WDM_TYPE, 0xC07, METHOD_BUFFERED, FILE_READ_ACCESS)

// Serial port write 
#define IOCTL_WDM_WRITE_USB_SERIAL \
	CTL_CODE(USB_WDM_TYPE, 0xC08, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Read serial port settings
#define IOCTL_WDM_READ_USB_SERIAL_SETTINGS \
	CTL_CODE(USB_WDM_TYPE, 0xC09, METHOD_BUFFERED, FILE_READ_ACCESS)

// Write serial port settings
#define IOCTL_WDM_WRITE_USB_SERIAL_SETTINGS \
	CTL_CODE(USB_WDM_TYPE, 0xC0A, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Get Vendor ID, Product ID, and Device ID
#define IOCTL_WDM_USB_VENDOR_INFO \
	CTL_CODE(USB_WDM_TYPE, 0xC0B, METHOD_BUFFERED, FILE_READ_ACCESS)

// Prime the image download process (Prior to actual sequence start)
#define IOCTL_WDM_PRIME_USB_SEQUENCE_DOWNLOAD \
	CTL_CODE(USB_WDM_TYPE, 0xC0C, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Prime a continuous imaging series
#define IOCTL_WDM_PRIME_CONTINUOUS_IMAGING \
	CTL_CODE(USB_WDM_TYPE, 0xC0D, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Prime a continuous imaging series
#define IOCTL_WDM_STOP_CONTINUOUS_IMAGING \
	CTL_CODE(USB_WDM_TYPE, 0xC0E, METHOD_BUFFERED, FILE_WRITE_ACCESS)

// Get custom OEM serial number string
#define IOCTL_WDM_READ_CUSTOM_OEM_SERIAL_NUMBER \
	CTL_CODE(USB_WDM_TYPE, 0xC0F, METHOD_BUFFERED, FILE_READ_ACCESS)



// 8051 USB Firmware revision
#define IOCTL_WDM_USB_8051_FIRMWARE_REVISION \
	CTL_CODE(USB_WDM_TYPE, 0xCFD, METHOD_BUFFERED, FILE_READ_ACCESS)

// Driver version number query
#define IOCTL_WDM_USB_DRIVER_VERSION \
	CTL_CODE(USB_WDM_TYPE, 0xCFE, METHOD_BUFFERED, FILE_READ_ACCESS)

// General purpose IOCTL for Apogee development and debug
#define IOCTL_WDM_USB_REQUEST \
	CTL_CODE(USB_WDM_TYPE, 0xCFF, METHOD_BUFFERED, FILE_READ_ACCESS)

