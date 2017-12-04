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

  Modified by: Jasem Mutlaq (2013)

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

#include "fishcamp.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <stdbool.h>

#include <libusb-1.0/libusb.h>

#define MAXRBUF    512

// globals
struct libusb_context *gCtx;

fc_Camera_Information	gCamerasFound[kNumCamsSupported];
 
bool			gFWInitialized;						// set when the fcUsb_init is executed the first time.
char			gBuffer[513];

UInt16			gRelease;							// set in the 'RawDeviceAdded' routine.  Is actually used
													// by fishcamp as a camera serial number
int				gNumCamerasDiscovered;				// total cameras found on the USB interface
 
bool			gReadBlack[kNumCamsSupported];		// set if the host wishes to read the black pixels
int				gDataXfrReadMode[kNumCamsSupported];// the type of data transfers we will be using when taking picts
int				gDataFormat[kNumCamsSupported];		// the desired camera data format
 
fcFindCamState	gFindCamState;						// the progress state codes that the fcUsb_FindCameras routine reports
float			gFindCamPercentComplete;			// 0.0 -> 100.0 progress as a percent of total time.

UInt32			gCurrentIntegrationTime[kNumCamsSupported];
UInt32			gCurrentGuiderIntegrationTime[kNumCamsSupported];	// currently set exposure time for the guider part of the camera


// add an internal frame buffer to handle time when we do black level line compensation.
// we use the internal frame buffer which is bigger than what the user asked for in order to 
// be able to handle the extra black columns.  We then strip it out of the uploaded image
// so the user never sees them
UInt16			*gFrameBuffer;
UInt16			gRoi_left[kNumCamsSupported];		// this is the requested size from the user
UInt16			gRoi_top[kNumCamsSupported];
UInt16			gRoi_right[kNumCamsSupported];
UInt16			gRoi_bottom[kNumCamsSupported];

// for IBIS 1300 image sensor we look at the average of the first row of black pixels to perform the column normalization
// this is where we store the average.  It is computed every time the gain setting on the image sensor is made.
SInt32			gBlackOffsets[1280];	


// for the Starfish PRO camera, we will calculate the column and row offsets from information in the overscan pixels
// we will use the vertical overscan to calculate the column offsets and the horizontal overscan to calculate the
// row offsets.  The final number in this vector will be the number that needs to be added to the given row/col
// to normalize the image.  Thus the reason of using SInt32.
SInt32			gProBlackColOffsets[4096];			// offsets.  one for each column
SInt32			gProBlackRowOffsets[4096];			// offsets.  one for each row
bool			gProWantColNormalization;			// true if we want column normalization

//CCyUSBDevice*		gUSBDevice;

bool			gDoLogging;							// set to TRUE to enable logging to the log file
bool            gDoSimulation;                      // set to TRUE to enable simulation

int				gCameraImageFilter[kNumCamsSupported];	// type of image filter for post processing on this camera

UInt16			gBlackPedestal[kNumCamsSupported];

// send data via the designated camera's bulk out endpoint
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int SendUSB(int camNum, unsigned char* data, int length)
{
	int retVal, transferred;
	struct libusb_device_handle *dev;

	retVal = 0;

	dev = gCamerasFound[camNum - 1].dev;
	if (dev != NULL)
		retVal = libusb_bulk_transfer(dev, FC_STARFISH_BULK_OUT_ENDPOINT, data, length, &transferred, 10000);

	if (retVal == 0)
		return transferred;
	else
		return -1;
}

// receive data via the designated camera's bulk in endpoint
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int RcvUSB(int camNum, unsigned char* data, int maxBytes)
{
	int retVal, transferred;
	struct libusb_device_handle *dev;

	retVal = 0;

	dev = gCamerasFound[camNum - 1].dev;
	if (dev != NULL)
		retVal = libusb_bulk_transfer(dev, FC_STARFISH_BULK_IN_ENDPOINT, data, maxBytes, &transferred, 10000);

	if (retVal == 0)
		{
//		printf("RcvUSB - %d bytes\n", transferred);
		return transferred;
		}
	else
		{
//		printf("RcvUSB - ERROR\n");
		return -1;
		}
}

// routine to check to see if we have a starfish log file on disk
// return TRUE if one exists
//
bool haveStarfishLogFile()
{
	FILE	*theLogFile;
	bool	retValue;

	// assume the file isn't there
	retValue = false;

	// Open for read (will fail if file does not exist)
    char filename[MAXRBUF];
    snprintf(filename, MAXRBUF, "%s/.indi/starfish_log.txt", getenv("HOME"));
    if( (theLogFile  = fopen( filename, "r" )) == NULL )
		{
		// no such file
//		printf("haveStarfishLogFile = FALSE\n");
		retValue = false;
		}
	else
		{
		// the file was there
//		printf("haveStarfishLogFile = TRUE\n");
		fclose( theLogFile );
		retValue = true;
		}

   return retValue;
}


// routine to create the starfish log file on disk.
// will first check to see if the file already exists.  
// will simply return if the file is there already
// otherwise it will create it.
//
void creatStarfishLogFile()
{
	FILE	*theLogFile;

	if (! haveStarfishLogFile() )
		{
        char filename[MAXRBUF];
        snprintf(filename, MAXRBUF, "%s/.indi/starfish_log.txt", getenv("HOME"));
        if (!(theLogFile  = fopen( filename, "w" )))
        {
            fprintf(stderr, "Error opening Starfish log file (%s) : %s\n", filename, strerror(errno));
            return;
        }
		fclose( theLogFile );
		}

}

// routine that is called everytime we make a log file entry.
// we don't want the log file to get too big, so we check its size
// and resize it if it was getting too big.  Simple routine
// here is that it checks the size of the file and if too
// big, it simply clears it out starting at zero again.
//
void check4tooLongLogFile()
{
	FILE	*theLogFile;
	long	length;
	int		result;


	if ( haveStarfishLogFile() )
		{
		// Open for write
        char filename[MAXRBUF];
        snprintf(filename, MAXRBUF, "%s/.indi/starfish_log.txt", getenv("HOME"));
        theLogFile = fopen( filename, "a+" );
		fseek( theLogFile, 0, SEEK_END);
		length = ftell( theLogFile );
		fclose( theLogFile );

		if (length > 8000000)
			{
			// file too big, need to reset it and start over
//			printf("Re-creating (smaller) Starfish Log File.\n");
            theLogFile = fopen( filename, "w" );	// "w" will re-create the file
			fclose( theLogFile );
			}

		}

}


void fc_timestamp(FILE *out)
{
	time_t			t;
	struct timeval	tv;
	char			timestamp[40];

	gettimeofday(&tv, NULL);
	*(unsigned long *)&t = tv.tv_sec;
	strftime(timestamp, sizeof timestamp, "%e %b %G %T", localtime(&t));

	fprintf(out, "%s.%02ld ", 
                timestamp, tv.tv_usec / 10000);
}

// 
// routine to add a log entry to the Starfish log file
//
// The log file is by default placed in \etc\fishcamp\starfish_log.txt
//
// This routine will first check to see if the log file exists and create it if it doesn't
//
// Then the routine will append the passed logString to the end of the file.
//
void Starfish_Log (char *logString)
{
	FILE	*theLogFile;
	int		numWritten;
	char	tmpDateBuf[128];
	char	tmpTimeBuf[128];
	char	buffer[200];


	if (gDoLogging)		// only do if logging has been enabled
		{
		// check to see if we have the starfish log file yet
		// if not create one
		if (! haveStarfishLogFile() )
			{
			creatStarfishLogFile();
			}

		// simple way of assuring the log file doesn't get too big
		check4tooLongLogFile();

        char filename[MAXRBUF];
        snprintf(filename, MAXRBUF, "%s/.indi/starfish_log.txt", getenv("HOME"));
        if( (theLogFile = fopen( filename, "a+" )) != NULL )
			{
			// get operating system-style date and time. 
            fc_timestamp(theLogFile);

			sprintf( buffer, "- %s", logString ); 

			numWritten = fputs( buffer, theLogFile );
			fclose( theLogFile );

			}
//		else
//			printf( "Problem opening the file\n" );

	
		}
}

// utility routine to see if we have already discovered that a certain
// camera was connected to this computer.  Will return TRUE if so.
//
// enter with:
//		- FC vendor ID
//		- RAW camera product ID or -1 if not valid
//		- Final camera product ID or -1 if not valid
//		- camera serial number
//
// the routine will use the desired camera product ID specified.  pass in a -1 if not valid.
//
bool sawThisCameraAlready (UInt16 vendor, UInt16 rawProduct, UInt16 finalProduct, UInt16 release)
{
	int			i;
	bool		retValue;
	
	retValue = false;

	for (i = 0; i < kNumCamsSupported; i++)
		{
		if (gCamerasFound[i].camVendor == vendor)
			{
			if ((rawProduct == 0xffff) || (gCamerasFound[i].camRawProduct == rawProduct))
				{
				if ((finalProduct == 0xffff) || (gCamerasFound[i].camFinalProduct == finalProduct))
					{
					if (gCamerasFound[i].camRelease == release)
						{
						retValue = true;
						}
					}
				}
			}
		}
	
	return retValue;
}

// utility routine to return the index of the camera in the local 'gCamerasFound' array
// will return 0 -> (kNumCamsSupported - 1) if we have the camera in our data base
// will return -1 if we don't.
//
// enter with:
//		- FC vendor ID
//		- RAW camera product ID or -1 if not valid
//		- Final camera product ID or -1 if not valid
//		- camera serial number
//
// the routine will use the desired camera product ID specified.  pass in a -1 if not valid.
//
int getCameraDBIndex (UInt16 vendor, UInt16 rawProduct, UInt16 finalProduct, UInt16 release)
{
	int		i;
	int		retValue;
	
	retValue = -1;

	for (i = 0; i < kNumCamsSupported; i++)
		{
		if (gCamerasFound[i].camVendor == vendor)
			{
			if ((rawProduct == 0xffff) || (gCamerasFound[i].camRawProduct == rawProduct))
				{
				if ((finalProduct == 0xffff) || (gCamerasFound[i].camFinalProduct == finalProduct))
					{
					if (gCamerasFound[i].camRelease == release)
						{
						retValue = i;
						}
					}
				}
			}
		}
	
	return retValue;
}

// utility routine to return the DB index of the next available slot in the local 'gCamerasFound' array
// will return 0 -> (kNumCamsSupported - 1) if we have room in our data base
// will return -1 if we don't.
//
//
int getNextFreeDBIndex (void)
{
	int		i;
	int		retValue;
	
	retValue = -1;

	for (i = 0; i < kNumCamsSupported; i++)
		{
		if (gCamerasFound[i].camVendor == 0)
			{
			retValue = i;
			break;
			}
		}
	
	return retValue;
}

// utility debug routine used to print out the camera data base
//
void printOutDiscoveredCamerasDB (void)
{
	int		i;
	char	buffer[200];
	

	for (i = 0; i < kNumCamsSupported; i++)
		{
		sprintf( buffer, "Camera DB for discovered camera # - %d\n", i ); 
		Starfish_Log( buffer );

		sprintf( buffer, "     gCamerasFound[%d].camVendor       - %04x\n", i, gCamerasFound[i].camVendor);
		Starfish_Log( buffer );

		sprintf( buffer, "     gCamerasFound[%d].camRawProduct   - %04x\n", i, gCamerasFound[i].camRawProduct);
		Starfish_Log( buffer );

		sprintf( buffer, "     gCamerasFound[%d].camFinalProduct - %04x\n", i, gCamerasFound[i].camFinalProduct);
		Starfish_Log( buffer );

		sprintf( buffer, "     gCamerasFound[%d].camRelease      - %04x\n", i, gCamerasFound[i].camRelease);
		Starfish_Log( buffer );
		}
	
}

void AnchorWrite(struct libusb_device_handle *dev_handle, UInt16 anchorAddress, UInt16 count, UInt8 writeBuffer[])
{
	int retval;
	
	retval = libusb_control_transfer(
		dev_handle, /*dev_handle*/
		LIBUSB_REQUEST_TYPE_VENDOR, //REQ_VENDOR, /*request type*/ 
		0xa0, /*request*/
		anchorAddress, /*value*/
		0, /*index*/
		writeBuffer, /*(unsigned char *)data*/
		count, /*length*/
		5000 /*timeout*/
	);
	if (retval < 0)
		Starfish_Log("Error in control transfer.\n");
	
}

FILE *openFile(UInt16 vendor, UInt16 product)
{
    char name[]="/lib/firmware/gdr_usb.hex";
    char name2[]="/lib/firmware/pro_usb.hex";
	char path[256];
	FILE *hexFile;
 	char buffer[200];

    Starfish_Log("openFile routine\n");

	if (product == 0x0008)
		{
		sprintf(buffer, "File to open is \"%s\"\n", name2);
		Starfish_Log( buffer );

		hexFile = fopen(name2, "r");
		}
	else
		{
		sprintf(buffer, "File to open is \"%s\"\n", name);
		Starfish_Log( buffer );

		hexFile = fopen(name, "r");
		}
    
    return(hexFile);
}

