/*****************************************************************************************
NAME
 QSI Linux API Demo Application

DESCRIPTION
 Source code to demonstrate the use of the API

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2011

REVISION HISTORY
 DRC 03.11.07 Original Version
 DRC 01.01.08 Add 520 features
 DRC 07.17.08 Add QSIxxx features
 DRC 05.07.11 Changes for 600 series
 AM  12.22.13 Contributions from Prof. Dr. Andreas Muller: argv/getopt options
 *****************************************************************************************/

#include "qsiapi.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <stdlib.h>
//#include "dumapp.h"
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */


void	usage(const char *progname) {
	std::cout << "usage:" << std::endl;
	std::cout << progname << " [ -tbmhzs? ]" << std::endl;
	std::cout << "options:"<< std::endl;
	std::cout << "  -t   enable temperatur regulation test" << std::endl;
	std::cout << "  -b   binning test" << std::endl;
	std::cout << "  -m   manual mechanical shutter test" << std::endl;
	std::cout << "  -h   host time exposure test" << std::endl;
	std::cout << "  -z   show auto-zero info" << std::endl;
	std::cout << "  -s   enable shutter status output" << std::endl;
	std::cout << "  -?   display this help message" << std::endl;
}

#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
int WriteTIFF(unsigned short * buffer, int cols, int rows, char * filename);
void AdjustImage(unsigned short * buffer, int cols, int rows, unsigned char * out);
#endif

#define NUMTESTIMAGES 10

	QSICamera cam;

