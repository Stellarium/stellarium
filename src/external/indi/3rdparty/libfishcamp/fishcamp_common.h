/*

  Copyright (c) 2001-2013 Fishcamp Engineering (support@fishcamp.com)

  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

        Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.

        Redistributions in binary form must reproduce the above
        copyright notice, this list of conditions and the following
        disclaimer in the documentation and/or other materials
        provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  POSSIBILITY OF SUCH DAMAGE.
  ======================================================================
*/

#ifndef FISHCAMP_COMMON
#define FISHCAMP_COMMON

#include <libusb-1.0/libusb.h>

typedef unsigned char  UInt8;
typedef signed char    SInt8;
typedef unsigned short UInt16;
typedef signed short   SInt16;
typedef unsigned long  UInt32;
typedef signed long    SInt32;

// USB ID codes follow
#define fishcamp_USB_VendorID        0x1887

// defines for prototype starfish guide camera
#define starfish_mono_proto_raw_deviceID   0x0001
#define starfish_mono_proto_final_deviceID 0x0000

// defines for prototype starfish guide camera w/ DMA logic
#define starfish_mono_proto2_raw_deviceID   0x0004
#define starfish_mono_proto2_final_deviceID 0x0005

// defines for REV2 (production) starfish guide camera
#define starfish_mono_rev2_raw_deviceID   0x0002
#define starfish_mono_rev2_final_deviceID 0x0003

// defines for IBIS Imager camera
#define starfish_ibis13_raw_deviceID   0x0006
#define starfish_ibis13_final_deviceID 0x0007

// defines for Starfish PRO 4M Imager camera
#define starfish_pro4m_raw_deviceID   0x0008
#define starfish_pro4m_final_deviceID 0x0009

#define kNumCamsSupported	8


#define FC_STARFISH_BULK_OUT_ENDPOINT 0x02
#define FC_STARFISH_BULK_IN_ENDPOINT  0x86




//structure used for command only messages that don't have any parameters
typedef struct {
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	cksum;
	} fc_no_param;




// structure used when calling the low level set register command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	registerAddress;
    UInt16	dataValue;
    UInt16	cksum;
	} fc_setReg_param;



// structure used when calling the low level get register command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	registerAddress;
    UInt16	cksum;
	} fc_getReg_param;


// structure used to hold the return information from the fcGETREG command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	dataValue;
	} fc_regInfo;
	

// structure used when calling the fcSETINTTIME command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	timeHi;
    UInt16	timeLo;
    UInt16	cksum;
	} fc_setIntTime_param;
	

// structure used when calling the fcSETFGTP command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	state;
    UInt16	cksum;
	} fc_setFgTp_param;



// structure used when calling the fcRDSCANLINE command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	LineNum;
    UInt16	padZero;
    UInt16	Xmin;
    UInt16	Xmax;
    UInt16	cksum;
	} fc_rdScanLine_param;



// structure used to hold the return information from the fcRDSCANLINE command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	LineNum;
    UInt16	padZero;
    UInt16	Xmin;
    UInt16	Xmax;
    UInt16	lineBuffer[2048];
	} fc_scanLineInfo;




// structure used when calling the fcSETROI command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	left;
    UInt16	top;
    UInt16	right;
    UInt16	bottom;
    UInt16	cksum;
	} fc_setRoi_param;


// structure used when calling the fcSETBIN command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	binMode;
    UInt16	cksum;
	} fc_setBin_param;







// structure used when calling the fcSETRELAY command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	relayNum;
    UInt16	cksum;
	} fc_setClrRelay_param;



// structure used when calling the fcPULSERELAY command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	relayNum;
    UInt16	highPulseWidth;
    UInt16	lowPulseWidth;
    UInt16	repeats;
    UInt16	cksum;
	} fc_pulseRelay_param;




// structure used to hold the return information from the fcGETTEMP command
typedef struct{
    UInt16	header;
    UInt16	command;
    SInt16	tempValue;
    UInt16	TECPwrValue;
    UInt16	TECInPwrOK;
	} fc_tempInfo;





// structure used when calling the fcSETTEMP command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    SInt16	theTemp;
    UInt16	cksum;
	} fc_setTemp_param;








// structure used when calling the fcSETREADMODE command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    SInt16	ReadBlack;
    UInt16	DataXfrReadMode;
    UInt16	DataFormat;
    SInt16	AutoOffsetCorrection;
    UInt16	cksum;
	} fc_setReadMode_param;





// structure used to store information about any cameras that were detected by the application
typedef struct {
    UInt16	camVendor;					// FC vendor ID
    UInt16	camRawProduct;				// RAW, uninitialized camera ID
    UInt16	camFinalProduct;			// initialized camera ID
    UInt16	camRelease;					// camera serial number
//	CCyUSBDevice	**camUsbIntfc;		// handle to this camera
	struct libusb_device_handle *dev;	// handle to this camera
 	} fc_Camera_Information;






// structure used when calling the fcSETGAIN command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	theGain;
    UInt16	cksum;
	} fc_setGain_param;


// structure used when calling the fcSETOFFSET command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	theOffset;
    UInt16	cksum;
	} fc_setOffset_param;



// structure used to hold the return information from the fcGETPEDESTAL command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	dataValue;
	} fc_blackPedestal;



// structure used for the fcSETPROPERTY command
typedef struct{
    UInt16	header;
    UInt16	command;
    UInt16	length;
    UInt16	propertyType;
    UInt16	propertyValue;
    UInt16	cksum;
	} fc_setProperty_param;








#define MAX_INTEL_HEX_RECORD_LENGTH 16
typedef struct _INTEL_HEX_RECORD
{
   UInt32  	Length;
   UInt32 	Address;
   UInt32  	Type;
   UInt8  	Data[MAX_INTEL_HEX_RECORD_LENGTH];
} INTEL_HEX_RECORD, *PINTEL_HEX_RECORD;

#define	k8051_USBCS		0xE600

#endif // FISHCAMP_COMMON