/*
    Description of Intel hex records.

Position	Description
1		Record Marker: The first character of the line is always a colon (ASCII 0x3A) to identify 
                        the line as an Intel HEX file
2 - 3	Record Length: This field contains the number of data bytes in the register represented 
                        as a 2-digit hexidecimal number. This is the total number of data bytes, 
                        not including the checksum byte nor the first 9 characters of the line.
4 - 7	Address: This field contains the address where the data should be loaded into the chip. 
                        This is a value from 0 to 65,535 represented as a 4-digit hexidecimal value.
8 - 9	Record Type: This field indicates the type of record for this line. The possible values 
                        are: 00=Register contains normal data. 01=End of File. 02=Extended address.
10 - ?	Data Bytes: The following bytes are the actual data that will be burned into the EPROM. The 
                        data is represented as 2-digit hexidecimal values.
Last 2 characters	Checksum: The last two characters of the line are a checksum for the line. The 
                        checksum value is calculated by taking the twos complement of the sum of all 
                        the preceeding data bytes, excluding the checksum byte itself and the colon 
                        at the beginning of the line.
*/
int hexRead(INTEL_HEX_RECORD *record, FILE *hexFile, UInt16 vendor, UInt16 product)
{	// Read the next hex record from the file into the structure

    // **** Need to impliment checksum checking ****
    if (gDoSimulation)
        return 0;

	char	c;
    UInt16	i;
    UInt16	newChecksum;
	int		n, c1, check, len;
 	char	buffer[200];

	c = getc(hexFile);
    
    if(c != ':')
    {
        sprintf(buffer, "Line does not start with colon (%d)\n", c);
		Starfish_Log( buffer );
        return(-1);
    }
    n = fscanf(hexFile, "%2lX%4lX%2lX", &record->Length, &record->Address, &record->Type);
    if(n != 3)
    {
        sprintf(buffer, "Could not read line preamble %d\n", c);
		Starfish_Log( buffer );
        return(-1);
    }
    
    len = record->Length;
    if(len > MAX_INTEL_HEX_RECORD_LENGTH)
    {
        sprintf(buffer, "length is more than can fit %d, %d\n", len, MAX_INTEL_HEX_RECORD_LENGTH);
		Starfish_Log( buffer );
        return(-1);
   }
    for(i = 0; i<len; i++)
    {
        n = fscanf(hexFile, "%2X", &c1);
        if(n != 1)
        {
            if(i != record->Length)
            {
                sprintf(buffer, "Line finished at wrong time %d, %ld\n", i, record->Length);

				Starfish_Log( buffer );
                return(-1);
            }
        }
        record->Data[i] = c1;
    
    }
    n = fscanf(hexFile, "%2X\n", &check);
    if(n != 1)
    {
        sprintf(buffer, "Check not found\n");
		Starfish_Log( buffer );
        return(-1);
    }
	
	
	// final operation.  stuff the serial number of the RAW camera into the firmware file of the final camera
//	printf("     record->Length  = %ld\n", record->Length);
//	printf("     record->Address = %ld\n", record->Address);
//	printf("     record->Type    = %ld\n", record->Type);
	if ((record->Length == 16) && (record->Address == 0x0700) && (record->Type == 00))
		{
        Starfish_Log("setting configured productID\n");
		record->Data[10] = (product & 0xFF) | 0x01;		// even# was RAW device, odd will be CONFIGURED device
		record->Data[11] = (product >> 8) & 0xFF;

        Starfish_Log("setting serial number\n");
		record->Data[12] = gRelease & 0xFF;
		record->Data[13] = (gRelease >> 8) & 0xFF;


		// compute new checksum
		newChecksum = 0x10 + 0x07;
		for(i = 0; i<len; i++)
			{
			newChecksum = newChecksum + record->Data[i];
			}
		newChecksum = ~newChecksum;
		newChecksum = newChecksum + 1;
		newChecksum = newChecksum & 0xff;

//		printf("     old check       = %ld\n", check);
		check = newChecksum;
//		printf("     new check       = %ld\n", check);
		}
	
		
		
    return(0);
}

// routine used to load the USB chip's firmware.
// called from RawDeviceAdded,
//
int DownloadToAnchorDevice(struct libusb_device_handle *dev, UInt16 vendor, UInt16 product)
{
    UInt8 				writeVal;
    int					kr;
	FILE				*hexFile;
	INTEL_HEX_RECORD	anchorCode;
    
    Starfish_Log("DownloadToAnchorDevice routine\n");

    if (gDoSimulation)
        return 0;

    hexFile = openFile(vendor, product);
    if (hexFile == NULL) {
        Starfish_Log("DownloadToAnchorDevice could not open file.\n");
        return -1;
    }

    // Assert reset
    writeVal = 1;
    AnchorWrite(dev, k8051_USBCS, 1, &writeVal);
    
    // Download code
    while (1) {
        kr = hexRead(&anchorCode, hexFile, vendor, product);
        if (anchorCode.Type != 0)
            break;

        if (kr == 0)
            AnchorWrite(dev, anchorCode.Address, anchorCode.Length, anchorCode.Data);
    }

    // De-assert reset
    writeVal = 0;
    AnchorWrite(dev, k8051_USBCS, 1, &writeVal);
    
    return 0;
}

// Routine to search the 'fishcamp' directory for the latest version of the
// MicroBlaze SREC file.  The following file name format is assumed
//		"\etc\fishcamp\Guider_mono_revXX_intel.srec";
//
// where revXX is an incrementing rev number.
//
// will return the rev number of the latest file found or -1 if no file found
//
int GetLatestSrecFileRevNumber(void)
{
	int			returnNumber;
	int			tempInt;
	bool		done;
 	char		fileName[200];
	FILE		*srecFile;
 	char		buffer[200];

	Starfish_Log("GetLatestSrecFileRevNumber\n");

	returnNumber = 16;		// rev 16 is the latest version released
	done = false;
	while (!done)
		{
		// generate the next file name to look for
        sprintf(fileName, "/lib/firmware/Guider_mono_rev%02d_intel.srec", returnNumber);

		sprintf(buffer, "  Looking for file - \"%s\"\n", fileName);
		Starfish_Log( buffer );

		// try to open the file
		srecFile = fopen(fileName, "r");
		if (srecFile == NULL)
			{
				// file didn't exist so we must be done looking
			Starfish_Log("  Done looking\n");
			done = true;
			}
		else
			{
			// file was there so keep looking
			Starfish_Log("  Found SREC file\n");

			fclose(srecFile);
			returnNumber++;
			}
		}

	returnNumber--;

	if (returnNumber == 15)
		{
		Starfish_Log("  Could not find an SREC file\n");
		returnNumber = -1;
		}
	else
		{
        sprintf(fileName, "/lib/firmware/Guider_mono_rev%02d_intel.srec", returnNumber);
		sprintf(buffer, "  Most recent SREC file is - \"%s\"\n", fileName);
		Starfish_Log( buffer );
		}

	return( returnNumber );


}


// routine to open the file containing the program to be executed by
// the camera's microBlaze processor.  File is S-records
//
FILE *openSrecFile(UInt16 vendor, UInt16 rawProduct)
{
    char name[]="/lib/firmware/Guider_mono_rev16_intel.srec";
    char name1[]="/lib/firmware/ibis_rev1_intel.srec";			// ibis 1300
    char name2[]="/lib/firmware/StarfishPRO4M_rev1_intel.srec";	// StarfishPRO4M
 	char buffer[200];
	int	 fileRevNumber;
 	char fileName[200];

	FILE *srecFile;

    Starfish_Log("openSrecFile routine\n");

	if (rawProduct == starfish_ibis13_raw_deviceID)
		{
		sprintf(buffer, "File to open is \"%s\"\n", name1);
		Starfish_Log( buffer );

		srecFile = fopen(name1, "r");
		}
	else
		{

		if (rawProduct == starfish_pro4m_raw_deviceID)
			{
			sprintf(buffer, "File to open is \"%s\"\n", name2);
			Starfish_Log( buffer );

			srecFile = fopen(name2, "r");
			}

		else
			{
	
			// look for the most recent SREC file
			fileRevNumber = GetLatestSrecFileRevNumber();

			if (fileRevNumber != -1)
				{
                sprintf(fileName, "/lib/firmware/Guider_mono_rev%02d_intel.srec", fileRevNumber);

				sprintf(buffer, "File to open is \"%s\"\n", fileName);
				Starfish_Log( buffer );

				srecFile = fopen(fileName, "r");
				}
			else
				{
				srecFile = NULL;
				Starfish_Log("  Could not open the file\n");
				}
			}
		}
	            
    return(srecFile);
}

char* srecRead(UInt8 *record, FILE *srecFile)
{
    // Read the next srecord from the file into the buffer
	int	maxLineSize;

	maxLineSize = 100;
	return (fgets((char*)record, maxLineSize, srecFile));

}

// will download the application program to the MicroBlaze processor in the camera.
// At boot-up, the camera is looking for srecords.  After the download the processor
// will jump execution to the downloaded program.
//
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//

void DownloadtToMicroBlaze(int camNum)


{
	FILE			*mySrecFile;
	char			*lineReadResult;
	bool			done;
	static char		srecBuffer[513];
	int				index, retVal, msgSize, transferred;
    UInt16 			vendor, rawProduct, finalProduct, release;
	struct libusb_device_handle *USBdev;


    Starfish_Log("DownloadtToMicroBlaze routine\n");

    if (gDoSimulation)
        return;

	vendor		 = gCamerasFound[camNum - 1].camVendor;
	rawProduct	 = gCamerasFound[camNum - 1].camRawProduct;
	finalProduct = gCamerasFound[camNum - 1].camFinalProduct;
	release		 = gCamerasFound[camNum - 1].camRelease;
	USBdev       = gCamerasFound[camNum - 1].dev;

	mySrecFile = openSrecFile(vendor, rawProduct);

	if (mySrecFile != NULL)
		{
		done = false;
		while (!done)
			{
			lineReadResult = srecRead((UInt8*)&srecBuffer[0], mySrecFile);
//			printf("%s", &srecBuffer[0]);
			if (lineReadResult == 0)
				done = true;
			else
				{			
				msgSize = strlen(srecBuffer);
			 	retVal = libusb_bulk_transfer(USBdev, FC_STARFISH_BULK_OUT_ENDPOINT, (unsigned char *)&srecBuffer[0], msgSize, &transferred, 10000);
				}
			}

		fclose(mySrecFile);
		}

}

// Notification.  Here when we discover that the guider camera is first plugged in.
//
//   - reads, VendorID, ProductID, and ReleaseID
//   - calls 'DownloadToAnchorDevice' to load the USB chip's firmware
//
//void RawDeviceAdded(void)
//{
//    UInt16		vendor;
//    UInt16		product;
//    UInt16		release;
//	int			index;
//	int			kr;
// 	char		buffer[200];
//
//
//
//    Starfish_Log("RawDeviceAdded routine\n");
//
//       
//	vendor  = gUSBDevice->VendorID;
//	product = gUSBDevice->ProductID;
//	release = gUSBDevice->BcdDevice;
//
//	sprintf(buffer, "     vendor  = %08x\n", vendor);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     product = %08x\n", product);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     release = %08x\n", release);
//	Starfish_Log( buffer );
//			
//	// put the new RAW camera in our data base
//	if (!sawThisCameraAlready (vendor, product, 0xffff, release))
//		{
//		index = getNextFreeDBIndex();
//		
//		gCamerasFound[index].camVendor       = vendor;
//		gCamerasFound[index].camRawProduct   = product;
//		gCamerasFound[index].camFinalProduct = 0;
//		gCamerasFound[index].camRelease      = release;
////		gCamerasFound[index].camUsbIntfc     = 0;
//
//		sprintf(buffer, "RawDeviceAdded - added RAW camera to DB index at index = %d\n", index);
//		Starfish_Log( buffer );
//		}
//		
//	// the ReleaseNumber is used by fishcamp as a camera serial number.  Store it away
//	gRelease = release;
//
//    ConfigureAnchorDevice();
//
//    kr = DownloadToAnchorDevice(vendor, product);
//    if (0 != kr)
//		{
//        sprintf(buffer, "unable to download to device: %08x\n", kr);
//		Starfish_Log( buffer );
//        }
//
//}


//Notfication.  here when we discover our newly programmed guider device
//  - calls 'FindInterfaces'
//  - calls 'DownloadtToMicroBlaze' to load final camera software
//
//void NewDeviceAdded(void)
//{
//    int			kr;
//	int			kr2;
//    SInt32		score;
//    UInt16		vendor;
//    UInt16		product;
//    UInt16		release;
//	int			index;
// 	char		buffer[200];
//
//
//
//
//    Starfish_Log("NewDeviceAdded routine\n");
//            						
//	vendor  = gUSBDevice->VendorID;
//	product = gUSBDevice->ProductID;
//	release = gUSBDevice->BcdDevice;
//
//	sprintf(buffer, "     vendor  = %08x\n", vendor);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     product = %08x\n", product);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     release = %08x\n", release);
//	Starfish_Log( buffer );
//
//	// put the new initialized camera in our data base
//	// at this point only the RAW version of the camera will be in the data base, if at all.  It
//	// won't be in it at all if we re-started the application after previously been running
//	// and the camera is already initialized from before.
//	// by convention, the RAW productID will be an even number, the initialized version will be odd.
//	// we use this knowledge to look for any previous entries of the RAW version of this camera.
//	if (!sawThisCameraAlready (vendor, product & 0xfffe, 0xffff, release))
//		{
//		// here if we haven't seen this camera in its RAW state
//				
//		// check to see if it was already addded in its initialized state.  add it if not.
//		if (getCameraDBIndex (vendor, 0xffff, product, release) == -1)
//			{
//			index = getNextFreeDBIndex();
//			
//			gCamerasFound[index].camVendor       = vendor;
//			gCamerasFound[index].camRawProduct   = 0;
//			gCamerasFound[index].camFinalProduct = product;
//			gCamerasFound[index].camRelease      = release;
////			gCamerasFound[index].camUsbIntfc     = 0;
//				
//			sprintf(buffer, "NewDeviceAdded - added FINAL camera to DB at index = %d\n", index);
//			Starfish_Log( buffer );
//			}
//		else
//			Starfish_Log("NewDeviceAdded -  FINAL camera already in DB\n");
//		}
//	else
//		{
//		index =  getCameraDBIndex (vendor, product & 0xfffe, 0xffff, release);
//			
//		gCamerasFound[index].camVendor       = vendor;
//		// don't touch the following field.  we already saw this camera added as a RAW device
////		gCamerasFound[index].camRawProduct   = 0;
//		gCamerasFound[index].camFinalProduct = product;
//		gCamerasFound[index].camRelease      = release;
////		gCamerasFound[index].camUsbIntfc     = 0;
//
//		sprintf(buffer, "NewDeviceAdded - updating RAW camera at DB index = %d\n", index);
//		Starfish_Log( buffer );
//		}
//
//
//
//	// the ReleaseNumber is used by fishcamp as a camera serial number.  Store it away
//	gRelease = release;
//
//    ConfigureAnchorDevice();
//
//	FindInterfaces();
//
//	// one last thing....
//	// need to load the MicroBlaze executable program before doing any camera stuff.
//
//	if (gCamerasFound[index].camRawProduct != 0)	// if the camera was discovered in the RAW state.  we will know this cause it will be non-zero
//		{
//		Starfish_Log("Calling - DownloadtToMicroBlaze \n");
//		DownloadtToMicroBlaze(gCamerasFound[index].camVendor, gCamerasFound[index].camRawProduct, gCamerasFound[index].camFinalProduct, gCamerasFound[index].camRelease);
//		}
//	else
//		{
//		Starfish_Log("didn't need to call - DownloadtToMicroBlaze \n");
//		}
//	
////	printOutDiscoveredCamerasDB();	
//}

// given a pointer to a command parameter struct, this routine will 
// calculate the correct cksum field
UInt16	fcUsb_GetUsbCmdCksum(UInt16 *cmdParams)
{
    UInt16	theCksum;
    UInt16	*ptr16;
    UInt16	theLength;
	int		i;
	
	ptr16 = cmdParams;
	
	theCksum = *ptr16++;	// start with the header
	
	theCksum += *ptr16++;	// add in the command field
	
	theLength = *ptr16++;
	theCksum += theLength;
	
	for (i = 0; i < (theLength - 8); i += 2)
		{
		theCksum += *ptr16++;
		}
	
	
	return theCksum;
	
}

