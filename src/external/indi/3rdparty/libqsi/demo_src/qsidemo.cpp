/*****************************************************************************************
NAME
 QSI API Simple Demo Application

DESCRIPTION
 Simple QSI API example

COPYRIGHT (C)
 QSI (Quantum Scientific Imaging) 2005-2008

REVISION HISTORY
 DRC 11.10.08 Original Version
 *****************************************************************************************/

#include "qsiapi.h"
#include <cstdio>
#include <unistd.h>
#include <iostream>
#include <cmath>
#include <stdlib.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#ifdef HAVE_TIFFIO_H
#include <tiffio.h>
void AdjustImage(unsigned short * buffer, int cols, int rows, unsigned char * out);
#endif /* HAVE_TIFFIO_H */
int WriteTIFF(unsigned short * buffer, int cols, int rows, char * filename);

#ifdef HAVE_FITSIO_H
#include <fitsio.h>
#endif /* HAVE_FITSIO_H */
int WriteFITS(unsigned short * buffer, int cols, int rows, char * filename);

void	usage() {
	std::cerr << "usage: qsidemo [ -ft ] [ -d directory ]" << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << " -f         write images as FITS (if supported)" << std::endl;
	std::cerr << " -t         write images as FITS (if supported)" << std::endl;
	std::cerr << " -d <dir>   directory where to store the images" << std::endl;
}

