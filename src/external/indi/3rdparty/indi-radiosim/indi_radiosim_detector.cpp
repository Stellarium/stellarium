/*
    indi_radiosim_detector - a simulated radio telescope driver for INDI
    Copyright (C) 2017  Ilia Platone

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "indi_radiosim_detector.h"
#include "hires.h"
#include <math.h>
#include <unistd.h>
#include <indilogger.h>
#include <memory>

#define MAX_TRIES 20
#define MAX_DEVICES 4
#define SUBFRAME_SIZE 256
#define SAMPLE_RATE 1000000
#define FREQUENCY 20000000000
#define SIDEREAL_DAY 86164.090530833
#define RAD_AS ((360 * 60 * 60) / M_PI)
#define AIRY 1.21966
#define AIRY_AS (AIRY * RAD_AS)
#define LIGHT_SPEED 299792458.0
#define RESOLUTION0 (AIRY_AS * LIGHT_SPEED / FREQUENCY)
#define DISH_SIZE_M 5.0
#define MAX_DISH_SIZE_M 32.0
#define RESOLUTION_AS(size) (RESOLUTION0 / size)
#define RESOLUTION_MAX (RESOLUTION0 / MAX_DISH_SIZE_M)
#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1200
#define FOV_DEG (360.0 * IMAGE_WIDTH / SIDEREAL_DAY)
#define RESOLUTION_PX(size) (RESOLUTION_AS(size) * IMAGE_WIDTH / (FOV_DEG*60*60))
#define RESOLUTION_PY(size) (RESOLUTION_AS(size) * IMAGE_HEIGHT / (FOV_DEG*60*60))

const int POLLMS = 500; /* Polling interval 500 ms */

static RadioSim *receiver;

static void cleanup()
{
    delete receiver;
}

void ISInit()
{
    static bool isInit = false;
    if (!isInit)
    {
        receiver = new RadioSim();

        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISGetProperties(dev);
        }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewSwitch(dev, name, states, names, num);
        }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewText(dev, name, texts, names, num);
        }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewNumber(dev, name, values, names, num);
        }
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    ISInit();

        receiver->ISSnoopDevice(root);
}