// helper routine for fcImage_doFullFrameRowLevelNormalization.
// will calculate the average level of the pixels in the
// black cols of the image sensor
//
float fcImage_calcFullFrameAllColAvg(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			floatPixel;
	float			retValue;
	int				row, col;
	bool			evenRow;
    UInt16			*inputPtr;
    UInt16			aPixel;

	retValue = 0.0;
	
	// we will average all of the pixels in cols 0 -> 14
	for (row = 0; row < imageHeight; row++)
		{
		inputPtr = frameBufferPtr;
		inputPtr = inputPtr + (row * imageWidth);

		for (col = 0; col < 14; col++)
			{
			// get the next pixel
			aPixel = *inputPtr++;
			
			floatPixel = (float) aPixel;
			retValue += floatPixel;
			}
		}
		

	//divide by the number of pixels in the black col
	retValue = retValue / (14.0 * (float)imageHeight);
	
	return retValue;
}

// helper routine for fcImage_doFullFrameRowLevelNormalization.
// will calculate the average level of the pixels in the
// defined ROW's black columns
float fcImage_calcFullFrameRowAvgForRow(UInt16 *frameBufferPtr, int imageWidth, int imageHeight, int theRow)
{
	float			floatPixel;
	float			retValue;
	int				row, col;
	bool			evenRow;
    UInt16			*inputPtr;
    UInt16			aPixel;

    UInt16			lowPixel;
    UInt16			hiPixel;
	
	retValue = 0.0;

	lowPixel = 0xffff;
	hiPixel  = 0;
	
	// we will average all of the pixels in cols 0 -> 14
	inputPtr = frameBufferPtr;
	inputPtr = inputPtr + (theRow * imageWidth);
		
	for (col = 0; col < 14; col++)
		{
		// get the next pixel
		aPixel = *inputPtr++;

		// find lowest and highest pixel
		if (lowPixel > aPixel)
			lowPixel = aPixel;

		if (hiPixel < aPixel)
			hiPixel = aPixel;
			
		floatPixel = (float) aPixel;
		retValue += floatPixel;
		}

	// throw out lowest and highest
//	floatPixel = (float) lowPixel;
//	retValue  -= floatPixel;
//	floatPixel = (float) hiPixel;
//	retValue  -= floatPixel;


	//divide by the number of pixels in the black col
	retValue = retValue / 14.0;
//	retValue = retValue / 12.0;
	
	return retValue;
}

// routine to perform line level normalization on the RAW camera image
// Used to get rid of the camera's read noise associated with ROWs
// enter with pointer to 16 bit image
//
 void fcImage_doFullFrameRowLevelNormalization(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			aPixel;
	float			rowAvg;
	float			thisRowAvg;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	float			colOffset;
	float			floatPixel;
	

    if (gDoSimulation)
    {
        srand(time(NULL));
        int i=0,j=0;

        for (i=0; i < imageHeight ; i++)
          for (j=0; j < imageWidth; j++)
              frameBufferPtr[i*imageWidth+j] = rand() % 65535;

        return;
    }

	// make sure we are dealing with 16 bit pixels
	//	printf("fcImage_doFullFrameRowLevelNormalization\n");
	
	// calculate the average of all the black pixels
	frameAvg = fcImage_calcFullFrameAllColAvg(frameBufferPtr, imageWidth, imageHeight);
	
	for (row = 0; row < imageHeight; row++)
		{
		if (row > 0)
			rowAvg  = fcImage_calcFullFrameRowAvgForRow(frameBufferPtr, imageWidth, imageHeight, (row - 1));
//		rowAvg  = [self calcFullFrameRowMedianForRow: (row - 1)];
		
		thisRowAvg  = fcImage_calcFullFrameRowAvgForRow(frameBufferPtr, imageWidth, imageHeight, row);
//		thisRowAvg = [self calcFullFrameRowMedianForRow: row];
		
		if (row == 0)
			rowOffset = frameAvg - thisRowAvg;
		else
			rowOffset = rowAvg - thisRowAvg;		// 11-23-07
		
		inputPtr = frameBufferPtr;
		inputPtr = inputPtr + (row * imageWidth);
		for (col = 0; col < imageWidth; col++)
//		for (col = 0; col < (imageWidth / 2); col++)
			{
			// get the next pixel
			aPixel = *inputPtr;
			
			floatPixel = (float)aPixel;
		
			floatPixel += rowOffset;
			
			if (floatPixel > 65535.0)
				floatPixel = 65535.0;
			
			if (floatPixel < 0.0)
				floatPixel = 0.0;

			// put corrected value back
            *inputPtr++ = (UInt16)floatPixel;
			}
		
		}
		
}

// routine to strip the black columns form the image read from the camera.  the camera's image is
// assumed to be in gFrameBuffer and the pointer passed to this routine is where we will put the 
// stripped image;
void fcImage_StripBlackCols(int camNum, UInt16 *frameBuffer)
{
	float			reference;
	int				row, col;
	int				imageHeight, imageWidth;
    UInt16			*inputPtr;
    UInt16			*outputPtr;
    UInt16			aPixel;
	float			rowAvg;
	float			thisRowAvg;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	float			colOffset;
	float			floatPixel;
	
	imageHeight = (gRoi_bottom[camNum - 1] - gRoi_top[camNum - 1])  + 1;
	imageWidth  = (gRoi_right[camNum - 1]  - gRoi_left[camNum - 1]) + 1;
	
	for (row = 0; row < imageHeight; row++)
		{		
		inputPtr = gFrameBuffer;
//		if (row < (imageHeight / 2))
			inputPtr = inputPtr + (row * (imageWidth + 16)) + 16;
//		else
//			inputPtr = inputPtr + (row * (imageWidth + 16));
		outputPtr = frameBuffer;
		outputPtr = outputPtr + (row * imageWidth);
	
		for (col = 0; col < imageWidth; col++)
			{
			// transfer pixel
			*outputPtr++ = *inputPtr++;			
			}
		
		}
		
}

// the following three routines are used to touch up the images from the IBIS1300 sensor
// it normalizes the offsets in the columns.
// clear out the black row vector for the IBIS1300 image sensor
void fcImage_IBIS_clearBlackRowAverage(void)
{
	int	i;
	
	for (i = 0; i < 1280; i++)
		gBlackOffsets[i] = 0;
}

// accumulate another image's first black row from the IBIS image sensor
void fcImage_IBIS_accumulateBlackRowAverage(void)
{
    UInt16			*inputPtr;
    UInt16			aPixel;
	SInt32			bigPixel;
	int				col;

	// we will average all of the pixels in the first row
	inputPtr = gFrameBuffer;

	for (col = 0; col < 1280; col++)
		{
		// get the next pixel
		aPixel = *inputPtr++;
			
		bigPixel = (SInt32) aPixel;
		gBlackOffsets[col] = gBlackOffsets[col] + bigPixel;
		}
}

// here after we accumulated all of the images first black row.  now divide by the num of images taken
void fcImage_IBIS_divideBlackRowAverage(void)
{
	int	i;
	
	for (i = 0; i < 1280; i++)
		gBlackOffsets[i] = gBlackOffsets[i] / 4;
}

// helper routine for fcImage_IBIS_doFullFrameColLevelNormalization.
// will calculate the average level of the pixels in the
// first black row of the image sensor
//
float fcImage_IBIS_calcFirstBlackRowAverage(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			floatPixel;
	float			retValue;
	int				row, col;
	bool			evenRow;
    UInt16			*inputPtr;
	SInt32			aPixel;
	
	retValue = 0.0;
	
	// we will average all of the pixels in the first row

	for (col = 0; col < imageWidth; col++)
		{
		// get the next pixel
		aPixel = gBlackOffsets[col];
			
		floatPixel = (float) aPixel;
		retValue += floatPixel;
		}
		

	//divide by the number of pixels in the black row
	retValue = retValue / (float)imageWidth;
	
	return retValue;
}

// routine which will subtract out the offset pedestal from the image.  It looks at the 
// first row of black pixels to determine the average of the row.  Then it subtracts out
// that number from each pixel in the image.
void fcImage_IBIS_subtractPedestal(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			aPixel;
	SInt32			bigPixel;
	SInt32			thePedestal;
	float			rowAvg;
	float			thisColBlack;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	float			colOffset;
	float			floatPixel;
	

	// calculate the average of all the black pixels in the first row
	frameAvg = fcImage_IBIS_calcFirstBlackRowAverage(frameBufferPtr, imageWidth, imageHeight);
	thePedestal = (SInt32) frameAvg;
	
	// don't touch the black row.  Start at row '1'
	inputPtr = frameBufferPtr;
	inputPtr = inputPtr + imageWidth;
	for (col = 0; col < imageWidth; col++)
		{		
		for (row = 1; row < imageHeight; row++)
			{
			aPixel = *inputPtr;
			bigPixel = (SInt32) aPixel;
			bigPixel = bigPixel - thePedestal;
			
			if (bigPixel > 65535)
				bigPixel = 65535;
		
			if (bigPixel < 0)
				bigPixel = 0;

			// put corrected value back
            *inputPtr = (UInt16)bigPixel;
		
			// point to next
			inputPtr++;
			}
		}
		
}

// this routine is called everytime the gain setting is changed on the IBIS1300 image sensor.  I takes 8 frames 
// and stores the average of the first black row of pixels in the sensor.  The resulting vector is then used
// to normalize the columns everytime a new image is readout.
void fcImage_IBIS_computeAvgColOffsets(int camNum)
{
    UInt32	savedIntegrationTime;
	bool	done;
    UInt16	state;
	int		i;
	
	
	// clear out the vector
	fcImage_IBIS_clearBlackRowAverage();
	
	// remember the integration time that the user wants
	savedIntegrationTime = gCurrentIntegrationTime[camNum - 1];

	// set a short integration time
	fcUsb_cmd_setIntegrationTime(camNum, 10);	// 10 ms
	
	
	
	// take an initial picture
	fcUsb_cmd_startExposure(camNum);
		
	// wait till the picture is done
	done = false;
	while (!done)
		{
		state = fcUsb_cmd_getState(camNum);
		if (state == 0)
			done = true;
		}



	for(i = 0; i < 4; i++)
		{
		// take a picture
		fcUsb_cmd_startExposure(camNum);
		
		// wait till the picture is done
		done = false;
		while (!done)
			{
			state = fcUsb_cmd_getState(camNum);
			if (state == 0)
				done = true;
			}
		
		// read in the image into our local frame buffer
		fcUsb_cmd_getRawFrame(camNum, 1024, 1280, gFrameBuffer);
		
		// add to the average
		fcImage_IBIS_accumulateBlackRowAverage();
		}
	
	// divide the vector by the number of picts taken
	fcImage_IBIS_divideBlackRowAverage();
	
	// put the users' desired integration time back
	fcUsb_cmd_setIntegrationTime(camNum, savedIntegrationTime);
}

// routine to perform column level normalization on the RAW camera image
// Used to get rid of the camera's fixed pattern noise associated with COLs
// enter with pointer to 16 bit image
//
 void fcImage_IBIS_doFullFrameColLevelNormalization(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			aPixel;
	float			rowAvg;
	SInt32			thisColBlack;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	SInt32			colOffset;
	float			floatPixel;
	SInt32			blackAvg;
	SInt32			bigPixel;
	SInt32			theOffset;
	

	// make sure we are dealing with 16 bit pixels
	
//	printf("fcImage_IBIS_doFullFrameColLevelNormalization\n");
	
	// calculate the average of all the black pixels
	frameAvg = fcImage_IBIS_calcFirstBlackRowAverage(frameBufferPtr, imageWidth, imageHeight);
	blackAvg = (SInt32) frameAvg;
	
	for (col = 0; col < imageWidth; col++)
		{
		// first get this cols black pixel from the first row of the image
		thisColBlack = gBlackOffsets[col];
		
		colOffset = blackAvg - thisColBlack;
		
		for (row = 1; row < imageHeight; row++)
			{
			// get the pixel for this row/col		
			inputPtr = frameBufferPtr;
			inputPtr = inputPtr + (row * imageWidth) + col;
			aPixel = *inputPtr;
			bigPixel = (SInt32) aPixel;
			
			// normalize
			bigPixel = bigPixel + colOffset;

			if (bigPixel > 65535)
				bigPixel = 65535;
		
			if (bigPixel < 0)
				bigPixel = 0;

			// put corrected value back
            *inputPtr = (UInt16)bigPixel;
		
			}
		}
		
}

// routine to compute the column level offsets in the image.
// We do this by examining the vertical overscan region in the image
// Computing the average in the particular column.
void fcImage_PRO_calcColOffsets(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			aPixel;
	float			rowAvg;
	float			thisRowAvg;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	float			colOffset;
	float			floatPixel;
	int				startRow, endRow;
	SInt32			colAverage;
	

	
//	printf("fcImage_PRO_calcColOffsets\n");
	Starfish_Log("fcImage_PRO_calcColOffsets\n");
	
	// clear out any old results
	for (col = 0; col < 4096; col++)
		gProBlackColOffsets[col] = 0;

	// define the start row and end row of the vertical overscan region of the image
	startRow = 2100;
	endRow   = 2300;

	// accumulate the average
	for (col = 0; col < imageWidth; col++)
		{
		for (row = startRow; row < endRow; row++)
			{		
			inputPtr = frameBufferPtr;
			inputPtr = inputPtr + (row * imageWidth) + col;
			
			// get the next pixel
			aPixel = *inputPtr;

			// add up
			gProBlackColOffsets[col] += (SInt32)aPixel;
			}

		// divide by the number of rows in the overscan area
		gProBlackColOffsets[col] = gProBlackColOffsets[col] / (SInt32)(endRow - startRow);
		}

	// now calculate the average of all the columns
	colAverage = 0;
	for (col = 0; col < imageWidth; col++)
		{
		colAverage += gProBlackColOffsets[col];
		}
		
	colAverage = colAverage / (SInt32)imageWidth;

	// normalize the offsets to the column average
	for (col = 0; col < imageWidth; col++)
		{
		gProBlackColOffsets[col] = gProBlackColOffsets[col] - colAverage;
		}
}

