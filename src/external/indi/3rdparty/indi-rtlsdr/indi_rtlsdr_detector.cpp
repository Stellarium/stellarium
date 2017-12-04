/*
    indi_rtlsdr_detector - a software defined radio driver for INDI
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

#include "indi_rtlsdr_detector.h"
#include <unistd.h>
#include <indilogger.h>
#include <libdspau.h>
#include <memory>

#define MAX_TRIES 20
#define MAX_DEVICES 4
#define SUBFRAME_SIZE 256
#define SAMPLE_RATE 10000

const int POLLMS = 500; /* Polling interval 500 ms */

static int iNumofConnectedDetectors;
static RTLSDR *receivers[MAX_DEVICES];

static void cleanup()
{
    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        delete receivers[i];
    }
}

void ISInit()
{
    static bool isInit = false;
    if (!isInit)
    {
        iNumofConnectedDetectors = 0;

        iNumofConnectedDetectors = rtlsdr_get_device_count();
        if (iNumofConnectedDetectors > MAX_DEVICES)
            iNumofConnectedDetectors = MAX_DEVICES;
        if (iNumofConnectedDetectors <= 0)
        {
            //Try sending IDMessage as well?
            IDLog("No RTLSDR receivers detected. Power on?");
            IDMessage(nullptr, "No RTLSDR receivers detected. Power on?");
        }
        else
        {
            for (int i = 0; i < iNumofConnectedDetectors; i++)
            {
                receivers[i] = new RTLSDR(i);
            }
        }

        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (iNumofConnectedDetectors == 0)
    {
        IDMessage(nullptr, "No RTLSDR receivers detected. Power on?");
        return;
    }

    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        RTLSDR *receiver = receivers[i];
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISGetProperties(dev);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        RTLSDR *receiver = receivers[i];
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewSwitch(dev, name, states, names, num);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        RTLSDR *receiver = receivers[i];
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewText(dev, name, texts, names, num);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        RTLSDR *receiver = receivers[i];
        if (dev == NULL || !strcmp(dev, receiver->getDeviceName()))
        {
            receiver->ISNewNumber(dev, name, values, names, num);
            if (dev != NULL)
                break;
        }
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

    for (int i = 0; i < iNumofConnectedDetectors; i++)
    {
        RTLSDR *receiver = receivers[i];
        receiver->ISSnoopDevice(root);
    }
}

RTLSDR::RTLSDR(uint32_t index)
{
	InCapture = false;

    detectorIndex = index;

    char name[MAXINDIDEVICE];
    snprintf(name, MAXINDIDEVICE, "%s %d", getDefaultName(), index);
    setDeviceName(name);
	continuum = (uint8_t*)malloc(sizeof(uint8_t));
	spectrum = (double *)malloc(sizeof(double));
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool RTLSDR::Connect()
{    	
    int r = rtlsdr_open(&rtl_dev, detectorIndex);
    if (r < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Failed to open rtlsdr device index %d.", detectorIndex);
		return false;
	}

    DEBUG(INDI::Logger::DBG_SESSION, "RTL-SDR Detector connected successfully!");
	// Let's set a timer that checks teleDetectors status every POLLMS milliseconds.
    // JM 2017-07-31 SetTimer already called in updateProperties(). Just call it once
    //SetTimer(POLLMS);


	return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool RTLSDR::Disconnect()
{
	InCapture = false;
	rtlsdr_close(rtl_dev);
	free(continuum);
	free(spectrum);
	DEBUG(INDI::Logger::DBG_SESSION, "RTL-SDR Detector disconnected successfully!");
	return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *RTLSDR::getDefaultName()
{
	return "RTL-SDR Receiver";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool RTLSDR::initProperties()
{
	// Must init parent properties first!
	INDI::Detector::initProperties();

	// We set the Detector capabilities
	uint32_t cap = DETECTOR_CAN_ABORT | DETECTOR_HAS_CONTINUUM | DETECTOR_HAS_SPECTRUM;
	SetDetectorCapability(cap);

	PrimaryDetector.setMinMaxStep("DETECTOR_CAPTURE", "DETECTOR_CAPTURE_VALUE", 0.1, 10, 1, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_FREQUENCY", 2.4e+7, 2.0e+9, 1, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_SAMPLERATE", 1, 1, 0, false);
	PrimaryDetector.setMinMaxStep("DETECTOR_SETTINGS", "DETECTOR_BITSPERSAMPLE", 8, 8, 1, false);

	// Add Debug, Simulator, and Configuration controls
	addAuxControls();

	return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool RTLSDR::updateProperties()
{
	// Call parent update properties first
	INDI::Detector::updateProperties();

	if (isConnected())
	{
		// Let's get parameters now from Detector
		setupParams();

		// Start the timer
		SetTimer(POLLMS);
	}

	return true;
}

/**************************************************************************************
** Setting up Detector parameters
***************************************************************************************/
void RTLSDR::setupParams()
{
	// Our Detector is an 8 bit Detector, 100MHz frequency 1MHz bandwidth.
	SetDetectorParams(10000.0, 1.0e+8, 8);
}

/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool RTLSDR::StartCapture(float duration)
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
bool RTLSDR::CaptureParamsUpdated(float sr, float freq, float bps)
{
    	INDI_UNUSED(bps);
    	INDI_UNUSED(sr);
	int r = 0;

	r = rtlsdr_set_center_freq(rtl_dev, (uint32_t)freq);
	if(r != 0)
		return false;
	if(rtlsdr_get_center_freq(rtl_dev) != freq)
		PrimaryDetector.setFrequency(rtlsdr_get_center_freq(rtl_dev));
	r = rtlsdr_set_sample_rate(rtl_dev, 1000000);
	if(r != 0)
		return false;

	return true;
}

/**************************************************************************************
** Client is asking us to abort a capture
***************************************************************************************/
bool RTLSDR::AbortCapture()
{
	InCapture = false;
	return true;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float RTLSDR::CalcTimeLeft()
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
void RTLSDR::TimerHit()
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

	SetTimer(POLLMS);
	return;
}

/**************************************************************************************
** Create the spectrum
***************************************************************************************/
void RTLSDR::grabData()
{
	int n_read, to_read, b_read;
	int len = SAMPLE_RATE * PrimaryDetector.getCaptureDuration();
	len -= (len % SUBFRAME_SIZE) - SUBFRAME_SIZE;
	if(len != PrimaryDetector.getContinuumBufferSize()) {
		PrimaryDetector.setContinuumBufferSize(1); 
		continuum = PrimaryDetector.getContinuumBuffer();
	}
	to_read = len;
	b_read = 0;
	unsigned char *data8 = (unsigned char *)malloc(len * sizeof(unsigned char));
	double *data48 = (double *)malloc(len * sizeof(double));
	double *flat48 = (double *)malloc(len * sizeof(double));
	spectrum = (double *)realloc(spectrum, len * sizeof(double));
	rtlsdr_reset_buffer(rtl_dev);
	while(to_read > 0 && b_read < len) {
		rtlsdr_read_sync(rtl_dev, data8 + b_read, to_read, &n_read);
		b_read += n_read;
		to_read = SUBFRAME_SIZE + ((len - b_read) % SUBFRAME_SIZE);
	}
	dspau_u8todouble(data8, data48, len);
	dspau_squarelawfilter(data48, flat48, len);
	continuum[0] = (unsigned char)dspau_mean(flat48, len);

        //Create the spectrum
	dspau_spectrum(data48, spectrum, 1, &len, magnitude_rooted);
	if(len != PrimaryDetector.getSpectrumBufferSize()) {
		PrimaryDetector.setSpectrumBufferSize(len, false);
		PrimaryDetector.setSpectrumBuffer(spectrum);
	}
	free(data8);
	free(data48);

	DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

	// Let INDI::Detector know we're done filling the data buffers
	CaptureComplete(&PrimaryDetector);
}