int main(int argc, char** argv)
{
	int x,y,z;
	std::string serial("");
	std::string desc("");
	std::string info = "";
	std::string modelNumber("");
	char filename[256];
	const char *dir = "/tmp";
	const char	*extension = "tiff";
	bool canSetTemp;
	bool hasFilters;
	short binX;
	short binY;
	long xsize;
	long ysize;
	long startX;
	long startY;
	int iNumFound;
	bool	tiffoutput = false;
	bool	fitsoutput = false;

	int	c;
	while (EOF != (c = getopt(argc, argv, "tfd:")))
		switch (c) {
		case 't':
#if HAVE_TIFFIO_H
			tiffoutput = true;
#else
			std::cerr << "no TIFF support" << std::endl;
			exit(EXIT_FAILURE);
#endif
			break;
		case 'f':
#if HAVE_FITSIO_H
			fitsoutput = true;
#else
			std::cerr << "no FITS support" << std::endl;
			exit(EXIT_FAILURE);
#endif
			break;
		case 'd':
			dir = optarg;
			break;
		}

	// for compatibility, of no option was present, and we have TIFF
	// support, then we use tiff output
#if HAVE_TIFFIO_H
	if ((!tiffoutput) && (!fitsoutput)) {
		tiffoutput = true;
	}
#endif

	if ((tiffoutput) && (fitsoutput)) {
		std::cerr << "you cannot request both TIFF and FITS." << std::endl;
		exit(EXIT_FAILURE);
	}
	if (fitsoutput) {
		extension = "fits";
	}

	QSICamera cam;

	cam.put_UseStructuredExceptions(true);
	try
	{
		cam.get_DriverInfo(info);
		std::cout << "qsiapi version: " << info << "\n";
		//Discover the connected cameras
		std::string camSerial[QSICamera::MAXCAMERAS];
		std::string camDesc[QSICamera::MAXCAMERAS];
		cam.get_AvailableCameras(camSerial, camDesc, iNumFound);

		if (iNumFound < 1)
		{
			std::cout << "No cameras found\n";
			exit(1);
		}

		for (int i = 0; i < iNumFound; i++)
		{
			std::cout << camSerial[i] << ":" << camDesc[i] << "\n";
		}

		cam.put_SelectCamera(camSerial[0]);

		cam.put_IsMainCamera(true);
		// Connect to the selected camera and retrieve camera parameters
		cam.put_Connected(true);
		std::cout << "Camera connected. \n";
		// Get Model Number
		cam.get_ModelNumber(modelNumber);
		std::cout << modelNumber << "\n";
		// Get Camera Description
		cam.get_Description(desc);
		std:: cout << desc << "\n";

		// Enable the beeper
		cam.put_SoundEnabled(true);
		// Enable the indicator LED
		cam.put_LEDEnabled(true);
		// Set the fan mode
		cam.put_FanMode(QSICamera::fanQuiet);
		// Query the current flush mode setting
		cam.put_PreExposureFlush(QSICamera::FlushNormal);

		// Query if the camera can control the CCD temp
		cam.get_CanSetCCDTemperature(&canSetTemp);
		if (canSetTemp)
		{
			// Set the CCD temp setpoint to 10.0C
			cam.put_SetCCDTemperature(10.0);
			// Enable the cooler
			cam.put_CoolerOn(true);
		}

		if (modelNumber.substr(0,1) == "6")
		{
			cam.put_ReadoutSpeed(QSICamera::FastReadout);
		}

		// Does the camera have a filer wheel?
		cam.get_HasFilterWheel(&hasFilters);
		if ( hasFilters)
		{
			// Set the filter wheel to position 1 (0 based position)
			cam.put_Position(0);
		} 

		if (modelNumber.substr(0,3) == "520" || modelNumber.substr(0,3) == "540")
		{
			cam.put_CameraGain(QSICamera::CameraGainHigh);
			cam.put_PreExposureFlush(QSICamera::FlushNormal);
		}
		//
		//////////////////////////////////////////////////////////////
		// Set image size
		//
		cam.put_BinX(1);
		cam.put_BinY(1);
		// Get the dimensions of the CCD
		cam.get_CameraXSize(&xsize);
		cam.get_CameraYSize(&ysize);
		// Set the exposure to a full frame
		cam.put_StartX(0);
		cam.put_StartY(0);
		cam.put_NumX(xsize);
		cam.put_NumY(ysize);
	
		// take 10 test images
		for (int i = 0; i < 10; i++)
		{
			bool imageReady = false;
			// Start an exposure, 0 milliseconds long (bias frame), with shutter open
			cam.StartExposure(0.000, true);
			// Poll for image completed
			cam.get_ImageReady(&imageReady);
			while(!imageReady)
			{
				usleep(100);
				cam.get_ImageReady(&imageReady);
			}
			// Get the image dimensions to allocate an image array
			cam.get_ImageArraySize(x, y, z);
			unsigned short* image = new unsigned short[x * y];
			// Retrieve the pending image from the camera
			cam.get_ImageArray(image);
			std::cout << "exposure #" << i;

			sprintf(filename, "%s/qsiimage%d.%s", dir, i, extension);
			if (tiffoutput) {
	#ifdef HAVE_TIFFIO_H
				WriteTIFF(image, x, y, filename);
	#endif
			} else if (fitsoutput) {
	#ifdef HAVE_FITSIO_H
				WriteFITS(image, x, y, filename);
	#endif
			}
			std::cout << "\n";	
			std::cout.flush();
			delete [] image;
		}
		cam.put_Connected(false);
		std::cout << "Camera disconnected.\nTest complete.\n";
		std::cout.flush();
		return 0;

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
}

int	WriteFITS(unsigned short *buffer, int cols, int rows, char *filename) {
#ifdef HAVE_FITSIO_H
	int	status = 0;
	fitsfile	*fits;
	unlink(filename);
	fits_create_file(&fits, filename, &status);
	if (status) {
		std::cerr << "cannot create file " << filename << std::endl;
		return -1;
	}
	long	naxes[2] = { cols, rows };
	fits_create_img(fits, SHORT_IMG, 2, naxes, &status);
	long	fpixel[2] = { 1, 1 };
	fits_write_pix(fits, TUSHORT, fpixel, cols * rows, buffer, &status);
	fits_close_file(fits, &status);
#else
	std::cerr << "FITS not supported" << std::endl;
	return -1;
#endif
}

#ifdef HAVE_TIFFIO_H
void AdjustImage(unsigned short * buffer, int cols, int rows, unsigned char * out);
#endif /* HAVE_TIFFIO_H */

int WriteTIFF(unsigned short * buffer, int cols, int rows, char * filename)
{
#ifdef HAVE_TIFFIO_H
	TIFF *image;
	unsigned char out[cols*rows];

	AdjustImage(buffer, cols, rows, out);

	// Open the TIFF file
	if((image = TIFFOpen(filename, "w")) == NULL)
	{
		printf("Could not open %s for writing\n", filename);
		exit(1);
	}
	
	// We need to set some values for basic tags before we can add any data
	TIFFSetField(image, TIFFTAG_IMAGEWIDTH, cols);
	TIFFSetField(image, TIFFTAG_IMAGELENGTH, rows);
	TIFFSetField(image, TIFFTAG_BITSPERSAMPLE, 8);
	TIFFSetField(image, TIFFTAG_SAMPLESPERPIXEL, 1);
	TIFFSetField(image, TIFFTAG_ROWSPERSTRIP, 1);
	
	TIFFSetField(image, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	TIFFSetField(image, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_MINISBLACK);
	TIFFSetField(image, TIFFTAG_FILLORDER, FILLORDER_LSB2MSB);
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
#else /* HAVE_TIFFIO_H */
	std::cerr << "TIFF not supported" << std::endl;
	return -1;
#endif /* HAVE_TIFFIO_H */
}

#ifdef HAVE_TIFFIO_H
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
			// Spread out pixel values for better veiwing
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
#endif /* HAVE_TIFFIO_H */