// routine to perform column level normalization on the RAW camera image
// Used to get rid of the sensor's fixed pattern noise associated with COLs
// enter with pointer to 16 bit image
//
 void fcImage_PRO_doFullFrameColLevelNormalization(UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			aPixel;
	float			rowAvg;
	float			thisRowAvg;
	float			minRowAvg;
	float			colAvg;
	float			frameAvg;
	float			rowOffset;
	float			colOffset;
	float			floatPixel;
	
	
//	printf("fcImage_PRO_doFullFrameColLevelNormalization\n");
	Starfish_Log("fcImage_PRO_doFullFrameColLevelNormalization\n");

	// calculate the average of all the black pixels in the vertical overscan area
//	fcImage_PRO_calcColOffsets(frameBufferPtr, imageWidth, imageHeight);
	
	for (row = 0; row < imageHeight; row++)
		{		
		inputPtr = frameBufferPtr;
		inputPtr = inputPtr + (row * imageWidth);
		for (col = 0; col < imageWidth; col++)
			{
			// get the next pixel
			aPixel = *inputPtr;

			// get the offset for this column
			colOffset = (float)gProBlackColOffsets[col];
			
			floatPixel = (float)aPixel;
		
			floatPixel -= colOffset;
			
			if (floatPixel > 65535.0)
				floatPixel = 65535.0;
			
			if (floatPixel < 0.0)
				floatPixel = 0.0;

			// put corrected value back
            *inputPtr++ = (UInt16)floatPixel;
			}
		
		}
		
}

// this routine is used internally to calibrate the PRO series cameras.  We do this each time
// the fcPROP_NUMSAMPLES property is changed or if the sensor's temperature changes by more than 1 degree C.
//
// the primary job this routine performs is taking a bias frame and measuring the vertical overscan region
// so that we know how to do column level normalization.  We only can collect this information during a
// bias frame since a long exposure light frame will have the scene bleed into the vertical overscan region
// and corrupt the measurement.
//
void fcImage_PRO_calibrateProCamera(int camNum, UInt16 *frameBufferPtr, int imageWidth, int imageHeight)
{
    UInt32	savedIntegrationTime;
	bool	done;
    UInt16	state;
	int		i;
	bool	savedWantNorm;
	
	Starfish_Log("fcImage_PRO_calibrateProCamera\n");

    if (gDoSimulation)
        return;
	
	// remember the current integration time setting so that we can put it back when we are done.
	savedIntegrationTime = gCurrentIntegrationTime[camNum - 1];
	savedWantNorm = gProWantColNormalization;

	gProWantColNormalization = false;

	// set a short integration time.  bias frame = 1ms exposure
	fcUsb_cmd_setIntegrationTime(camNum, 1);	// 1 ms
	
	for (i = 0; i < 2; i++)
	{
	// take a picture
	fcUsb_cmd_startExposure(camNum);
		
	// wait till the picture is done
	done = false;
	while (!done)
		{
		state = fcUsb_cmd_getState(camNum);
		if (state == 0)
			done = true;
		}
	
	usleep(1000000);

	// read in the image into our local frame buffer
	fcUsb_cmd_getRawFrame(camNum, imageHeight, imageWidth, frameBufferPtr);
	}	

	// calculate the offsets to use for all subsequent images
	fcImage_PRO_calcColOffsets(frameBufferPtr, imageWidth, imageHeight);

	// put the users' desired integration time back
	fcUsb_cmd_setIntegrationTime(camNum, savedIntegrationTime);

	gProWantColNormalization = savedWantNorm;
}

// routine to perform a 3x3 kernel filter on the image buffer
//
void fcImage_do_3x3_kernel(UInt16 imageHeight, UInt16 imageWidth, UInt16 *frameBuffer)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			*outputPtr;
    UInt16			aPixel;
    UInt32			accumPixel;
    UInt16			*tempBuffer;
	size_t			size;

	// this routine will work 'in place'.  We will first allocate a temporary image buffer	
	// we copy the image to it and then fill the original buffer with the filtered image
	//
	size = imageWidth * imageHeight * 2;		// 2 bytes/pixel
    tempBuffer = (UInt16 *) malloc(size);

	if (tempBuffer != NULL)
		{

		// copy the image buffer to my local storage
		memcpy( tempBuffer, frameBuffer, size );

		// Start at row '1'
		for (row = 1; row < (imageHeight - 1); row++)
			{		
			for (col = 1; col < (imageWidth - 1); col++)
				{
				inputPtr = tempBuffer;
				inputPtr = inputPtr + (row * imageWidth) + col;
				outputPtr = frameBuffer;
				outputPtr = outputPtr + (row * imageWidth) + col;

				accumPixel = 0;
				
				inputPtr = inputPtr - imageWidth - 1;			// 1
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 2
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 3
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr = inputPtr + imageWidth - 2;			// 4
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 5
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 6
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr = inputPtr + imageWidth - 2;			// 7
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 8
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				inputPtr++;										// 9
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;

				// divide by the kernel size
				accumPixel = accumPixel / 9;

				// put filtered value back
                *outputPtr = (UInt16)accumPixel;
		
				}
			}

		free (tempBuffer);
		}

}

// routine to perform a 5x5 kernel filter on the image buffer
//
void fcImage_do_5x5_kernel(UInt16 imageHeight, UInt16 imageWidth, UInt16 *frameBuffer)
{
	float			reference;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			*outputPtr;
    UInt16			aPixel;
    UInt32			accumPixel;
    UInt16			*tempBuffer;
	size_t			size;
	int				x, y;


	// this routine will work 'in place'.  We will first allocate a temporary image buffer	
	// we copy the image to it and then fill the original buffer with the filtered image
	//
	size = imageWidth * imageHeight * 2;		// 2 bytes/pixel
    tempBuffer = (UInt16 *) malloc(size);

	if (tempBuffer != NULL)
		{

		// copy the image buffer to my local storage
		memcpy( tempBuffer, frameBuffer, size );

		// Start at row '2'
		for (row = 2; row < (imageHeight - 2); row++)
			{		
			for (col = 2; col < (imageWidth - 2); col++)
				{
				inputPtr = tempBuffer;
				inputPtr = inputPtr + (row * imageWidth) + col;
				outputPtr = frameBuffer;
				outputPtr = outputPtr + (row * imageWidth) + col;

				accumPixel = 0;
				
				inputPtr = inputPtr - (3 * imageWidth) + 2;		

				for (y = 0; y < 5; y++)
					{
					inputPtr = inputPtr + imageWidth - 4;			
					aPixel = *inputPtr;
                    accumPixel = accumPixel + (UInt32)aPixel;

					for (x = 0; x < 4; x++)
						{
						inputPtr++;									
						aPixel = *inputPtr;
                        accumPixel = accumPixel + (UInt32)aPixel;
						}

					}


				// divide by the kernel size
				accumPixel = accumPixel / 25;

				// put filtered value back
                *outputPtr = (UInt16)accumPixel;
		
				}
			}

		free (tempBuffer);
		}

}

// routine to perform hot pixel removal filter on the image buffer
//
// algorithm looks for the center pixel ina 3x3 grid being more than 20%
// brighter than the brightest of the neigboring pixels.  If it is
// then it will replace it with the average of the neighboring pixels.
//
void fcImage_do_hotPixel_kernel(UInt16 imageHeight, UInt16 imageWidth, UInt16 *frameBuffer)
{
	float			floatBrightPixel;
	float			floatCenterPixel;
	int				row, col;
    UInt16			*inputPtr;
    UInt16			*outputPtr;
    UInt16			aPixel;
    UInt32			accumPixel;
    UInt16			*tempBuffer;
	size_t			size;
    UInt16			brightestNeighbor;
    UInt16			thisPixel;
	int				numHotPixels;
	char			buffer[200];


	// this routine will work 'in place'.  We will first allocate a temporary image buffer	
	// we copy the image to it and then fill the original buffer with the filtered image
	//
	size = imageWidth * imageHeight * 2;		// 2 bytes/pixel
    tempBuffer = (UInt16 *) malloc(size);

	if (tempBuffer != NULL)
		{
		numHotPixels = 0;

		// copy the image buffer to my local storage
		memcpy( tempBuffer, frameBuffer, size );

		// Start at row '1'
		for (row = 1; row < (imageHeight - 1); row++)
			{		
			for (col = 1; col < (imageWidth - 1); col++)
				{
				inputPtr = tempBuffer;
				inputPtr = inputPtr + (row * imageWidth) + col;
				outputPtr = frameBuffer;
				outputPtr = outputPtr + (row * imageWidth) + col;

				accumPixel        = 0;
				brightestNeighbor = 0;
				
				inputPtr = inputPtr - imageWidth - 1;			// 1
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr++;										// 2
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr++;										// 3
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr = inputPtr + imageWidth - 2;			// 4
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr++;										// 5 - center pixel
				aPixel = *inputPtr;
				thisPixel = aPixel;

				inputPtr++;										// 6
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr = inputPtr + imageWidth - 2;			// 7
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr++;										// 8
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				inputPtr++;										// 9
				aPixel = *inputPtr;
                accumPixel = accumPixel + (UInt32)aPixel;
				if (brightestNeighbor < aPixel)
					brightestNeighbor = aPixel;

				// divide by the number of surrounding pixels
				accumPixel = accumPixel / 8;

				floatBrightPixel = (float)brightestNeighbor;
				floatBrightPixel = floatBrightPixel * 1.2;

				floatCenterPixel = (float)thisPixel;

				if (floatCenterPixel > floatBrightPixel)
					{
					numHotPixels++;
					// substitute average
                    *outputPtr = (UInt16)accumPixel;
					}
		
				}
			}

		free (tempBuffer);
		}

//	sprintf(buffer, "fcImage_do_hotPixel_kernel numHotPixels = %d\n", numHotPixels);
//	Starfish_Log( buffer );

}

// This is the framework initialization routine and needs to be called once upon application startup
void fcUsb_init(void)
{
	int			i;
	size_t		size;
    UInt16		rows, cols;


	gDoLogging = true;	// default to do logging
    gDoSimulation = false;

	Starfish_Log("fcUsb_init routine\n");

    libusb_init(&gCtx);

    libusb_set_debug(gCtx, 3);

	if (!gFWInitialized)
		{
		for (i = 0; i < kNumCamsSupported; i++)
			{			
			gCamerasFound[i].camVendor       = 0;
			gCamerasFound[i].camRawProduct   = 0;
			gCamerasFound[i].camFinalProduct = 0;
			gCamerasFound[i].camRelease      = 0;
			gCamerasFound[i].dev             = 0;

			gReadBlack[i]					 = false;					// set if the host wishes to read the black pixels
			gDataXfrReadMode[i]				 = fc_classicDataXfr;		// the type of data transfers we will be using when taking picts
			gDataFormat[i]					 = fc_16b_data;				// the desired camera data format

			gCurrentIntegrationTime[i]       = 50;

			gRoi_left[i]   = 0;
			gRoi_top[i]    = 0;
			gRoi_right[i]  = 1279;
			gRoi_bottom[i] = 1023;

			gCameraImageFilter[i] = fc_filter_none;

			gBlackPedestal[i] = 0;
			}
		
		gRelease				= 0;
		gNumCamerasDiscovered   = 0;
		
		gFindCamState			= fcFindCam_notYetStarted;
		gFindCamPercentComplete = 0.0;		

		// allocate memory for the internal frame buffer
		rows = 2520;
		cols = 3364;
//		cols += 16;					// room for black cols
		size = rows * cols * 2;		// 2 bytes/pixel
        gFrameBuffer = (UInt16 *) malloc(size);

		gProWantColNormalization = true;

		gFWInitialized			= true;
		Starfish_Log("fcCamFw initialized\n");
		}
}

// Call this routine to enable / disable entrie in the log file
// by default, logging is turned off.  The log file will be created
// in C:\Program Files\fishcamp\starfish_log.txt
//
// loggingState = true to turn logging on
//
void fcUsb_setLogging(bool loggingState)
{
	gDoLogging = loggingState;
}

void fcUsb_setSimulation(bool simState)
{
    gDoSimulation = simState;
}

// This is the framework close routine and needs to be called just before application termination
void fcUsb_close(void)
{
	int			i;


	free (gFrameBuffer);


	for (i = 0; i < kNumCamsSupported; i++)
		{			
		gCamerasFound[i].camVendor       = 0;
		gCamerasFound[i].camRawProduct   = 0;
		gCamerasFound[i].camFinalProduct = 0;
		gCamerasFound[i].camRelease      = 0;

		if (gCamerasFound[i].dev != 0)
			{
			fcUsb_CloseCamera(i + 1);
			gCamerasFound[i].dev = 0;
			}
		}

	libusb_exit(gCtx);
}