int main(int argc, char** argv)
{
	int	c;
	// for temperature regulation testing
	bool	WAITFORTEMP = false;
	// for binning testing
	bool	BINTEST = false;
	// for manual mechanical shutter testing
	bool	MANUALSHUTTER = false;
	// for host timed exposure testing
	bool	TIMEDEXPOSURE = false;
	// for showing auto-zero info
	bool	SHOWAUTOZERO = false;
	// for enabling Shutter status output
	bool	SHUTTERSTATUS = false;
	while (EOF != (c = getopt(argc, argv, "tbmhzs?")))
		switch (c) {
		case 't':
			WAITFORTEMP = true;
			break;
		case 'b':
			BINTEST = true;
			break;
		case 'm':
			MANUALSHUTTER = true;
			break;
		case 'h':
			TIMEDEXPOSURE = true;
			break;
		case 'z':
			SHOWAUTOZERO = true;
			break;
		case 's':
			SHUTTERSTATUS = true;
			break;
		case '?':
			usage(argv[0]);
			break;
		}


	std::string msg("");
	int x,y,z;
	int filters = 0;
	int result;
	std::string serial("");
	std::string * names = NULL;
	std::string desc("");
	std::string info("");
	std::string modelNumber("");
	char filename[256] = "";
	long * offsets = NULL;
	bool LEDEnabled;
	bool soundEnabled;
	bool canAbort;
	bool canAsymBin;
	bool canGetPower;
	bool canGuide;
	bool canSetTemp;
	bool canStop;
	bool connected;
	bool coolerOn;
	bool hasFilters;
	bool hasShutter;
	bool isMain;
	bool isGuiding;
	bool p2bin;
	QSICamera::FanMode fan;
	QSICamera::PreExposureFlush flush;
	QSICamera::ShutterPriority shutterPriority;
	QSICamera::CameraState state;
	short binX;
	short binY;
	short pos;
	short maxBinX;
	short maxBinY;
	unsigned short mean;
	long xsize;
	long ysize;
	long maxX;
	long maxY;
	long startX;
	long startY;
	long adu;
	double expTime;
	double power;
	double ccdTemp;
	double sinkTemp;
	double coolerPower;
	double eADU;
	double fwc;
	double pixelX;
	double pixelY;

	// double expTimes[] = {0.0025, 0.0125, 0.0625, 0.3125, 1.5625, 7.8125, 39.0625, 195.3125};

	//QSICamera cam2;
	//
	// First show a couple of illegal operations:
	// you cannot assign one camera object to another.
	// Only one camera object is allowed per camera connection
	//
	//try
	//{
	//	cam2 = cam;
	//}
	//catch (std::runtime_error& err)
	//{
		//std::cout << err.what() << "(OK) \n";
	//}

	//try
	//{
	//	cam = QSICamera();
	//}
	//catch (std::runtime_error& err)
	//{
	//	//std::cout << err.what() << " (OK) \n";
	//}

	std::cout << "QSI API test start...\n";
	
	//
	// IMPORTANT:
	// This is just a collection of examples of the various methods for the camera object.
	// This does not represent an actual application
	//
	// The following sequence shows the use of the QSICamera class
	// using try/catch blocks to handle errors.
	// This is enabled by setting UseStructuredExceptions to true
	//
	// All QSICamera methods can be used in either fashion
	// It is up to the developer to choose which method is preferred
	//

	cam.put_UseStructuredExceptions(true);
	try
	{
		cam.get_DriverInfo(info);
		std::cout << "qsiapitest version: " << info << "\n";
		//Discover the connected cameras
		int iNumFound;
		std::string camSerial[QSICamera::MAXCAMERAS];
		std::string camDesc[QSICamera::MAXCAMERAS];

		cam.get_AvailableCameras(camSerial, camDesc, iNumFound);

		for (int i = 0; i < iNumFound; i++)
		{
			std::cout << camSerial[i] << ":" << camDesc[i] << "\n";
		}
		
		// Get the serial number of the selected camera in the setup dialog box
		cam.get_SelectCamera(serial);
		if (serial.length() !=0 )
				std::cout << "Saved selected camera serial number is " << serial << "\n";
		// Put the camera serial number in the dialog box
		// If there is more than one camera connected
		// The API will attempt to open the camera specified
		// In the setup dialog, which can be set with the call below
		// If there is only one camera connected, and the serial number has not
		// been selected using the QSISelectedDevice calls (below), the API
		// will automatically connect to the camera regardless of the dialog setting.
		cam.put_SelectCamera(serial);


#ifdef QSIMETHODS
		// In the case of a multi-threaded application, the setup dialog box strategy
		// to select an appropriate camera will not work, as there is only one
		// global selection from the setup dialog.
		// The QSISelectedCamera property overrides the dialog box setting and is thread specific.
		// If this property is set, the setup dialog box setting is ignore, and the API
		// will only open the camera with the serial number specified.  The single camera
		// default behavior will not be used in this case.
		cam.put_QSISelectedDevice(serial);
		// To retrieve the thread specific camera serial number selection:
		cam.get_QSISelectedDevice(serial);
		// You can null the QSISelectedDevice property to use the current connected camera
		// (if only one camera connected), or fall back to the dialog setting if more than
		// one camera is connected.
		cam.put_QSISelectedDevice(std::string(""));
#endif

		// Get the current selected camera role
		// Either main or guider
		cam.get_IsMainCamera(&isMain);
		// Set the camera role to main camera (not guider)
		cam.put_IsMainCamera(true);
		// Connect to the selected camera and retrieve camera parameters
		std::cout << "Try to connect to camera...\n";
		cam.put_Connected(true);
		std::cout << "Camera connected. \n";
		cam.get_SerialNumber(serial);
		std::cout << "Serial Number: " + serial + "\n";
		// Get Model Number
		cam.get_ModelNumber(modelNumber);
		std::cout << modelNumber << "\n";
		// Get the camera state
		// It should be idle at this point
		cam.get_CameraState(&state);
		// See if the beeper is enabled
		cam.get_SoundEnabled(soundEnabled);
		// Enable the beeper
		cam.put_SoundEnabled(true);
		// See if the status indicator LED is enabled
		cam.get_LEDEnabled(LEDEnabled);
		// Enable the indicator LED
		cam.put_LEDEnabled(true);
		// Get the current fan mode
		// Off, quiet, or full speed
		cam.get_FanMode(fan);
		// Set the fan mode to full speed
		cam.put_FanMode(QSICamera::fanFull);
		// Query the current flush mode setting
		cam.get_PreExposureFlush(&flush);
		// Set normal flush before the exposure
		cam.put_PreExposureFlush(QSICamera::FlushModest);
		// Get Camera Description
		cam.get_Description(desc);
		std:: cout << desc << "\n";
		if (modelNumber.substr(0,1) == "6")
		{
			std::cout << "600 Series camera detected.  Trying to set fast exposure speed...\n";
			//Select one by un-commenting the appropriate line
			cam.put_ReadoutSpeed(QSICamera::FastReadout);
			//cam.put_ReadoutSpeed(QSICamera::HighImageQuality);
			//
			QSICamera::ReadoutSpeed speed;
			cam.get_ReadoutSpeed(speed);
			if (speed == QSICamera::HighImageQuality)
				std::cout << "Readout Speed set to HighImageQuality. \n";
			else
				std::cout << "Readout Speed set to FastReadout. \n";
		}
		// Does the camera have a shutter?
		cam.get_HasShutter(&hasShutter);
		
		if (hasShutter)
		{
			std::cout << "Camera has a Shutter \n";
			// Query the type of shutter (Electronic or Mechanical)
			cam.get_ShutterPriority(&shutterPriority);
			cam.put_UseStructuredExceptions(false);
			// Produce an error with a 520i, 520i is Electronic shutter only
			result = cam.put_ShutterPriority(QSICamera::ShutterPriorityMechanical);
			// Produce an error with a 504/516/532, these models do not have
			// electronic shutters
			result = cam.put_ShutterPriority(QSICamera::ShutterPriorityElectronic);
			cam.put_UseStructuredExceptions(true);
			//
			// Test manual shutter operations
			// Requires firmware 3.5.11 or beyond
			//
			cam.get_HasFilterWheel(&hasFilters);
	
			if ( hasFilters)
			{
				// How many filter positions?
				cam.get_FilterCount(filters);
				cam.put_Position(4);
			}
			cam.put_ManualShutterMode(true);
			cam.put_ManualShutterOpen(true);
			cam.put_ManualShutterOpen(false);
			cam.put_ManualShutterMode(false);
		}
		else
		{
			std::cout << "No Shutter...\n";
		}
		// Set the fan mode to quiet.
		cam.put_FanMode(QSICamera::fanQuiet);
		// Get the CCD size
		cam.get_CameraXSize(&maxX);
		cam.get_CameraYSize(&maxY);
		// Query the current image settings
		// Image start location
		cam.get_StartX(&xsize);
		cam.get_StartY(&ysize);
		// Image size
		cam.get_NumX(&xsize);
		cam.get_NumY(&ysize);
		// Bin size
		cam.get_BinX(&binX);
		cam.get_BinY(&binY);
		// Set new image parameters
		cam.put_StartX(0);
		cam.put_StartY(0);
		cam.put_NumX(xsize);
		cam.put_NumY(ysize);
		cam.put_BinX(1);
		cam.put_BinY(1);

		// Demonstrate how to abort an exposure
		cam.get_CanAbortExposure(&canAbort);
		if (canAbort)
			cam.AbortExposure();
		// Query if camera can bin asymmetrically (binx != biny)
		cam.get_CanAsymmetricBin(&canAsymBin); 
		// Query if camera can return cooler power
		cam.get_CanGetCoolerPower(&canGetPower);
		if (canGetPower)
			cam.get_CoolerPower(&power);
		// Query if the camera can pulse guide, and demo guiding
		cam.get_CanPulseGuide(&canGuide);
		if (canGuide)
		{
			std::cout << "Can Pulse Guide...\n";
			cam.get_IsPulseGuiding(&isGuiding);
			if (!isGuiding)
				cam.PulseGuide(QSICamera::guideNorth, 100);
		}
		// Query if the camera can control the CCD temp
		cam.get_CanSetCCDTemperature(&canSetTemp);
		if (canSetTemp)
		{
			std::cout << "Can control temperature...\n";
			// Query the current CCD temp set point
			cam.get_SetCCDTemperature(&ccdTemp);
			// Set the CCD temp setpoint to 10.7C
			cam.put_SetCCDTemperature(10.7);
			// Query the heatsink temp
			// Cameras that don't support this return 0
			cam.get_HeatSinkTemperature(&sinkTemp);
		}
		// Demonstrate how to stop an exposure
		// StopExposure will return an image
		// AbortExposure will not return an image
		cam.get_CanStopExposure(&canStop);
		if (canStop)
			cam.StopExposure();
		// Query the CCD temperature
		cam.get_CCDTemperature(&ccdTemp);
		// Query the connection state
		cam.get_Connected(&connected);
		// Is the cooler enabled?
		cam.get_CoolerOn(&coolerOn);
		// Enable the cooler
		cam.put_CoolerOn(true);
		std::cout << "Cooler On...\n";
		// Query various camera parameters
		cam.get_Description(desc);
		cam.get_ElectronsPerADU(&eADU);
		cam.get_FullWellCapacity(&fwc);
		cam.get_MaxADU(&adu);
		cam.get_MaxBinX(&maxBinX);
		cam.get_MaxBinY(&maxBinY);
		cam.get_ModelNumber(info);
		cam.get_Name(info);
		cam.get_PixelSizeX(&pixelX);
		cam.get_PixelSizeY(&pixelY);
		cam.get_PowerOfTwoBinning(&p2bin);
		cam.get_SerialNumber(info);
		cam.get_MinExposureTime( &expTime );
		std::cout << "Min. Exposure Time: " << expTime << "\n";
		cam.get_MaxExposureTime( &expTime );
		// Does the camera have a filer wheel?
		cam.get_HasFilterWheel(&hasFilters);

		if ( hasFilters)
		{
			std::cout << "Camera has a FilterWheel...\n";
			// First test the "default" wheel
			// How many filter positions?
			cam.get_FilterCount(filters);
			// Query the current filter wheel position
			cam.get_Position(&pos);
			std::cout << "Initial position:  " << pos << "\n";
			for (int i = 0; i < 5; i++)
			{
				// Set the filter wheel to position 1 (0 based position)
				cam.put_Position(i);
				cam.get_Position(&pos);
				std::cout << "Moved to position: " << pos << "\n";
			}
			// Allocate arrays for filter names and offsets
			names = new std::string[filters];
			offsets = new long[filters];
			// Get the names and filter offsets
			// values are stored in the .QSIConfig file in
			// the user's home directory
			cam.get_Names(names);
			cam.get_FocusOffset(offsets);
			// change the name and offset of filter
			names[0] = "Red";
			names[1] = "Green";
			names[2] = "Blue";
			names[3] = "OII";
			names[4] = "UV";
			offsets[4] = 100L;
			// Save the new names and offsets
			cam.put_Names(names);
			cam.put_FocusOffset(offsets);
			// Display the filter names
			for (int i = 0; i < filters; i++)
			{
				std::cout << names[i];
				std::cout << "\n";
			}
			// Now test named wheels
			cam.NewFilterWheel("WheelOne");
			cam.NewFilterWheel("WheelTwo");
			cam.NewFilterWheel("WheelThree");
			cam.DeleteFilterWheel("WheelTwo");
			std::string WheelName;
			cam.get_SelectedFilterWheel(& WheelName);
			cam.put_SelectedFilterWheel("WheelOne");
			cam.get_SelectedFilterWheel(& WheelName);
			std::vector<std::string> Wheels;
			cam.get_FilterWheelNames( & Wheels);
			bool HasTrim = false;
			cam.get_HasFilterWheelTrim(& HasTrim);
			if (HasTrim)
			{
				std::cout << "Camera has filter wheel trim...\n";
				std::vector<short> Trims;
				cam.get_FilterPositionTrim( & Trims);
				Trims[0] = 100;
				cam.put_FilterPositionTrim( Trims );				
			}
			// How many filter positions?
			cam.get_FilterCount(filters);
			// Query the current filter wheel position
			cam.get_Position(&pos);
			// Set the filter wheel to position 1 (0 based position)
			cam.put_Position(4);
			// Allocate arrays for filter names and offsets
			delete[] names;
			delete[] offsets;
			names = new std::string[filters];
			offsets = new long[filters];
			// Get the names and filter offsets
			// values are stored in the .QSIConfig file in
			// the user's home directory
			cam.get_Names(names);
			cam.get_FocusOffset(offsets);
			// change the name and offset of filter 4
			names[4] = "New Number 4";
			offsets[4] = 100L;
			// Save the new names and offsets
			cam.put_Names(names);
			cam.put_FocusOffset(offsets);
			// Display the filter names
			for (int i = 0; i < filters; i++)
			{
				std::cout << names[i];
				std::cout << "\n";
			}
			cam.DeleteFilterWheel("WheelOne");
			cam.DeleteFilterWheel("WheelThree");
		}
		else
		{
			std::cout << "No Filter Wheel...\n";
		}
		//
		// Demonstrate Pixel masking
		bool enabled;
		std::vector<Pixel> map;
		// get old values to save
		cam.get_MaskPixels(&enabled);
		cam.get_PixelMask(&map);
		// create new settings
		std::vector<Pixel> newmap;
		newmap.push_back(Pixel(234,678));
		newmap.push_back(Pixel(233,244));
		cam.put_PixelMask(newmap);
		cam.put_MaskPixels(true);
		// Read back the new settings
		bool isenabled;
		std::vector<Pixel> checkmap;
		cam.get_MaskPixels(&isenabled);
		cam.get_PixelMask(&checkmap);
		std::cout << "Pixel mapping :" << isenabled << "\n";
		std::cout << "Pixels: \n";
		std::vector<Pixel>::iterator vi;
		for (vi = checkmap.begin(); vi != checkmap.end(); vi++)
		{
			// std::cout << "(" << (*vi).x << "," << (*vi).y << ")\n";
		}
		// now reset to the original settings
		cam.put_PixelMask(map);
		cam.put_MaskPixels(enabled);
	}
	catch (std::runtime_error &err)
	{
		std::string text = err.what();
		std::cout << text << "\n";
		std::string last("");
		cam.get_LastError(last);
		std::cout << last << "\n";
		std::cout << "exiting with errors\n";
		exit(1);
	}

	delete[] names;
	delete[] offsets;

	if (BINTEST) {
		// Test Asymmetric 1x2 binning
		try
		{
			bool imageReady = false;
			cam.put_BinX(4);
			cam.put_BinY(4);
			// Image size is specified in binned pixels,
			// First get the imager size (X axis)
			cam.get_CameraXSize(&xsize);
			// Reduce it by the binning factor
			cam.put_NumX(xsize/4);
			// First get the imager size (Y axis)
			cam.get_CameraYSize(&ysize);
			// Reduce it by the binning factor
			cam.put_NumY(ysize/4);
			std::cout << "Start 4x4 binning test\n";
			result = cam.StartExposure(0.500, true);
			// Poll for image completed
			result = cam.get_ImageReady(&imageReady);
			while(!imageReady)
			{
				usleep(5000);
				result = cam.get_ImageReady(&imageReady);
			}
			// Get the image dimensions to allocate an image array
			result = cam.get_ImageArraySize(x, y, z);
			unsigned short* image = new unsigned short[x * y];
			// Retrieve the pending image from the camera
			result = cam.get_ImageArray(image);
			delete [] image;
			std::cout << "4x4 binning test complete\n";
		}
		catch (std::runtime_error& err)
		{
			std::cout << "test asym bin block failed. \n";
		}
	}
//
// The following code shows use of the QSICamera class
// not using try/catch blocks, but testing result codes instead
// by setting UseStructuredException = false
//
// All QSICamera methods can be used in either fashion
// It is up to the developer to choose which method is preferred
//
	cam.put_UseStructuredExceptions(false);
	if (SHUTTERSTATUS) {
		result = cam.put_EnableShutterStatusOutput(true);
	} else {
		result = cam.put_EnableShutterStatusOutput(false);
	}
	// retrieve the test of the last error
	std::string last("");
	result = cam.get_LastError(last);
	if (result != 0)
		std::cout << last;
// End of example

	expTime = 0.030;
	binX = 1;
	binY = 1;
	cam.get_CameraXSize(&xsize);
	cam.get_CameraYSize(&ysize);

// Test 520 features
	if (modelNumber.substr(0,3) == "520")
	{
		std::cout << "Model 520 Camera...\n";
		cam.put_CameraGain(QSICamera::CameraGainHigh);
		cam.put_AntiBlooming(QSICamera::AntiBloomNormal);
		cam.put_PreExposureFlush(QSICamera::FlushNone);
		cam.put_ShutterPriority(QSICamera::ShutterPriorityElectronic);
		expTime = 0.0001;
		if (TIMEDEXPOSURE) {
			result = cam.put_HostTimedExposure(true);
			binX = 3;
			binY = 3;
			xsize = 20;
			ysize = 20;
		} else {
			result = cam.put_HostTimedExposure(false);
		}
		if (result != 0)
			std::cout << result << "\n";		
	}
//
//////////////////////////////////////////////////////////////
// Set image size
//

	cam.put_BinX(binX);
	cam.put_BinY(binY);
	cam.put_StartX(0);
	cam.put_StartY(0);
	cam.put_NumX(xsize);
	cam.put_NumY(ysize);
	cam.put_CoolerOn(false);
	cam.get_CCDTemperature(&ccdTemp);

	if (WAITFORTEMP) {
		while (ccdTemp < 20.0)
		{
			sleep(1000);
			cam.get_CCDTemperature(&ccdTemp);
			cam.get_CoolerPower(&coolerPower);
			std::cout << "temp: " << ccdTemp << " power: " << coolerPower << "\n";
		}
	}

	if (MANUALSHUTTER) {
		cam.put_ManualShutterMode(true);
		cam.put_EnableShutterStatusOutput(true);
	}

	cam.put_SetCCDTemperature(0.0);
	cam.put_CoolerOn(true);
	std::cout << "Camera Cooler On...\n";
	//****************************************************************************************************
	//
	// take NUMTESTIMAGES test images
	//
	//****************************************************************************************************
	for (int i = 0; i < NUMTESTIMAGES; i++) 
	{

		if (WAITFORTEMP) {
			if (i%100 == 0)
			{
				cam.get_CCDTemperature(&ccdTemp);
				cam.get_CoolerPower(&coolerPower);
				std::cout << "\ntemp: " << ccdTemp << " power: " << coolerPower << "\n";
			}
		}
		cam.get_CCDTemperature(&ccdTemp);


		if (BINTEST) {
			if (i%3 == 0)
			{
				if (++binX > maxBinX) binX = 1;
			}
			if (++binY > maxBinY) binY = 1;
			
			cam.put_BinX(binX);
			cam.put_BinY(binY);
			
			startX = (rand() % 4);
			startY = (rand() % 4);
			cam.put_StartX(startX);
			cam.put_StartY(startY);

			xsize = (rand() % (maxX / binX)) + 1;
			if ((startX + xsize) * binX > maxX) xsize = maxX / binX - startX;
			ysize = (rand() % (maxY / binY)) + 1;
			if ((startY + ysize) * binY > maxY) ysize = maxY / binY - startY;

			cam.put_NumX(xsize);
			cam.put_NumY(ysize);

			std::cout << "Start: " << startX << "," << startY << " ";
			std::cout << "Size: " << xsize << "," << ysize << " ";
			std::cout << "Bin : " << binX << "," << binY << "\n";
			// end of random binning test
		}

		if (MANUALSHUTTER) {
			cam.put_ManualShutterOpen(true);
		}
		bool imageReady = false;
		// Start an exposure, 30 milliseconds long, with shutter open
		std::cout << "<===================================================================================>\n";
		std::cout << "Starting exposure  with " << expTime << "s exposure time, Exposure #"<< i << " ...\n";
		result = cam.StartExposure(expTime, true);
		if (result != 0) 
		{
			std::cout << "StartExposure error \n";
			std::string last("");
			cam.get_LastError(last);
			std::cout << last << "\n";
			break;
		}
		std::cout << "Start Exposure Complete.  \nWaiting for Image Ready...\n";
		cam.get_CCDTemperature(&ccdTemp);
		std::cout << "CCD Temp: " << ccdTemp << "\n";
		// Poll for image completed
		result = cam.get_ImageReady(&imageReady);
		while(!imageReady)
		{
			usleep(1000);
			result = cam.get_ImageReady(&imageReady);
			if (result != 0) 
			{
				std::cout << "get_ImageReady error \n";
				std::string last("");
				cam.get_LastError(last);
				std::cout << last << "\n";
				break;
			}
		}
		if (MANUALSHUTTER) {
			cam.put_ManualShutterOpen(false);
		}

		std::cout << "Image Ready...\n";
		// Get the image dimensions to allocate an image array
		result = cam.get_ImageArraySize(x, y, z);
		if (result != 0) 
		{
			std::cout << "get_ImageArraySize error \n";
			std::string last("");
			cam.get_LastError(last);
			std::cout << last << "\n";
			break;
		}
		std::cout << "Image Size " << x << " x " << y << " " << x * y << " Pixels...\n";
		unsigned short* image = new unsigned short[x * y];
		// Retrieve the pending image from the camera
		result = cam.get_ImageArray(image);
		if (result != 0) 
		{
			std::cout << "get_ImageArray error \n";
			std::string last("");
			cam.get_LastError(last);
			std::cout << last << "\n";
			break;
		}

		std::cout << "Image transfer complete...\n";
		if (SHOWAUTOZERO) {
			result = cam.get_LastOverscanMean(&mean);
			if (mean < 400)
			{
				std::cout << "Overscan mean below 400.  Mean:" << mean << "\n";
			}

			std::cout << "OS Mean " << mean << "\n";
			std::cout << "Image data ";
			for (int k = 0; k < 16; k++)
			{
			  std::cout << image[k] << " ";
			}
			std::cout << "\n";
		}

		result = cam.get_HeatSinkTemperature(&sinkTemp);
		std::cout << "Heatsink temp: " << sinkTemp << "\n";

		result = cam.get_LastExposureStartTime(info);
		if (result != 0) {/*handle error*/}

		std::cout  << info << " ";
		result = cam.get_LastError(msg);
		if (result != 0) {/*handle error*/}

#ifdef HAVE_TIFFIO_H
		sprintf(filename, "/tmp/qsiimage%d.tif", i);
		WriteTIFF(image, x, y, filename);
#endif

		delete [] image;
		std::cout << "exposure #" << i << ",";
		std::cout << "\n";
		std::cout.flush();
	}
	
	std::cout << "Image loop complete.\n";
	std::cout.flush();
	cam.put_CoolerOn(false);
	std::cout << "Cooler Off complete.\n";
	std::cout.flush();
	result = cam.put_Connected(false);
	std::cout << "Camera disconnected.\nTest complete.\n";
	std::cout.flush();
	return 0;
}

