//
// Copyright (C) 2014  Quantum Scientific Imaging, Inc.
//  (c) 2014 Prof Dr Andreas Mueller, Hochschule Rapperswil, for QSI
//
/*
 * qsiimage.cpp
 *
 * 
 */
#include <qsiapi.h>

#ifdef cfitsio
#include <fitsio.h>
#endif

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <cmath>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

static bool	verbose = false;

/**
 * \brief Get a timestamp
 */
static double	gettime() {
	struct timeval	tv;
	gettimeofday(&tv, NULL);
	double	t = tv.tv_sec;
	t += tv.tv_usec * 0.000001;
	return t;
}

/**
 * \brief Format a timestamp with millisecond precision for verbose mode
 */
static double	starttime;
std::string	timestamp() {
	char	buffer[16];
	snprintf(buffer, sizeof(buffer), "[%8.3f]  ", gettime() - starttime);
	return std::string(buffer);
}

/**
 * \brief convert the name of a filter into a number
 *
 * The filter position number we manipulate here is the number printed on 
 * the filterwheel, which starts at 1. Thus it is the API filter position + 1.
 */
static short	name2position(QSICamera& cam, const std::string& filtername) {
	// if no name is specified, then we use filter position 1
	if (filtername.size() == 0) {
		return 1;
	}

	// find the number of filter positions
	int	filtercount = 0;
	cam.get_FilterCount(filtercount);

	// find out how many filter positions are available
	unsigned short	filterposition = atoi(filtername.c_str());
	if ((filterposition < 0) || (filterposition > filtercount)) {
		throw std::runtime_error("invalid filter number");
	}

	// if the filterposition is > 0, it is a valid position
	if (filterposition > 0) {
		return filterposition;
	}

	// handle the case where the filter name cannot be converted.
	// in this case we try to match the filter name with the names
	// stored in the camera
	// get the list of available filter names
	std::string	*names = new std::string[filtercount];
	try {
		cam.get_Names(names);
	} catch (const std::exception& x) {
		delete[] names;
		throw x;
	}
 
	// display the list of available filter names

	if (verbose) {
		std::cout << timestamp();
		std::cout << "available filters:" << std::endl;
		for (int i = 0; i < filtercount; i++) {
			std::cout << (i + 1) << ": " << names[i];
			std::cout << std::endl;
		}
	}

	// find a match for the filter name
	for (int i = 0; i < filtercount; i++) {
		if (names[i] == filtername) {
			delete[] names;
			return i + 1;
		}
	}

	// if we have not found a reasonable filter number, we throw
	// an exception
	delete[] names;
	throw std::runtime_error("unknown filter name");
}

/**
 * \brief usage display of the program
 */
void	usage() {
	std::cerr << "usage: qsimage [ options ] file.fits" << std::endl;
	std::cerr << "expose an image and save as a fits file" << std::endl;
	std::cerr << "options:" << std::endl;
	std::cerr << " -i <cameraid>    select camera based on id" << std::endl;
	std::cerr << " -e <time>        exposure time in seconds" << std::endl;
	std::cerr << " -f <filter>      name or number of filter" << std::endl;
	std::cerr << " -s {open|close}  whether to open the shutter"
		<< std::endl;
	std::cerr << " -w <width>       width of the image" << std::endl;
	std::cerr << " -h <height>      height of the image" << std::endl;
	std::cerr << " -x <xoff>        x offset of the image rectangle"
		<< std::endl;
	std::cerr << " -y <yoff>        y offset of the image rectangle"
		<< std::endl;
	std::cerr << " -b <binning      use <binning> x <binning>" << std::endl;
	std::cerr << " -v               verbose mode" << std::endl;
	std::cerr << " -t <temp>        CCD temperature in degress Celsius" << std::endl;
	std::cerr << " -?               display this help and exit" << std::endl;
	std::cerr << " -g               use high gain" << std::endl;
	std::cerr << " -d               slow download" << std::endl;
}

/**
 * \brief Main function
 *
 * To simplify exception handle, call this function from main(), and handle
 * exceptions there. 
 */