// the prefered way of finding and opening a communications link to any of our supported cameras
// This routine will call fcUsb_OpenCameraDriver and fcUsb_CloseCameraDriver routines to do its job
//
// will return the actual number of cameras found. 
//
// be carefull, this routine can take a long time (> 5 secs) to execute
//
// Change added 2/28/07 to allow more time between looking for the RAW camera and
// looking for the final cmaera.  We will look up till 10 seconds for this to happen
// before giving up.
//
//int fcUsb_FindCameras(void)
//{
//	int			result;
//	int			i;
//	GUID		guid;
//	bool		done;
//	int			d;
//	char		buffer[200];
//
//
//	Starfish_Log("fcUsb_FindCameras routine\n");
//	
//	gFindCamState			= fcFindCam_looking4supported;
//	gFindCamPercentComplete = 20.0;
//		
//			
//	// create the GUID used in the driver's .INF file that was used 
//	// to match the driver to the starfish camera
//	FcStringToGUID("B0C944D5-FBAF-478e-8E70-59F80C297347", &guid);
//
//	// get a reference to the CyAPI library that handles communication
//	// to any of our cameras.
//	gUSBDevice = new CCyUSBDevice(NULL, guid);	// Create an instance of CCyUSBDevice
//														// Does not register for PnP events
//	
//	// Look for any starfish cameras
//	gNumCamerasDiscovered = gUSBDevice->DeviceCount();
//	sprintf(buffer, "fcCamFw gNumCamerasDiscovered = %d\n", gNumCamerasDiscovered);
//	Starfish_Log (buffer );
//
//	gFindCamState			= fcFindCam_initializingUSB;
//	gFindCamPercentComplete = 50.0;
//
//	if (gNumCamerasDiscovered > 0)
//		{
//		// find any RAW starfish cameras
//		d = 0;
//		done = false;
//		do {
//			gUSBDevice->Open(d);  // Open automatically calls Close( ) if necessary
//
//			if ((gUSBDevice->VendorID == 0x1887) && (gUSBDevice->ProductID == 0x0002))
//				{
//				// we found one of our RAW starfish cameras
//				// we need to download the 'gdr_usb.hex' file to the USB controller RAM and then execute it
//				RawDeviceAdded();
//				}
//
//
//			d++;
//			if (d >= gNumCamerasDiscovered)
//				{
//				done = true;
//				}
//
//			}  while (!done);
//		}
//
//	printOutDiscoveredCamerasDB();
//
//	gUSBDevice->~CCyUSBDevice();
//
//
//
//	Sleep(1000);	// 2/28/08  used to wait a fixed 3 seconds
//
//
//
//	// found any RAW starfish cameras and renumerated them to the programmed state of the USB controller
//
//	gFindCamState			= fcFindCam_initializingIP;
//	gFindCamPercentComplete = 80.0;
//
//
//	// get a reference to the CyAPI library that handles communication
//	// to any of our cameras.
//	gUSBDevice = new CCyUSBDevice(NULL, guid);	// Create an instance of CCyUSBDevice
//														// Does not register for PnP events
//	
//	// Look for any starfish cameras
//	// we will look every second for up till 10 seconds.
//	// used to just look once  2/28/08
//	i = 0;
//	done = false;
//	while (!done)
//		{
//		gNumCamerasDiscovered = gUSBDevice->DeviceCount();
//		sprintf(buffer, "fcCamFw gNumCamerasDiscovered = %d\n", gNumCamerasDiscovered);
//		Starfish_Log( buffer );
//
//		i++;
//		if ((gNumCamerasDiscovered > 0) || (i > 10))
//			done = true;
//
//		Sleep(1000);
//		}
//
//	if (gNumCamerasDiscovered > 0)
//		{
//		// find any RAW starfish cameras
//		d = 0;
//		done = false;
//		do {
//			gUSBDevice->Open(d);  // Open automatically calls Close( ) if necessary
//
//			// make sure device is reset
//			gUSBDevice->Reset();
//			gUSBDevice->Open(d);
//
//			if ((gUSBDevice->VendorID == 0x1887) && (gUSBDevice->ProductID == 0x0003))
//				{
//				// we found one of our RAW starfish cameras
//				// we need to download the 'gdr_usb.hex' file to the USB controller RAM and then execute it
//				NewDeviceAdded();
//				}
//
//
//			d++;
//			if (d >= gNumCamerasDiscovered)
//				{
//				done = true;
//				}
//
//			}  while (!done);
//
//		}
//
//	printOutDiscoveredCamerasDB();
//
//
//
//	gFindCamState			= fcFindCam_finished;
//	gFindCamPercentComplete = 100.0;
//
//
//
//	if (gNumCamerasDiscovered > 0)
//		{
//		// set some default timeouts
//		gUSBDevice->BulkInEndPt->TimeOut  = 20000;		// increased to 20000 from 10000   6_13_10
//		gUSBDevice->BulkOutEndPt->TimeOut = 20000;		// increased to 20000 from 10000   6_13_10
//
//		// see results
////		printOutDiscoveredCamerasDB();
//		
//		// set some camera defaults
//		for (i = 0; i < gNumCamerasDiscovered; i++)
//			{
//			if (gCamerasFound[i].camFinalProduct == starfish_mono_rev2_final_deviceID)	// is this a Starfish?
//				{
//				sprintf(buffer, "Found Starfish - SN%04d\n", gCamerasFound[i].camRelease);
//				Starfish_Log( buffer );
//				fcUsb_setStarfishDefaultRegs(i+1);
//				}
//			}
//
//		}
//
//	return gNumCamerasDiscovered;
//}





// will return the actual number of cameras found. 
//
//int fcUsb_FindCameras(struct libusb_context *ctx)
int fcUsb_FindCameras(void)
{
 	struct libusb_config_descriptor *cdesc;
	struct libusb_device **devs_list;
	struct libusb_interface_descriptor *idesc;
	struct libusb_endpoint_descriptor *edesc;
	struct libusb_device_descriptor descriptor;
	libusb_device *device;
	int cnt;
 	int ret;
	int i, j, k, l;
 	int retValue, err;

    UInt16		vendor;
    UInt16		product;
    UInt16		release;
	int			index;
	int			kr;
 	char		buffer[200];

	Starfish_Log("fcUsb_FindCameras routine\n");

	gNumCamerasDiscovered   = 0;	
	gFindCamState			= fcFindCam_looking4supported;
	gFindCamPercentComplete = 20.0;

    if (gDoSimulation)
    {
        gFindCamState			= fcFindCam_finished;
        gFindCamPercentComplete = 100.0;

        gCamerasFound[0].camVendor       = fishcamp_USB_VendorID;
        gCamerasFound[0].camRawProduct   = starfish_mono_rev2_raw_deviceID;
        gCamerasFound[0].camFinalProduct = starfish_mono_rev2_final_deviceID;
        gCamerasFound[0].camRelease      = 12345;
        gCamerasFound[0].dev             = 0;

        gNumCamerasDiscovered = 1;

        return gNumCamerasDiscovered;
    }

	if ((cnt = libusb_get_device_list(gCtx, &devs_list)) < 0) 
		{
		sprintf(buffer, "  libusb_get_device_list failed with 0x%x error code\n", cnt);
		Starfish_Log( buffer );
 		return (EXIT_FAILURE);
 		}
 
	if (cnt == 0) 
		{
 		sprintf( buffer, "  No device match or lack of permissions.\n");
		Starfish_Log( buffer );
 		return (EXIT_SUCCESS);
	 	}

 	sprintf( buffer, "There are %i devices\n", cnt);
	Starfish_Log( buffer );
	for (i = 0 ; i < cnt ; i++) 
		{
		sprintf( buffer, "|-- device number = %i\n", i);
		Starfish_Log( buffer );
		device = devs_list[i];
		err = libusb_get_device_descriptor(device, &descriptor);

		vendor  = descriptor.idVendor;
		product = descriptor.idProduct;
		release = descriptor.bcdDevice;

		sprintf(buffer, "|---- vendor  = %08x\n", vendor);
		Starfish_Log( buffer );
		sprintf(buffer, "|---- product = %08x\n", product);
		Starfish_Log( buffer );
		sprintf(buffer, "|---- release = %08x\n", release);
		Starfish_Log( buffer );

		if ((vendor == fishcamp_USB_VendorID) && (product == starfish_mono_rev2_raw_deviceID))
			{
			// we found one of our RAW starfish cameras
			// put the new RAW camera in our data base
			if (!sawThisCameraAlready (vendor, product, 0xffff, release))
				{
				index = getNextFreeDBIndex();
		
				gCamerasFound[index].camVendor       = vendor;
				gCamerasFound[index].camRawProduct   = product;
				gCamerasFound[index].camFinalProduct = 0;
				gCamerasFound[index].camRelease      = release;
				gCamerasFound[index].dev             = 0;

				sprintf(buffer, "|---- RawDeviceAdded - added RAW camera to DB index at index = %d\n", index);
				Starfish_Log( buffer );
				}

			}

		if ((vendor == fishcamp_USB_VendorID) && (product == starfish_mono_rev2_final_deviceID))
			{
			// put the new initialized camera in our data base
			// at this point only the RAW version of the camera will be in the data base, if at all.  It
			// won't be in it at all if we re-started the application after previously been running
			// and the camera is already initialized from before.
			// by convention, the RAW productID will be an even number, the initialized version will be odd.
			// we use this knowledge to look for any previous entries of the RAW version of this camera.
			if (!sawThisCameraAlready (vendor, product & 0xfffe, 0xffff, release))
				{
				// here if we haven't seen this camera in its RAW state
				
				// check to see if it was already addded in its initialized state.  add it if not.
				if (getCameraDBIndex (vendor, 0xffff, product, release) == -1)
					{
					index = getNextFreeDBIndex();
			
					gCamerasFound[index].camVendor       = vendor;
					gCamerasFound[index].camRawProduct   = product & 0xfffe;
					gCamerasFound[index].camFinalProduct = product;
					gCamerasFound[index].camRelease      = release;
					gCamerasFound[index].dev             = 0;
				
					sprintf(buffer, "|---- NewDeviceAdded - added FINAL camera to DB at index = %d\n", index);
					Starfish_Log( buffer );
					}
				else
					Starfish_Log("|---- NewDeviceAdded -  FINAL camera already in DB\n");
				}
			else
				{
				index =  getCameraDBIndex (vendor, product & 0xfffe, 0xffff, release);
			
				gCamerasFound[index].camVendor       = vendor;
				// don't touch the following field.  we already saw this camera added as a RAW device
//				gCamerasFound[index].camRawProduct   = 0;
				gCamerasFound[index].camFinalProduct = product;
				gCamerasFound[index].camRelease      = release;
				gCamerasFound[index].dev             = 0;

				sprintf( buffer, "|---- NewDeviceAdded - updating RAW camera at DB index = %d\n", index);
				Starfish_Log( buffer );
				}
			}


		retValue = libusb_get_active_config_descriptor(devs_list[i], &cdesc);

		if (retValue == LIBUSB_SUCCESS) 
			{
			sprintf( buffer, "|---- bLength : 0x%.2x\n", cdesc->bLength);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- bDescriptorType : 0x%.2x\n", cdesc->bDescriptorType);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- wTotalLength : 0x%.2x\n", cdesc->wTotalLength);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- bNumInterfaces : 0x%.2x\n", cdesc->bNumInterfaces);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- bConfigurationValue : 0x%.2x\n", cdesc->bConfigurationValue);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- iConfiguration : 0x%.2x\n", cdesc->iConfiguration);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- bmAttributes : 0x%.2x\n", cdesc->bmAttributes);
			Starfish_Log( buffer );
			sprintf( buffer, "|---- MaxPower : 0x%.2x\n", cdesc->MaxPower);
			Starfish_Log( buffer );
						
//			for (j = 0 ; j < cdesc->bNumInterfaces ; j++) 
//				{
//				for (k = 0 ; k < cdesc->interface[j].num_altsetting ; k++) 
//					{
//					idesc = &cdesc->interface[j].altsetting[k];
//					printf("|---- INTERFACE :\n");
//					printf("|------ Interface %i%i\n", j, k);
//					printf("|------ bLength 0x%.2x\n", idesc->bLength);
//					printf("|------ bDescriptorType 0x%.2x\n", idesc->bDescriptorType);
//					printf("|------ bInterfaceNumber 0x%.2x\n", idesc->bInterfaceNumber);
//					printf("|------ bAlternateSetting 0x%.2x\n", idesc->bAlternateSetting);
//					printf("|------ bNumEndpoints 0x%.2x\n", idesc->bNumEndpoints);
//					printf("|------ bInterfaceClass 0x%.2x\n", idesc->bInterfaceClass);
//					printf("|------ bInterfaceSubClass 0x%.2x\n", idesc->bInterfaceSubClass);
//					printf("|------ bInterfaceProtocol 0x%.2x\n", idesc->bInterfaceProtocol);
//					printf("|------ iInterface 0x%.2x\n|\n", idesc->iInterface);
//					for (l = 0 ; l < idesc->bNumEndpoints ; l++) 
//						{
//						edesc = &idesc->endpoint[l];
//						printf("|------ ENDPOINT DESCRIPTOR :\n");
//						printf("|-------- bLength 0x%.2x\n", edesc->bLength);
//						printf("|-------- bDescriptorType 0x%.2x\n", edesc->bDescriptorType);
//						printf("|-------- bEndpointAddress 0x%.2x\n", edesc->bEndpointAddress);
//						printf("|-------- bmAttributes 0x%.2x\n", edesc->bmAttributes);
//						printf("|-------- wMaxPacketSize 0x%.4x\n", edesc->wMaxPacketSize);
//						printf("|-------- bInterval 0x%.2x\n", edesc->bInterval);
//						printf("|-------- bRefresh 0x%.2x\n", edesc->bRefresh);
//						printf("|-------- bSynchAddress 0x%.2x\n|\n", edesc->bSynchAddress);
//						}	// for l
//					}	// for k
//				}	// for j
 			} 	// if (retValue == LIBUSB_SUCCESS)
		else 
			{
 			sprintf( buffer, "libusb_get_active_config_descriptor failed\n");
			Starfish_Log( buffer );
			return (EXIT_FAILURE);
 			}

		}	// for i

	libusb_free_config_descriptor(cdesc);

	printOutDiscoveredCamerasDB();

	gFindCamState			= fcFindCam_finished;
	gFindCamPercentComplete = 100.0;

	gNumCamerasDiscovered = getNextFreeDBIndex();
	return gNumCamerasDiscovered;
}


// call this routine to know how what state the fcUsb_FindCameras routine is currently executing
// will return a result of fcFindCamState type
//
int fcUsb_GetFindCamsState(void)
{
	return gFindCamState;
}


// call this routine to know how long it will take for the fcUsb_FindCameras routine to complete.
//
float fcUsb_GetFindCamsPercentComplete(void)
{
	return gFindCamPercentComplete;
}