#ifdef HAVE_TIFFIO_H
int WriteTIFF(unsigned short * buffer, int cols, int rows, char * filename)
{
	TIFF *image;
	unsigned char *out;
	out = new unsigned char[cols*rows];

	AdjustImage(buffer, cols, rows, out);

	// Open the TIFF file
	if((image = TIFFOpen(filename, "w")) == NULL)
	{
		printf("Could not open output.tif for writing\n");
		exit(42);
	}
	
	// We need to set some values for basic tags before we can add any data
	TIFFSetField(image, TIFFTAG_IMAGEWIDTH, cols);
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, rows);
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);
	
	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_MSB2LSB);
	TIFFSetField(image, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	
	TIFFSetField(image, TIFFTAG_XRESOLUTION, 150.0);
	TIFFSetField(image, TIFFTAG_YRESOLUTION, 150.0);
	TIFFSetField(image, TIFFTAG_RESOLUTIONUNIT, RESUNIT_INCH);
	
	// Write the information to the file
	for (int y = 0; y < rows; y++)
	{
		TIFFWriteScanline(image, &out[cols*y], y);
	}
	
	// Close the file
	TIFFClose(image);
	delete[] (out);
	return 0;
}

void AdjustImage(unsigned short * buffer, int x, int y, unsigned char * out)
{
	//
	// adjust the image to better display and
	// covert to a byte array
	//
	// Compute the average pixel value and the standard deviation
	double avg = 0;
	double total = 0;
	double deltaSquared = 0;
	double std = 0;
	
	for (int j = 0; j < y; j++)
		for (int i = 0; i < x; i++)
			total += (double)buffer[((j * x) + i)];

	avg = total / (x * y);

	for (int j = 0; j < y; j++)
		for (int i = 0; i < x; i++)
			deltaSquared += pow((avg - buffer[((j * x) + i)]), 2);

	std = sqrt(deltaSquared / ((x * y) - 1));
	
	// re-scale scale pixels to three standard deviations for display
	double minVal = avg - std*3;
	if (minVal < 0) minVal = 0;
	double maxVal = avg + std*3;
	if (maxVal > 65535) maxVal = 65535;
	double range = maxVal - minVal;
	if (range == 0)
		range = 1;
	double spread = 65535 / range;
	//
	// Copy image to bitmap for display and scale during the copy
	//
	int pix;
	double pl;
	unsigned char level;
	
	for (int j = 0; j < y; j++)
	{
		for (int i = 0; i < x; i++)
		{
			pix = ((j * x) + i);
			pl = (double)buffer[pix];
			// Spread out pixel values for better viewing
			pl = (pl - minVal) * spread;
			// Scale pixel value
			pl = (pl*255)/65535;
			if (pl > 255) pl = 255;
			//
			level = (unsigned char)pl;
			out[pix] = level;
		}
	}
	return;
}
#endif