RadioSim::RadioSim()
{
	InCapture = false;
DishSize = 5;
Ra = 0;
Dec = FOV_DEG / 2;
    setDeviceName(getDefaultName());
	continuum = (uint8_t*)malloc(sizeof(uint8_t));
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool RadioSim::Connect()
{

    DEBUG(INDI::Logger::DBG_SESSION, "RadioSim connected successfully!");
	// Let's set a timer that checks teleDetectors status every POLLMS milliseconds.
    // JM 2017-07-31 SetTimer already called in updateProperties(). Just call it once
    //SetTimer(POLLMS);


	return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool RadioSim::Disconnect()
{
	InCapture = false;
	free(continuum);
	DEBUG(INDI::Logger::DBG_SESSION, "RadioSim Detector disconnected successfully!");
	return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *RadioSim::getDefaultName()
{
	return "RadioSim Receiver";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool RadioSim::initProperties()
{
	// Must init parent properties first!
	INDI::Detector::initProperties();

	// We set the Detector capabilities
	uint32_t cap = DETECTOR_CAN_ABORT | DETECTOR_HAS_CONTINUUM;
	SetDetectorCapability(cap);

	IUFillNumber(&DetectorPropertiesN[0], "DETECTOR_SIZE", "Dish size (m)", "%4.0f", 5, MAX_DISH_SIZE_M, 1, 5.0);
	IUFillNumberVector(&DetectorPropertiesNP, DetectorPropertiesN, 1, getDeviceName(), "DETECTOR_PROPERTIES", "Control", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

	IUFillNumber(&DetectorCoordsN[0], "DETECTOR_RA", "Position (RA)", "%2.3f", 0, FOV_DEG, 1, 0);
	IUFillNumber(&DetectorCoordsN[1], "DETECTOR_DEC", "Position (DEC)", "%2.3f", 0, FOV_DEG, 1, FOV_DEG/2);
	IUFillNumberVector(&DetectorCoordsNP, DetectorCoordsN, 2, getDeviceName(), "DETECTOR_COORDS", "Coordinates", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

	PrimaryDetector.setMinMaxStep("DETECTOR_CAPTURE", "DETECTOR_CAPTURE_VALUE", 0.3, 10, 1, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_FREQUENCY", FREQUENCY, FREQUENCY, 0, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_SAMPLERATE", 0, 1.0, 0, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_BITSPERSAMPLE", 8, 8, 1, false);

	// Add Debug, Simulator, and Configuration controls
	addAuxControls();

	return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool RadioSim::updateProperties()
{
	// Call parent update properties first
	INDI::Detector::updateProperties();

	if (isConnected())
	{
		// Let's get parameters now from Detector
		setupParams();
		defineNumber(&DetectorPropertiesNP);
		defineNumber(&DetectorCoordsNP);

		// Start the timer
		SetTimer(POLLMS);
	}

	return true;
}

/**************************************************************************************
** Setting up Detector parameters
***************************************************************************************/
void RadioSim::setupParams()
{
	// Our Detector is an 8 bit Detector, 100MHz frequency 1MHz bandwidth.
	SetDetectorParams(1, 1.42e+9, 8);
}

/**************************************************************************************
** INDI is asking us to submit list of properties for the device
***************************************************************************************/
void RadioSim::ISGetProperties(const char *dev)
{
    INDI::Detector::ISGetProperties(dev);

    if (isConnected())
    {
        // Define our properties
        defineNumber(&DetectorPropertiesNP);
        defineNumber(&DetectorCoordsNP);
    }
}

/**************************************************************************************
** Client is asking us to set a new number
***************************************************************************************/
bool RadioSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (strcmp (dev, getDeviceName()))
        return false;

    if (!strcmp(name, DetectorPropertiesNP.name))
    {
        IUUpdateNumber(&DetectorPropertiesNP, values, names, n);

        DishSize = (DetectorPropertiesN[0].value);

        DetectorPropertiesNP.s = IPS_OK;
        IDSetNumber(&DetectorPropertiesNP, NULL);
        return true;
    }

    if (!strcmp(name, DetectorCoordsNP.name))
    {
        IUUpdateNumber(&DetectorCoordsNP, values, names, n);

        Ra = (DetectorCoordsN[0].value);
        Dec = (DetectorCoordsN[1].value);

        DetectorCoordsNP.s = IPS_OK;
        IDSetNumber(&DetectorCoordsNP, NULL);
        return true;
    }

    return INDI::Detector::ISNewNumber(dev,name,values,names,n);
}

/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool RadioSim::StartCapture(float duration)
{
	CaptureRequest = duration;

	// Since we have only have one Detector with one chip, we set the exposure duration of the primary Detector
	PrimaryDetector.setCaptureDuration(duration);
	PrimaryDetector.setSampleRate(1.0 / duration);
	gettimeofday(&CapStart, nullptr);

	InCapture = true;

	// We're done
	return true;
}

/**************************************************************************************
** Client is updating capture settings
***************************************************************************************/
bool RadioSim::CaptureParamsUpdated(float sr, float freq, float bps)
{
    	INDI_UNUSED(bps);
    	INDI_UNUSED(freq);
    	INDI_UNUSED(sr);

	return true;
}

/**************************************************************************************
** Client is asking us to abort a capture
***************************************************************************************/
bool RadioSim::AbortCapture()
{
	InCapture = false;
	return true;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float RadioSim::CalcTimeLeft()
{
	double timesince;
	double timeleft;
	struct timeval now;
	gettimeofday(&now, nullptr);

	timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
				(double)(CapStart.tv_sec * 1000.0 + CapStart.tv_usec / 1000);
	timesince = timesince / 1000;

	timeleft = CaptureRequest - timesince;
	return timeleft;
}

/**************************************************************************************
** Main device loop. We check for capture progress here
***************************************************************************************/
void RadioSim::TimerHit()
{
	long timeleft;

	if (isConnected() == false)
		return; //  No need to reset timer if we are not connected anymore

	if (InCapture)
	{
		timeleft = CalcTimeLeft();
		if(timeleft < 0.1)
		{
			/* We're done capturing */
			DEBUG(INDI::Logger::DBG_SESSION, "Capture done, downloading data...");
			grabData();
			InCapture = false;
			timeleft = 0.0;
		}

		// This is an over simplified timing method, check DetectorSimulator and rtlsdrDetector for better timing checks
		PrimaryDetector.setCaptureLeft(timeleft);
	}
	double value = (DetectorCoordsN[0].value + (360.0 / SIDEREAL_DAY) * POLLMS / 1000.0);
	if (value >= FOV_DEG)
		value -= FOV_DEG;
	Ra = value;
	DetectorCoordsN[0].value = Ra;
	IDSetNumber(&DetectorCoordsNP, nullptr);
	SetTimer(POLLMS);
	return;
}

/**************************************************************************************
** Get the Data
***************************************************************************************/
void RadioSim::grabData()
{
	int to_read;
	int len = static_cast<int>RESOLUTION_PX(DishSize);
	double val = 0;
	int x = static_cast<int>(Ra * IMAGE_WIDTH / FOV_DEG);
	int y = static_cast<int>(Dec * IMAGE_HEIGHT / FOV_DEG);
	PrimaryDetector.setContinuumBufferSize(1);
	continuum = PrimaryDetector.getContinuumBuffer();
	for(to_read = 0; to_read < len && x + to_read < IMAGE_WIDTH; to_read++)
		val += MagickImage[x + (y * IMAGE_WIDTH) + to_read];
	continuum[0] = static_cast<unsigned char>(val / (to_read));
	IDMessage (getDeviceName(), "value: %d", continuum[0]);

	DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

	// Let INDI::Detector know we're done filling the data buffers
	CaptureComplete(&PrimaryDetector);
}