// need to open the camera before using it.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_OpenCamera(int camNum)
{
	int			result;
	int			i;
	bool		done;
	int			d;
	char		buffer[200];
    UInt16		vendor;
    UInt16		product;
    UInt16		release;
    struct libusb_device_handle *usb_handle;
	int			kr;
    int			tries;
	int 		retval;

	Starfish_Log("fcUsb_OpenCamera routine\n");
	
    if (gDoSimulation)
    {
        sprintf(buffer, "Found Starfish - SN%04d\n", gCamerasFound[camNum - 1].camRelease);
        Starfish_Log( buffer );
        fcUsb_setStarfishDefaultRegs(camNum);
        return 0;
    }

	if (gCamerasFound[camNum - 1].dev == 0)
		{
		// this camera wasn't being used yet
		vendor = gCamerasFound[camNum - 1].camVendor;

		if (vendor != 0)
			{
			// this camera has a DB entry
			product = gCamerasFound[camNum - 1].camFinalProduct;
			if (product == 0)
				{
				// we have a RAW starfish camera being opened
				product = gCamerasFound[camNum - 1].camRawProduct;

                sprintf(buffer, "Opening raw USB device with vendor: %08x prodcut: %08x\n", vendor, product);
                Starfish_Log(buffer);
				usb_handle = libusb_open_device_with_vid_pid(gCtx, vendor, product);
				gCamerasFound[camNum - 1].dev = usb_handle;
			    if (usb_handle == NULL)
					{
			    	Starfish_Log("Unable to open the raw USB device\n");
                    return -1;
				    }

				// the ReleaseNumber is used by fishcamp as a camera serial number.  Store it away
				// make sure we store the camera serial number in gRelease.
				// the DownloadToAnchorDevice routine will make sure the newly renumerated
				// camera will have the same serial number.
				release = gCamerasFound[camNum - 1].camRelease;
				gRelease = release;

			    kr = DownloadToAnchorDevice(usb_handle, vendor, product);
			    if (0 != kr)
					{
					sprintf(buffer, "unable to download to device: %08x\n", kr);
					Starfish_Log( buffer );
                    libusb_close(usb_handle);
                    return -1;
			        }

			    libusb_close(usb_handle);
				usb_handle = NULL;
				gCamerasFound[camNum - 1].dev = usb_handle;


				// found a RAW starfish cameras and renumerated it to the programmed state of the USB controller
				// the RAW camera has been renumerated.  Wait for it to brecome ready
				// takes about three seconds for the device to reset and my laptop to find it again. Wait for 10.
				product = product + 1;
   				for (tries=0; tries < 25; tries++) 
					{
					usb_handle = libusb_open_device_with_vid_pid(gCtx, vendor, product);
					if (usb_handle)
						{
						gCamerasFound[camNum - 1].camFinalProduct = product;
						break;
						}
					usleep(400 * 1000);
					}


				// let the device settle -- sometimes we're able to open it before it's ready to accept the S-Record download
				usleep(1000*1000);

				gCamerasFound[camNum - 1].dev = usb_handle;
			    if (usb_handle == NULL)	
					{
			    	Starfish_Log("Unable to open the final USB device\n");
				    }
				else
					{
					retval = libusb_claim_interface(usb_handle, 0);
					if (retval < 0)
						Starfish_Log("Couldn't claim interface 0\n");

					// one last thing....
					// need to load the MicroBlaze executable program before doing any camera stuff.

					if (gCamerasFound[camNum - 1].camRawProduct != 0)	// if the camera was discovered in the RAW state.  we will know this cause it will be non-zero
						{
						Starfish_Log("Calling - DownloadtToMicroBlaze \n");
						DownloadtToMicroBlaze(camNum);
						}
					else
						{
						Starfish_Log("didn't need to call - DownloadtToMicroBlaze \n");
						}

//				    download_srec(usb_handle, argv[1], FC_STARFISH_BULK_OUT_ENDPOINT);
					}
				}	//(product == 0)	// we have a RAW starfish camera being opened
			else
				{
				// we have a FINAL starfish camera being opened
				Starfish_Log("we have a FINAL starfish camera being opened.\n");

				usb_handle = libusb_open_device_with_vid_pid(gCtx, vendor, product);
				
				gCamerasFound[camNum - 1].dev = usb_handle;
			    if (usb_handle == NULL)	
					{
			    	Starfish_Log("Unable to open the final USB device\n");
				    }
				else
					{
					retval = libusb_claim_interface(usb_handle, 0);
					if (retval < 0)
						Starfish_Log("Couldn't claim interface 0\n");

					if (gCamerasFound[camNum - 1].camFinalProduct == starfish_mono_rev2_final_deviceID)	// is this a Starfish?
						{
						sprintf(buffer, "Found Starfish - SN%04d\n", gCamerasFound[camNum - 1].camRelease);
						Starfish_Log( buffer );
						fcUsb_setStarfishDefaultRegs(camNum);
						}
					}
				}	// we have a FINAL starfish camera being opened
			}	// if - this camera has a DB entry
		}	 // this camera wasn't being used yet


//	printOutDiscoveredCamerasDB();

	return 0;
}

// call this routine after you are finished making calls to this camera
// This routine will free the designated camera for other applications to use
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_CloseCamera(int camNum)
{
    if (gDoSimulation)
        return 0;

	if (gCamerasFound[camNum - 1].dev)
		{
		libusb_release_interface(gCamerasFound[camNum - 1].dev, 0);
		libusb_reset_device(gCamerasFound[camNum - 1].dev);
    	libusb_close(gCamerasFound[camNum - 1].dev);

		gCamerasFound[camNum - 1].dev = NULL;
		}

}

// call this routine to know how many of our supported cameras are available for use.
//
int fcUsb_GetNumCameras(void)
{
	return gNumCamerasDiscovered;
}

bool fcUsb_haveCamera(void)
{
	bool	haveCamera;

	haveCamera = true;
	
	// do we have at least one camera?
	if (gNumCamerasDiscovered == 0)
		haveCamera = false;
		
	return haveCamera;
}

// return the numeric serial number of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_GetCameraSerialNum(int camNum)
{
	return gCamerasFound[camNum - 1].camRelease;
}

// return the numeric vendorID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int	fcUsb_GetCameraVendorID(int camNum)
{
	return gCamerasFound[camNum - 1].camVendor;
}

// return the numeric productID of the camera specified.
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int	fcUsb_GetCameraProductID(int camNum)
{
	return gCamerasFound[camNum - 1].camFinalProduct;
}

// send the nop command to the starfish camera
//
int fcUsb_cmd_nop(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;

	Starfish_Log("fcUsb_cmd_nop\n");

    if (gDoSimulation)
        return 0;
	
    myParameters.header = 'fc';
	myParameters.command = fcNOP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// send the rst command to the starfish camera
//
int fcUsb_cmd_rst(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;

	Starfish_Log("fcUsb_cmd_rst\n");

    if (gDoSimulation)
        return 0;
	
	myParameters.header = 'fc';
	myParameters.command = fcRST;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// send the fcGETINFO command to the starfish camera
// read the return information
//
int	fcUsb_cmd_getinfo(int camNum, fc_camInfo *camInfo)
{
    UInt32		msgSize;
    UInt32		numBytesRead;
	fc_no_param	myParameters;
    UInt16		*wordPtr1;
    UInt16		*wordPtr2;
	int			i;
	char		buffer[200];
	int			maxBytes;
	
	Starfish_Log("fcUsb_cmd_getinfo\n");

    if (gDoSimulation)
    {
        camInfo->boardRevision = 1;
        camInfo->boardVersion = 2;
        camInfo->fpgaRevision = 1;
        camInfo->fpgaVersion = 2;
        camInfo->pixelHeight = 7;
        camInfo->pixelWidth = 7;
        camInfo->width = 1280;
        camInfo->height = 1024;
    }
    else
    {
        // send the command to the camera
        myParameters.header = 'fc';
        myParameters.command = fcGETINFO;
        myParameters.length = sizeof(myParameters);
        myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

        msgSize = sizeof(myParameters);
        SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);

        // get the response to the command
        maxBytes = 512;
        numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);


        // we have the ACK message in gBuffer.  It is proceeded by two words before the fc_camInfo
        // data.  copy the real data to the user's data structure.
        wordPtr1 = (UInt16 *)&gBuffer[4];
        wordPtr2 = (UInt16 *)camInfo;

        for (i = 0; i < sizeof(fc_camInfo); i = i + 2)
            *wordPtr2++ = *wordPtr1++;

    }

	// bug fix.  The starfish puts out garbled text for the camera name string
	camInfo->camNameStr[0]  = 'S';
	camInfo->camNameStr[1]  = 't';
	camInfo->camNameStr[2]  = 'a';
	camInfo->camNameStr[3]  = 'r';
	camInfo->camNameStr[4]  = 'f';
	camInfo->camNameStr[5]  = 'i';
	camInfo->camNameStr[6]  = 's';
	camInfo->camNameStr[7]  = 'h';
	camInfo->camNameStr[8]  = ' ';
	camInfo->camNameStr[9]  = 'M';
	camInfo->camNameStr[10] = 'o';
	camInfo->camNameStr[11] = 'n';
	camInfo->camNameStr[12] = 'o';
	camInfo->camNameStr[13] = 0;

	// also need to fix up the serial number
	sprintf( buffer, "%04x-%04x-%04x-%04x", 	gCamerasFound[camNum-1].camVendor, 
												gCamerasFound[camNum-1].camRawProduct, 
												gCamerasFound[camNum-1].camFinalProduct, 
												gCamerasFound[camNum-1].camRelease);
	for (i = 0; i < 32; i++)
		camInfo->camSerialStr[i] = (UInt8)buffer[i];

	camInfo->camSerialStr[19] = 0;

    // JM (2013-12-13) camInfo->pixelWidth returns 1312 (0x520) which is wrong
    camInfo->pixelWidth = 52;
    camInfo->pixelHeight = 52;

	// print out the information returned
	Starfish_Log("fcUsb_cmd_getinfo:\n");
	sprintf(buffer, "     boardVersion  - 0x%02x\n", camInfo->boardVersion);
	Starfish_Log( buffer );
	sprintf(buffer, "     boardRevision - 0x%02x\n", camInfo->boardRevision);
	Starfish_Log( buffer );
	sprintf(buffer, "     fpgaVersion   - 0x%02x\n", camInfo->fpgaVersion);
	Starfish_Log( buffer );
	sprintf(buffer, "     fpgaRevision  - 0x%02x\n", camInfo->fpgaRevision);
	Starfish_Log( buffer );
	sprintf(buffer, "     width         - 0x%02x\n", camInfo->width);
	Starfish_Log( buffer );
	sprintf(buffer, "     height        - 0x%02x\n", camInfo->height);
	Starfish_Log( buffer );
	sprintf(buffer, "     pixelWidth    - 0x%02x\n", camInfo->pixelWidth);
	Starfish_Log( buffer );
	sprintf(buffer, "     pixelHeight   - 0x%02x\n", camInfo->pixelHeight);
	Starfish_Log( buffer );
	sprintf(buffer, "     camSerialStr  - %s\n",     camInfo->camSerialStr);
	Starfish_Log( buffer );
	sprintf(buffer, "     camNameStr    - %s\n",     camInfo->camNameStr);
	Starfish_Log( buffer );
	
	
	return 0;
}

// call to set a low level register in the camera
//
// for Starfish camera:
//		- call to set a low level register in the micron image sensor
//		- enter with the micron address register, and register data value
//			refer to the micron image sensor documentation for details 
//			on available registers and bit definitions
//
// for IBIS1300
//		- call to set the serial configuration word on the camera
//		- enter with regAddress = 0, dataValue
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_cmd_setRegister(int camNum, UInt16 regAddress, UInt16 dataValue)
{

    UInt32			msgSize;
	fc_setReg_param	myParameters;
    UInt32			numBytesRead;
	int				maxBytes;

	
	// print out the information 
//	printf("fcUsb_cmd_setRegister:\n");
//	printf("     address - 0x%04x\n", regAddress);
//	printf("     data    - 0x%04x\n", dataValue);

	Starfish_Log("fcUsb_cmd_setRegister\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETREG;
	myParameters.length = sizeof(myParameters);
	myParameters.registerAddress = regAddress;
	myParameters.dataValue = dataValue;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// call to get a low level register from the micron image sensor
//

UInt16 fcUsb_cmd_getRegister(int camNum, UInt16 regAddress)
{
    UInt32			msgSize;
	fc_getReg_param	myParameters;
    UInt32			numBytesRead;
    UInt16			retValue;
	fc_regInfo		myRegInfo;
	char			buffer[200];
	int				maxBytes;

	
	Starfish_Log("fcUsb_cmd_getRegister\n");

    if (gDoSimulation)
        return 0;
	
	myParameters.header = 'fc';
	myParameters.command = fcGETREG;
	myParameters.length = sizeof(myParameters);
	myParameters.registerAddress = regAddress;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);

//	sprintf(buffer, "     myParameters.header           - 0x%04x\n", myParameters.header);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     myParameters.command          - 0x%04x\n", myParameters.command);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     myParameters.length           - 0x%04x\n", myParameters.length);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     myParameters.registerAddress  - 0x%04x\n", myParameters.registerAddress);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     myParameters.cksum            - 0x%04x\n", myParameters.cksum);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     msgSize                       - 0x%08x\n", msgSize);
//	Starfish_Log( buffer );

//	sprintf(buffer, "     gUSBDevice                    - 0x%08x\n", gUSBDevice);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     BulkOutEndPt                  - 0x%08x\n", gUSBDevice->BulkOutEndPt);
//	Starfish_Log( buffer );

	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
//	sprintf(buffer, "     msgSize                       - 0x%08x\n", msgSize);
//	Starfish_Log( buffer );   

	// get the ACK to the command
	maxBytes = 512;

//	sprintf(buffer, "     numBytesRead                  - 0x%08x\n", numBytesRead);
//	Starfish_Log( buffer );

	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myRegInfo, &gBuffer, sizeof(myRegInfo) );
	
//	// print out the information returned
//	sprintf(buffer, "fcUsb_cmd_getRegister:\n");
//	Starfish_Log( buffer );
//	sprintf(buffer, "     address       - 0x%04x\n", regAddress);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     header        - 0x%04x\n", myRegInfo.header);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     command       - 0x%04x\n", myRegInfo.command);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     dataValue     - 0x%04x\n", myRegInfo.dataValue);
//	Starfish_Log( buffer );
//	sprintf(buffer, "     numBytesRead  - 0x%08x\n", numBytesRead);
//	Starfish_Log( buffer );

	return myRegInfo.dataValue;
}

// call to set the integration time register.  only 22 LSB significant  (69 minutes with 1ms resolution)
//
int fcUsb_cmd_setIntegrationTime(int camNum, UInt32 theTime)
{
    UInt32				msgSize;
	fc_setIntTime_param	myParameters;
    UInt32				numBytesRead;
    UInt16				aWord;
	int					maxBytes;

	Starfish_Log("fcUsb_cmd_setIntegrationTime\n");

    if (gDoSimulation)
        return 0;

	// store the time away so we can decide if we want to read the black columns
	gCurrentIntegrationTime[camNum - 1] = theTime;

	// special handling for the starfish guide camera
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_mono_rev2_final_deviceID)
		{
		// tell the camera the new setting of the read black cols mode bit.  We will read the
		// black cols anytime the integratiion time is less than 2 seconds.
		fcUsb_cmd_setReadMode(camNum, gDataXfrReadMode[camNum - 1], gDataFormat[camNum - 1]);
		// resend the ROI params.  the fcUsb_cmd_setReadMode routine will have set the gReadBlack variable
		// so the next routine will trick the camera into using a wider ROI.
		fcUsb_cmd_setRoi(camNum, gRoi_left[camNum - 1], gRoi_top[camNum - 1], gRoi_right[camNum - 1], gRoi_bottom[camNum - 1]);
		}


	myParameters.header = 'fc';
	myParameters.command = fcSETINTTIME;
	myParameters.length = sizeof(myParameters);
	aWord = theTime >> 16;
	aWord = aWord & 0x0ffff;
	myParameters.timeHi = aWord;
	aWord = theTime & 0x0ffff;
	myParameters.timeLo = aWord;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// command to set the integration time period of the guider portion of