int	image_main(int argc, char *argv[]) {

	// we will need a camera quite early, already when parsing the
	QSICamera	cam;

	int	c;
	int	width = -1;
	int	height = -1;
	int	x = -1;
	int	y = -1;
	int	binning = 1;
	std::string	filtername;
	std::string	cameraid;
	bool	shutter_open = true;
	double	temperature = -1000;
	double	exposuretime = 0;
	bool	highgain = false;
	bool	slowdownload = false;

	// parse command line options
	while (EOF != (c = getopt(argc, argv, "e:f:s:i:h:w:x:y:b:vt:?gd")))
		switch (c) {
		case 'i':
			cameraid = optarg;
			break;
		case 'e':
			exposuretime = atof(optarg);
			break;
		case 'f':
			filtername = optarg;
			break;
		case 's':
			shutter_open = false;
			break;
		case 'w':
			width = atoi(optarg);
			break;
		case 'h':
			height = atoi(optarg);
			break;
		case 'x':
			x = atoi(optarg);
			break;
		case 'y':
			y = atoi(optarg);
			break;
		case 'b':
			binning = atoi(optarg);
			break;
		case 'v':
			verbose = true;
			break;
		case 't':
			temperature = atof(optarg);
			break;
		case 'g':
			highgain = true;
			break;
		case 'd':
			slowdownload = true;
			break;
		case '?':
			usage();
			return EXIT_SUCCESS;
		}

	// sanitize the data
	if (x < 0) { x = 0; }
	if (y < 0) { y = 0; }

	// next argument should be the filename
	if (argc <= optind) {
		std::cerr << "file name argument missing" << std::endl;
		usage();
		return(EXIT_FAILURE);
	}
	const char	*filename = argv[optind];
	if (verbose) {
		std::cout << timestamp();
		std::cout << "image will be saved in " << filename << std::endl;
	}

	// make sure e get structured exceptions, so we don't have to check return codes
	cam.put_UseStructuredExceptions(true);
	
	// open the device
	if (cameraid.size() > 0) {
		cam.put_SelectCamera(cameraid);
	}
	cam.put_Connected(true);
	if (verbose) {
		std::cout << timestamp();
		std::cout << "camera connected" << std::endl;
	}

	// handle the filter position
	bool	hasfilterwheel;
	cam.get_HasFilterWheel(&hasfilterwheel);
	if (hasfilterwheel) {
		short	filterposition = name2position(cam, filtername);;
		cam.put_Position(filterposition - 1);
		if (verbose) {
			std::cout << timestamp();
			std::cout << "filter position: " << filterposition;
			std::cout << std::endl;
		}
	}

	// set binning
	short	maxbinx, maxbiny;
	cam.get_MaxBinX(&maxbinx);
	cam.get_MaxBinY(&maxbiny);
	if ((binning < 0) || (binning > maxbinx)) {
		throw std::runtime_error("invalid x binning");
	}
	cam.put_BinX(binning);
	if ((binning < 0) || (binning > maxbiny)) {
		throw std::runtime_error("invalid y binning");
	}
	cam.put_BinY(binning);
	if (verbose) {
		std::cout << timestamp();
		std::cout << "set " << binning << "x" << binning << " binning";
		std::cout << std::endl;
	}

	// set sub image rectangle
	long	xsize, ysize;
	cam.get_CameraXSize(&xsize);
	xsize /= binning;	// camera expects everything in binned pixels
	ysize /= binning;
	if (width < 0) { width = xsize; }
	if ((x < 0) || (x >= xsize)) {
		throw std::runtime_error("x origin invalid");
	}
	cam.get_CameraYSize(&ysize);
	if (height < 0) { height = ysize; }
	if ((y < 0) || (y >= ysize)) {
		throw std::runtime_error("y origin invalid");
	}
	if ((((x + width) > xsize) || ((y + height) > ysize))) {
		throw std::runtime_error("area too large");
	}
	cam.put_StartX(x);
	cam.put_StartY(y);
	cam.put_NumX(width);
	cam.put_NumY(height);
	if (verbose) {
		std::cout << timestamp();
		std::cout << "rectangle: " << width << " x " << height << " @ ";
		std::cout << x << "," << y << std::endl;
	}

	// check the exposure time
	double	minexp, maxexp;
	cam.get_MinExposureTime(&minexp);
	cam.get_MaxExposureTime(&maxexp);
	if ((exposuretime < minexp) || (exposuretime > maxexp)) {
		throw std::runtime_error("invalid exposure time");
	}

	// start the cooler, if available
	bool	cansettemperature;
	cam.get_CanSetCCDTemperature(&cansettemperature);
	if (temperature < -273.15) {
		cansettemperature = false;
	}
	if (cansettemperature) {
		if (verbose) {
			std::cout << timestamp();
			std::cout << "start CCD cooling" << std::endl;
		}
		cam.put_SetCCDTemperature(temperature);
		cam.put_CoolerOn(true);
		double	currenttemperature;
		double	delta;
		do {
			cam.get_CCDTemperature(&currenttemperature);
			delta = currenttemperature - temperature;
			sleep(1);
			if (verbose) {
				std::cout << timestamp();
				std::cout << "CCD temperature now: ";
				std::cout << currenttemperature << std::endl;
			}
		} while (fabs(delta) > 1);
		if (verbose) {
			std::cout << timestamp();
			std::cout << "CCD temperature reached: ";
			std::cout << currenttemperature << std::endl;
		}
	} else {
		if (verbose) {
			std::cout << timestamp();
			std::cout << "CCD cooling not available/requested" << std::endl;
		}
	}

	// set the camera gain
	cam.put_CameraGain((highgain)
				? QSICamera::CameraGainHigh
				: QSICamera::CameraGainLow);

	// set the readout speed
	cam.put_ReadoutSpeed((slowdownload)
				? QSICamera::HighImageQuality
				: QSICamera::FastReadout);

	// start exposure
	cam.StartExposure(exposuretime, shutter_open);
	if (verbose) {
		std::cout << timestamp();
		std::cout << "started " << exposuretime << " seconds exposure";
		std::cout << std::endl;
	}

	// wait for the exposure to complete
	bool	imageReady = false;
	cam.get_ImageReady(&imageReady);
	while (!imageReady) {
		usleep(5000);
		cam.get_ImageReady(&imageReady);
	}
	if (verbose) {
		std::cout << timestamp();
		std::cout << "exposure complete" << std::endl;
	}

	// retreive the image array
	int	w, h, pixelsize;
	cam.get_ImageArraySize(w, h, pixelsize);
	unsigned short	*imagedata = new unsigned short [w * h];
	long	pixels = w * h;
	if (verbose) {
		std::cout << timestamp();
		std::cout << "image array size: " << w << " x " << h;
		std::cout << ", " << pixels << " pixels";
		std::cout << std::endl;
	}

	double	t0 = gettime();
	cam.get_ImageArray(imagedata);
	double	t1 = gettime();
	if (verbose) {
		std::cout << timestamp();
		std::cout << "image download complete: " << (t1 - t0);
		std::cout << " seconds" << std::endl;
	}
#ifdef cfitsio
	// write the data to the FITS file
	int	status = 0;
	try {
		unlink(filename); // delete the file if present, as the fits
				// library will never do that
		fitsfile	*fits = NULL;
		fits_create_file(&fits, filename, &status);
		if (status) {
			throw std::runtime_error("cannot create FITS file");
		}
		long	naxes[2] = { w, h };
		fits_create_img(fits, USHORT_IMG, 2, naxes, &status);
		if (status) {
			throw std::runtime_error("cannot create FITS image");
		}
		long	fpixel[2] = { 1, 1 };
		fits_write_pix(fits, TUSHORT, fpixel, pixels, imagedata,
			&status);
		if (status) {
			throw std::runtime_error("cannot write FITS pixel data");
		}
		fits_close_file(fits, &status);
		if (status) {
			throw std::runtime_error("cannot close FITS file");
		}
	} catch (std::exception& x) {
		char	error[40];
		fits_get_errstatus(status, error);
		std::cerr << "failed to write FITS the image: " << error;
		std::cerr << std::endl;
	}
#endif
	
	// turn off the cooler
	if (cansettemperature) {
		cam.put_CoolerOn(false);
	}

	// disconnect
	cam.put_Connected(false);

	// cleanup
	delete[] imagedata;
	
	// if we get here, everything was successful
	return EXIT_SUCCESS;
}

int	main(int argc, char *argv[]) {
	starttime = gettime();
	try {
		return image_main(argc, argv);
	} catch (const std::exception& x) {
		std::cerr << "Exception: " << x.what() << std::endl;
	}
}