// the IBIS1300 image sensor.  This command is not recognized by the Starfish camera
//
int fcUsb_cmd_setGuiderIntegrationTime(int camNum, UInt32 theTime)
{
    UInt32				msgSize;
	fc_setIntTime_param	myParameters;
    UInt32				numBytesRead;
    UInt16				aWord;
	int					maxBytes;


	Starfish_Log("fcUsb_cmd_setGuiderIntegrationTime\n");

    if (gDoSimulation)
        return 0;

	// store the time away so we can decide if we want to read the black columns
	gCurrentIntegrationTime[camNum - 1] = theTime;

	// only the ibis1300 camera supports this right now
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		{
		// store the time away so we can decide if we want to read the black columns
		gCurrentGuiderIntegrationTime[camNum - 1] = theTime;

		myParameters.header = 'fc';
		myParameters.command = fcSETGUIDEINTTIME;
		myParameters.length = sizeof(myParameters);
		aWord = theTime >> 16;
		aWord = aWord & 0x0ffff;
		myParameters.timeHi = aWord;
		aWord = theTime & 0x0ffff;
		myParameters.timeLo = aWord;
		myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

		msgSize = sizeof(myParameters);
		SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
		// get the ACK to the command
		maxBytes = 512;
		numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);
		}

	return 0;
}

// send the 'start exposure' command to the camera
//
int fcUsb_cmd_startExposure(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;

	Starfish_Log("fcUsb_cmd_startExposure\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSTARTEXP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// send the 'start exposure' command to the guider portion of the
// IBIS1300 image sensor.    This command is not recognized by the Starfish camera
//
int fcUsb_cmd_startGuiderExposure(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;
	
	Starfish_Log("fcUsb_cmd_startGuiderExposure\n");

    if (gDoSimulation)
        return 0;

	// only the ibis1300 camera supports this right now
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		{
		myParameters.header = 'fc';
		myParameters.command = fcSTARTGUIDEEXP;
		myParameters.length = sizeof(myParameters);
		myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

		msgSize = sizeof(myParameters);
		SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
		// get the ACK to the command
		maxBytes = 512;
		numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);
		}

	return 0;
}

// send the 'abort exposure' command to the camera
//
int fcUsb_cmd_abortExposure(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;

	Starfish_Log("fcUsb_cmd_abortExposure\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcABORTEXP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// send the 'abort Guider exposure' command to the camera
// This command is not recognized by the Starfish camera
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
//
int fcUsb_cmd_abortGuiderExposure(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;

	Starfish_Log("fcUsb_cmd_abortGuiderExposure\n");

    if (gDoSimulation)
        return 0;

	// only the ibis1300 camera supports this right now
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		{
		myParameters.header = 'fc';
		myParameters.command = fcABORTGUIDEEXP;
		myParameters.length = sizeof(myParameters);
		myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

		msgSize = sizeof(myParameters);
		SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
		// get the ACK to the command
		maxBytes = 512;
		numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);
		}

	return 0;
}

// send a command to get the current camera state
//
UInt16 fcUsb_cmd_getState(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
    UInt16		retValue;
	fc_regInfo	myRegInfo;
	int			maxBytes;

    if (gDoSimulation)
        return 0;
	
//	printf("fcUsb_cmd_getState:\n");
//	Starfish_Log("fcUsb_cmd_getState\n");

	myParameters.header = 'fc';
	myParameters.command = fcGETSTATE;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
//	printf("fcUsb_cmd_getState -  about to read:\n");

	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myRegInfo, &gBuffer, sizeof(myRegInfo) );

	// print out the information returned
//	printf("fcUsb_cmd_getState:\n");
//	printf("     dataValue  - 0x%02x\n", myRegInfo.dataValue);

	usleep(10000);


	return myRegInfo.dataValue;
}

// send a command to get the current camera state of the guider sensor
// This command is not recognized by the Starfish camera
//
// valid camNum is 1 -> fcUsb_GetNumCameras()
// return values are:
//	0 - idle
//	1 - integrating
//	2 - processing image
//
UInt16 fcUsb_cmd_getGuiderState(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
    UInt16		retValue;
	fc_regInfo	myRegInfo;
	int			maxBytes;

	
//	printf("fcUsb_cmd_getGuiderState:\n");
	Starfish_Log("fcUsb_cmd_getGuiderState\n");

    if (gDoSimulation)
        return 0;

	// only the ibis1300 camera supports this right now
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		{
		myParameters.header = 'fc';
		myParameters.command = fcGETGUIDESTATE;
		myParameters.length = sizeof(myParameters);
		myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

		msgSize = sizeof(myParameters);
		SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
//		printf("fcUsb_cmd_getGuiderState -  about to read:\n");

		// get the ACK to the command
		maxBytes = 512;
		numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

		// copy the Rx buffer to my local storage
		memcpy( &myRegInfo, &gBuffer, sizeof(myRegInfo) );

		// print out the information returned
//		printf("fcUsb_cmd_getGuiderState:\n");
//		printf("     dataValue  - 0x%02x\n", myRegInfo.dataValue);

		usleep(10000000);
	
		return myRegInfo.dataValue;
		}
	else
		{
		return 0;		// idle state if not supported
		}
}

// turn on/off the frame grabber's test pattern generator
// 0 = off, 1 = on
//
int fcUsb_cmd_setFrameGrabberTestPattern(int camNum, UInt16 state)
{
    UInt32				msgSize;
	fc_setFgTp_param	myParameters;
    UInt32				numBytesRead;
    UInt16				retValue;
	int					maxBytes;

	
	Starfish_Log("fcUsb_cmd_setFrameGrabberTestPattern\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETFGTP;
	myParameters.length = sizeof(myParameters);
	myParameters.state = state;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// here to read a specific scan line from the camera's frame grabber buffer
//
int fcUsb_cmd_rdScanLine(int camNum, UInt16 lineNum, UInt16 Xmin, UInt16 Xmax, UInt16 *lineBuffer)
{
    UInt32				msgSize;
    UInt32				numBytesRead;
	fc_rdScanLine_param	myParameters;
    UInt16				dataOdd;
    UInt16				dataEven;
    UInt16				*wordPtr1;
    UInt16				*wordPtr2;
	int					i;
	int					scanLineSize;
	fc_scanLineInfo		myScanLineInfo;
	int					maxBytes;


	Starfish_Log("fcUsb_cmd_rdScanLine\n");

	// send the command to the camera
	myParameters.header = 'fc';
	myParameters.command = fcRDSCANLINE;
	myParameters.LineNum = lineNum;
	myParameters.padZero = 0;
	myParameters.Xmin = Xmin;
	myParameters.Xmax = Xmax;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);

	// get the response to the command
	maxBytes = (((Xmax - Xmin) + 1) * 2) + 12;		// bigger is OK
	maxBytes = 2048;
	maxBytes = sizeof(myScanLineInfo);
	maxBytes = 4608;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&myScanLineInfo, maxBytes);

	// we have the ACK message in myScanLineInfo.  It is proceeded by several words before the fc_scanLineInfo
	// data.  copy the real data to the user's data structure.
	wordPtr1 = &myScanLineInfo.lineBuffer[0];
	wordPtr2 = lineBuffer;
	
	scanLineSize = Xmax - Xmin + 1; 
	for (i = 0; i < scanLineSize; i = i + 2)
		{
//		*wordPtr2++ = *wordPtr1++;
		dataOdd = *wordPtr1++;
		dataEven = *wordPtr1++;
		
//		*wordPtr2++ = dataEven;
//		*wordPtr2++ = dataOdd;
		*wordPtr2++ = dataOdd;
		*wordPtr2++ = dataEven;
		}
	
	return 0;
}

// here to specify a new ROI to the sensor
// X = 0 -> 2047

// Y = 0 -> 1535
//
int fcUsb_cmd_setRoi(int camNum, UInt16 left, UInt16 top, UInt16 right, UInt16 bottom)
{
    UInt32			msgSize;
	fc_setRoi_param	myParameters;
    UInt32			numBytesRead;
    UInt16			regVal;
	int				maxBytes;

	Starfish_Log("fcUsb_cmd_setRoi\n");

	// store user requested number away
	gRoi_left[camNum - 1]   = left;
	gRoi_top[camNum - 1]    = top;
	gRoi_right[camNum - 1]  = right;
	gRoi_bottom[camNum - 1] = bottom;

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETROI;
	myParameters.length = sizeof(myParameters);
	myParameters.left = left;
	myParameters.top = top;
	myParameters.right = right;
	myParameters.bottom = bottom;

	// if we need to read the black cols, increase the ROI width appropriately
	if (gReadBlack[camNum - 1])
		myParameters.right = right + 16;

	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);
	
//	regVal = top + 20;	// default first row is 20
//	regVal = regVal & 0x7fe;	// 11 bits, must be even
//	fcUsb_cmd_setRegister(0x01, regVal);
//	regVal = left + 32;	// default first row is 32
//	regVal = regVal & 0xffe;	// 12 bits, must be even
//	fcUsb_cmd_setRegister(0x02, regVal);
//	regVal = bottom - top;	
//	regVal = regVal & 0x7ff;	// 11 bits, must be odd
//	regVal = regVal | 0x0001;
//	fcUsb_cmd_setRegister(0x03, regVal);
//	regVal = right - left;	
//	regVal = regVal & 0x7ff;	// 11 bits, must be odd
//	regVal = regVal | 0x0001;
//	fcUsb_cmd_setRegister(0x04, regVal);

		
	// test
//	regVal = fcUsb_cmd_getRegister(camNum, 0x01);
//	regVal = fcUsb_cmd_getRegister(camNum, 0x02);
//	regVal = fcUsb_cmd_getRegister(camNum, 0x03);
//	regVal = fcUsb_cmd_getRegister(camNum, 0x04);


	return 0;
}

// set the binning mode of the camera.  Valid binModes are 1, 2, 3
// 
int	fcUsb_cmd_setBin(int camNum, UInt16 binMode)
{
    UInt32			msgSize;
	fc_setBin_param	myParameters;
    UInt32			numBytesRead;
    UInt16			regVal;
	int				maxBytes;
	
	Starfish_Log("fcUsb_cmd_setBin\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETBIN;
	myParameters.length = sizeof(myParameters);
	myParameters.binMode = binMode;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);
		
	// test
//	regVal = fcUsb_cmd_getRegister(camNum, 0x22);
//	regVal = fcUsb_cmd_getRegister(camNum, 0x23);


	return 0;
}

// turn on one of the relays on the camera.  whichRelay is one of enum fc_relay
//
int	fcUsb_cmd_setRelay(int camNum, int whichRelay)
{
    UInt32					msgSize;
	fc_setClrRelay_param	myParameters;
    UInt32					numBytesRead;
    UInt16					regVal;
	int						maxBytes;
	
	Starfish_Log("fcUsb_cmd_setRelay\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETRELAY;
	myParameters.length = sizeof(myParameters);
	myParameters.relayNum = whichRelay;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// turn off one of the relays on the camera.  whichRelay is one of enum fc_relay
//
int	fcUsb_cmd_clearRelay(int camNum, int whichRelay)
{
    UInt32					msgSize;
	fc_setClrRelay_param	myParameters;
    UInt32					numBytesRead;
    UInt16					regVal;
	int						maxBytes;	

	Starfish_Log("fcUsb_cmd_clearRelay\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcCLRRELAY;
	myParameters.length = sizeof(myParameters);
	myParameters.relayNum = whichRelay;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// generate a pulse on one of the relays on the camera.  whichRelay is one of enum fc_relay
// pulse width parameters are in mS.  you can specify the hi and lo period of the pulse.
// if 'repeats' is true then the pulse will loop.
//
int	fcUsb_cmd_pulseRelay(int camNum, int whichRelay, int onMs, int offMs, bool repeats)
{
    UInt32				msgSize;
	fc_pulseRelay_param	myParameters;
    UInt32				numBytesRead;
    UInt16				regVal;
	int					maxBytes;	
	
	Starfish_Log("fcUsb_cmd_pulseRelay\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcPULSERELAY;
	myParameters.length = sizeof(myParameters);
	myParameters.relayNum = whichRelay;
	myParameters.highPulseWidth = onMs & 0x7fff;		// only 15 bits
	myParameters.lowPulseWidth = offMs & 0x7fff;		// only 15 bits
	if (repeats)
		myParameters.repeats = 1;
	else
		myParameters.repeats = 0;	
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// tell the camera what temperature setpoint to use for the TEC controller
// this will also turn on cooling to the camera
//
int fcUsb_cmd_setTemperature(int camNum, SInt16 theTemp)
{
    UInt32				msgSize;
	fc_setTemp_param	myParameters;
    UInt32				numBytesRead;
	int					maxBytes;	
	
    if (gDoSimulation)
        return 0;

	Starfish_Log("fcUsb_cmd_setTemperature\n");

	myParameters.header = 'fc';
	myParameters.command = fcSETTEMP;
	myParameters.length = sizeof(myParameters);
	myParameters.theTemp = theTemp;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// get the temperature of the image sensor
//
SInt16 fcUsb_cmd_getTemperature(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
    UInt16		regVal;
	fc_tempInfo	myTemperatureInfo;
	float		theCurTemperature;
	char		buffer[200];
	int			maxBytes;	
	
	Starfish_Log("fcUsb_cmd_getTemperature\n");

    if (gDoSimulation)
        return 25;

	myParameters.header = 'fc';
	myParameters.command = fcGETTEMP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myTemperatureInfo, &gBuffer, sizeof(myTemperatureInfo) );

	// put the current temperature in the log file
	theCurTemperature = (float)myTemperatureInfo.tempValue;
	theCurTemperature = theCurTemperature / 100.0;
	sprintf(buffer, "     Got temperature - %2.1f degrees C\n", theCurTemperature);
	Starfish_Log( buffer );

	return myTemperatureInfo.tempValue;
}

UInt16 fcUsb_cmd_getTECPowerLevel(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
    UInt16		regVal;
	fc_tempInfo	myTemperatureInfo;
	char		buffer[200];
    UInt16		theCurPower;
	int			maxBytes;	
	
	Starfish_Log("fcUsb_cmd_getTECPowerLevel\n");

    if (gDoSimulation)
        return 50;

	myParameters.header = 'fc';
	myParameters.command = fcGETTEMP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myTemperatureInfo, &gBuffer, sizeof(myTemperatureInfo) );

	// put the current power in the log file
	theCurPower = myTemperatureInfo.TECPwrValue;
	sprintf(buffer, "     Got power level - %d percent\n", theCurPower);
	Starfish_Log( buffer );


	return myTemperatureInfo.TECPwrValue;
}

bool fcUsb_cmd_getTECInPowerOK(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
    UInt16		regVal;
	fc_tempInfo	myTemperatureInfo;
	int			maxBytes;
		
	Starfish_Log("fcUsb_cmd_getTECInPowerOK\n");

    if (gDoSimulation)
        return true;

	myParameters.header = 'fc';
	myParameters.command = fcGETTEMP;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myTemperatureInfo, &gBuffer, sizeof(myTemperatureInfo) );

	if (myTemperatureInfo.TECInPwrOK == 0)
		return false;
	else
		return true;
}

// command camera to turn off the TEC cooler
//
int fcUsb_cmd_turnOffCooler(int camNum)
{
    UInt32		msgSize;
	fc_no_param	myParameters;
    UInt32		numBytesRead;
	int			maxBytes;	

	Starfish_Log("fcUsb_cmd_turnOffCooler\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcTURNOFFTEC;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// here to read an entire frame in RAW format
//
int fcUsb_cmd_getRawFrame(int camNum, UInt16 numRows, UInt16 numCols, UInt16 *frameBuffer)
{
    UInt32		msgSize;
    UInt32		numBytesRead;
	fc_no_param	myParameters;
    UInt16		dataOdd;
    UInt16		dataEven;
    UInt16		*wordPtr1;
    UInt16		*wordPtr2;
	int			i;
	char		errorString[513];
	int			maxBytes;	
	char		buffer[200];
	

	Starfish_Log("fcUsb_cmd_getRawFrame\n");

    if (gDoSimulation)
        return 0;

	// send the command to the camera
	myParameters.header = 'fc';
	myParameters.command = fcGETRAWFRAME;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the response to the command

	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_pro4m_final_deviceID)
		{
		maxBytes = numRows * numCols * 2;	// 2 bytes / pixel
		numBytesRead = RcvUSB(camNum, (unsigned char*)&frameBuffer, maxBytes);

        sprintf( buffer, "   read - %ld bytes\n", numBytesRead );
		Starfish_Log( buffer );


		if (gProWantColNormalization)
			fcImage_PRO_doFullFrameColLevelNormalization(frameBuffer, numCols, numRows);
		}
	else
		{
		if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
			{
			maxBytes = numRows * numCols * 2;	// 2 bytes / pixel
			numBytesRead = RcvUSB(camNum, (unsigned char*)&frameBuffer, maxBytes);
		
			fcImage_IBIS_doFullFrameColLevelNormalization(frameBuffer, numCols, numRows);
			fcImage_IBIS_subtractPedestal(frameBuffer, numCols, numRows);
			}
		else
			{
			// if we are doing black level compensation, then we read into our internal buffer
			// then strip out the balck cols after we are done with them
			if (gReadBlack[camNum - 1])
				{
				maxBytes = numRows * (numCols + 16) * 2;	// 2 bytes / pixel
				numBytesRead = RcvUSB(camNum, (unsigned char*)gFrameBuffer, maxBytes);

				}
			else
				{
				maxBytes = numRows * numCols * 2;	// 2 bytes / pixel
				numBytesRead = RcvUSB(camNum, (unsigned char*)frameBuffer, maxBytes);
				}

			sprintf( buffer, "   fcUsb_cmd_getRawFrame - numBytesRead - 0x%08x\n", (unsigned int)numBytesRead);
			Starfish_Log( buffer );

			if (gReadBlack[camNum - 1])
				{
				fcImage_doFullFrameRowLevelNormalization(gFrameBuffer, (numCols + 16), numRows);
				fcImage_StripBlackCols(camNum, frameBuffer);		
				}
			}	// if Starfish
		}


	if (gCameraImageFilter[camNum - 1] == fc_filter_3x3)
		{
		// perform 3x3 kernel filter
		fcImage_do_3x3_kernel(numRows, numCols, frameBuffer);
		}

	if (gCameraImageFilter[camNum - 1] == fc_filter_5x5)
		{
		// perform 5x5 kernel filter
		fcImage_do_5x5_kernel(numRows, numCols, frameBuffer);
		}

	if (gCameraImageFilter[camNum - 1] == fc_filter_hotPixel)
		{
		// perform hot pixel removal filter
		fcImage_do_hotPixel_kernel(numRows, numCols, frameBuffer);
		}



	return (numBytesRead);
}

// here to set the analog gain on the camera.
// 
// theGain the gain number desired.  
// 
// Valid gains are between 0 and 15.  
//		For the IBIS camera this corresponds to 1.28 to 17.53 as per the sensor data sheet
//
int fcUsb_cmd_setCameraGain(int camNum, UInt16 theGain)
{
    UInt32				msgSize;
	fc_setGain_param	myParameters;
    UInt32				numBytesRead;
	int					maxBytes;	
	
	Starfish_Log("fcUsb_cmd_setCameraGain\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETGAIN;
	myParameters.length = sizeof(myParameters);
	myParameters.theGain = theGain;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// if this is the IBIS1300 camera, then we need to get new black row averages
	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		fcImage_IBIS_computeAvgColOffsets(camNum);


	gBlackPedestal[camNum - 1] = fcUsb_cmd_getBlackPedestal(camNum);

	return 0;
}

// here to set the analog offset on the camera.
// 
// theOffset the offset number desired.  
// 
// For the IBIS camera valid offsets are between 0 and 15.
//
int fcUsb_cmd_setCameraOffset(int camNum, UInt16 theOffset)
{
    UInt32				msgSize;
	fc_setOffset_param	myParameters;
    UInt32				numBytesRead;
	int					maxBytes;	
	
	Starfish_Log("fcUsb_cmd_setCameraOffset\n");

    if (gDoSimulation)
        return 0;

	myParameters.header = 'fc';
	myParameters.command = fcSETOFFSET;
	myParameters.length = sizeof(myParameters);
	myParameters.theOffset = theOffset;
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	gBlackPedestal[camNum - 1] = fcUsb_cmd_getBlackPedestal(camNum);


	return 0;
}

// here to define some image readout modes of the camera.  The state of these bits will be 
// used during the call to fcUsb_cmd_startExposure.  When an exposure is started and subsequent
// image readout is begun, the camera will assume an implied fcUsb_cmd_getRawFrame command
// when the 'DataXfrReadMode' is a '1' or '2' and begin uploading pixel data to the host as the image 
// is being read from the sensor.  When the 'ReadBlack' bit is set, the first black rows and cols
// of pixels will also be read from the sensor.
// DataFormat can be one of 8, 10, 12, 14, 16
//     8 - packed into a single byte
//     others - packed into a 16 bit word
//
int	fcUsb_cmd_setReadMode(int camNum, int DataXfrReadMode, int DataFormat)
{
    UInt32					msgSize;
	fc_setReadMode_param	myParameters;
    UInt32					numBytesRead;
	bool					DoOffsetCorrection;
	bool					ReadBlack;
	int						maxBytes;		

	Starfish_Log("fcUsb_cmd_setReadMode\n");

    if (gDoSimulation)
        return 0;

	if (gCamerasFound[camNum - 1].camFinalProduct == starfish_ibis13_final_deviceID)
		{
		ReadBlack = false;
		DoOffsetCorrection = false;
		
		// remember settings
		gReadBlack[camNum - 1] = ReadBlack;					// set if the host wishes to read the black pixels
		gDataXfrReadMode[camNum - 1] = DataXfrReadMode;		// the type of data transfers we will be using when taking picts
		gDataFormat[camNum - 1] = DataFormat;				// the desired camera data format

		myParameters.header = 'fc';
		myParameters.command = fcSETREADMODE;
		myParameters.length = sizeof(myParameters);

		if (ReadBlack)
			myParameters.ReadBlack = -1;
		else
			myParameters.ReadBlack = 0;

		myParameters.DataXfrReadMode = DataXfrReadMode;
		myParameters.DataFormat = DataFormat;

		if (DoOffsetCorrection)
			myParameters.AutoOffsetCorrection = -1;
		else
			myParameters.AutoOffsetCorrection = 0;
			
		}
	else
		{
		// the following two parameters are now managed automatically
		if (gCurrentIntegrationTime[camNum - 1] <= 2000)
			ReadBlack = true;
		else
			ReadBlack = false;

		DoOffsetCorrection = true;


		// remember settings
		gReadBlack[camNum - 1] = ReadBlack;					// set if the host wishes to read the black pixels
		gDataXfrReadMode[camNum - 1] = DataXfrReadMode;		// the type of data transfers we will be using when taking picts
		gDataFormat[camNum - 1] = DataFormat;				// the desired camera data format

		myParameters.header = 'fc';
		myParameters.command = fcSETREADMODE;
		myParameters.length = sizeof(myParameters);

		if (ReadBlack)
			myParameters.ReadBlack = -1;
		else
			myParameters.ReadBlack = 0;

		myParameters.DataXfrReadMode = DataXfrReadMode;
		myParameters.DataFormat = DataFormat;

		if (DoOffsetCorrection)
			myParameters.AutoOffsetCorrection = -1;
		else
			myParameters.AutoOffsetCorrection = 0;

		}


	// send the constructed command
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	return 0;
}

// set some register defaults for the starfish camera
void fcUsb_setStarfishDefaultRegs(int camNum)
{
    UInt16		regValue;

	Starfish_Log("fcUsb_setStarfishDefaultRegs\n");

    if (gDoSimulation)
        return;

	// Turn off auto black level compensation in the micron image sensor
	regValue = fcUsb_cmd_getRegister(camNum, 0x5F);
	regValue = regValue | 0x8080;
	fcUsb_cmd_setRegister(camNum, 0x5F, regValue);
	regValue = fcUsb_cmd_getRegister(camNum, 0x62);
	regValue = regValue | 0x0001;
	fcUsb_cmd_setRegister(camNum, 0x62, regValue);


	// setup some minimum offset in the 4 bayer channels.  
	// Don't want to be too close to all zeros for pixel data
	fcUsb_cmd_setRegister(camNum, 0x60, 0x08);
	fcUsb_cmd_setRegister(camNum, 0x61, 0x08);
	fcUsb_cmd_setRegister(camNum, 0x63, 0x08);
	fcUsb_cmd_setRegister(camNum, 0x64, 0x08);

}

// here to set the filter type used for image processing
// The specified filter will be performed on any images
// transfered from the camera.  
// 
// 'theImageFilter' is one of fc_imageFilter
//
void fcUsb_cmd_setImageFilter(int camNum, int theImageFilter)
{
	gCameraImageFilter[camNum - 1] = theImageFilter;
}

// call to get the black level pedestal of the image sensor.  This is usefull if you wish
// to subtract out the pedestal from the image returned from the camera.
//
// the return value will be scalled according to the currently set fc_dataFormat. See the
// fcUsb_cmd_setReadMode routine for information on the pixel format
//
UInt16 fcUsb_cmd_getBlackPedestal(int camNum)
{
    UInt32					msgSize;
	fc_no_param		myParameters;
    UInt32					numBytesRead;
	fc_blackPedestal	myPedestalInfo;
	char					buffer[200];
    UInt16					retValue;
	int						maxBytes;	
	
	Starfish_Log("fcUsb_cmd_getBlackPedestal\n");

    if (gDoSimulation)
        return 0;
	
	myParameters.header = 'fc';
	myParameters.command = fcGETPEDESTAL;
	myParameters.length = sizeof(myParameters);
	myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

	msgSize = sizeof(myParameters);
	SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);
	
	// get the ACK to the command
	maxBytes = 512;
	numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);

	// copy the Rx buffer to my local storage
	memcpy( &myPedestalInfo, &gBuffer, sizeof(myPedestalInfo) );
	retValue = myPedestalInfo.dataValue;

	// put the current power in the log file
	sprintf(buffer, "     Got pedestal - 0x%04x\n", retValue);
	Starfish_Log( buffer );


	return retValue;
}

	// call this routine to set a camera property.  Various cameras support different properties
	// the following properties are defined:
	//
	//	1) fcPROP_NUMSAMPLES - enter with 'propertyValue' set to the number desired.  Valid
	//                         numbers are binary numbers starting with 4 (4, 8, 16, 32, 64, 128)
	//
	//  2) fcPROP_PATTERNENABLE - enter with 'propertyValue' set to '1' to enable the frame grabber
	//                         pattern generator.  '0' to get true sensor data
	//
	//  3) fcPROP_PIXCAPTURE - enter with 'propertyValue' set to '0' for normal CDS pixel sampling,
	//                         '1' for pixel level only and '2' for reset level only.
	//
	//	4) fcPROP_SHUTTERMODE - enter with 'propertyValue' set to '0' for AUTOMATIC shutter,
	//                         '1' for MANUAL shutter.
	//
	//  5) fcPROP_SHUTTERPRIORITY - enter with 'propertyValue' set to '0' for MECHANICAL shutter priority,
	//                         '1' for ELECTRONIC shutter priority
	//
	//  6) fcPROP_MOVESHUTTERFLAG = enter with 'propertyValue' set to '0' to CLOSE the mechanical shutter,
	//                         '1' to OPEN the mechanical shutter
	//
	//  7) fcPROP_COLNORM - enter with 'propertyValue' set to '0' for no column level normalization
	//                         '1' for column level normalization.
	//
	//  8) fcPROP_SERVOOPENPOS - enter with 'propertyValue' set to servo pulse period used for the OPEN position
	//
	//  9) fcPROP_SERVOCLOSEDPOS - enter with 'propertyValue' set to servo pulse period used for the CLOSED position
	//
	//
	void fcUsb_cmd_setCameraProperty(int camNum, int propertyType, int propertyValue)
	{
    UInt32						msgSize;
	fc_setProperty_param	myParameters;
    UInt32						numBytesRead;
	int							maxBytes;
	
	
	// print out the information 
//	printf("fcUsb_cmd_setCameraProperty:\n");
//	printf("     propertyType  - 0x%04x\n", propertyType);
//	printf("     propertyValue - 0x%04x\n", propertyValue);

	Starfish_Log("fcUsb_cmd_setCameraProperty\n");

    if (gDoSimulation)
        return;

	if (propertyType == fcPROP_COLNORM)
		{
		// this property is handled by the PC driver
		if (propertyValue == 0)
			gProWantColNormalization = false;
		else
			gProWantColNormalization = true;
		}
	else
		{
		// this one needs to be sent to the camera
		myParameters.header = 'fc';
		myParameters.command = fcSETPROPERTY;
		myParameters.length = sizeof(myParameters);
		myParameters.propertyType = propertyType;
        myParameters.propertyValue = (UInt16)propertyValue;
		myParameters.cksum = fcUsb_GetUsbCmdCksum(&myParameters.header);

		msgSize = sizeof(myParameters);
		SendUSB(camNum, (unsigned char*)&myParameters, (int)msgSize);

		// get the ACK to the command
		maxBytes = 512;
		numBytesRead = RcvUSB(camNum, (unsigned char*)&gBuffer, maxBytes);


		// if the property was to change the number of samples, we need to re-calibrate the camera.
//		if (propertyType == fcPROP_NUMSAMPLES)
//			{
//			fcImage_PRO_calibrateProCamera(camNum, gFrameBuffer, 2304, 2305);
//			}

		}

	}

