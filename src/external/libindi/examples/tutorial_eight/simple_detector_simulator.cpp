/*
   INDI Developers Manual
   Tutorial #3

   "Simple Detector Driver"

   We develop a simple Detector driver.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file simpledetector.cpp
    \brief Construct a basic INDI Detector device that simulates exposure & temperature settings. It also generates a random pattern and uploads it as a FITS file.
    \author Ilia Platone, clearly taken from SimpleCCD by Jasem Mutlaq

    \example simpledetector.cpp
    A simple Detector device that can capture images and control temperature. It returns a FITS image to the client. To build drivers for complex Detectors, please
    refer to the INDI Generic Detector driver template in INDI SVN (under 3rdparty).
*/

#include "simple_detector_simulator.h"

#include <memory>

const int POLLMS           = 500; /* Polling interval 500 ms */
//const int MAX_DETECTOR_TEMP     = 45;  /* Max Detector temperature */
//const int MIN_DETECTOR_TEMP     = -55; /* Min Detector temperature */
//const float TEMP_THRESHOLD = .25; /* Differential temperature threshold (C)*/

/* Macro shortcut to Detector temperature value */
#define currentDetectorTemperature TemperatureN[0].value

std::unique_ptr<SimpleDetector> simpleDetector(new SimpleDetector());

void ISGetProperties(const char *dev)
{
    simpleDetector->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    simpleDetector->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    simpleDetector->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    simpleDetector->ISNewNumber(dev, name, values, names, n);
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
    simpleDetector->ISSnoopDevice(root);
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool SimpleDetector::Connect()
{
    IDMessage(getDeviceName(), "Simple Detector connected successfully!");

    // Let's set a timer that checks teleDetectors status every POLLMS milliseconds.
    SetTimer(POLLMS);

    return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool SimpleDetector::Disconnect()
{
    IDMessage(getDeviceName(), "Simple Detector disconnected successfully!");
    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *SimpleDetector::getDefaultName()
{
    return "Simple Detector";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool SimpleDetector::initProperties()
{
    // Must init parent properties first!
    INDI::Detector::initProperties();

    // We set the Detector capabilities
    uint32_t cap = DETECTOR_CAN_ABORT | DETECTOR_HAS_COOLER | DETECTOR_HAS_SHUTTER | DETECTOR_HAS_CONTINUUM | DETECTOR_HAS_SPECTRUM;
    SetDetectorCapability(cap);

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();

    return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool SimpleDetector::updateProperties()
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
** Client is updating capture settings
***************************************************************************************/
bool SimpleDetector::CaptureParamsUpdated(float sr, float freq, float bps)
{
    	INDI_UNUSED(bps);
    	INDI_UNUSED(freq);
    	INDI_UNUSED(sr);
	return true;
}

/**************************************************************************************
** Setting up Detector parameters
***************************************************************************************/
void SimpleDetector::setupParams()
{
    // Our Detector is an 8 bit Detector, 100MHz frequency 1MHz samplerate.
    SetDetectorParams(1000000.0, 100000000.0, 8);
}

/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool SimpleDetector::StartCapture(float duration)
{
    CaptureRequest = duration;

    // Since we have only have one Detector with one chip, we set the exposure duration of the primary Detector
    PrimaryDetector.setCaptureDuration(duration);

    gettimeofday(&CapStart, nullptr);

    InCapture = true;

    // We're done
    return true;
}

/**************************************************************************************
** Client is asking us to abort an exposure
***************************************************************************************/
bool SimpleDetector::AbortCapture()
{
    InCapture = false;
    return true;
}

/**************************************************************************************
** Client is asking us to set a new temperature
***************************************************************************************/
int SimpleDetector::SetTemperature(double temperature)
{
    TemperatureRequest = temperature;

    // 0 means it will take a while to change the temperature
    return 0;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float SimpleDetector::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };

    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(CapStart.tv_sec * 1000.0 + CapStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = CaptureRequest - timesince;
    return timeleft;
}

/**************************************************************************************
** Main device loop. We check for exposure and temperature progress here
***************************************************************************************/
void SimpleDetector::TimerHit()
{
    long timeleft;

    if (!isConnected())
        return; //  No need to reset timer if we are not connected anymore

    if (InCapture)
    {
        timeleft = CalcTimeLeft();

        // Less than a 0.1 second away from exposure completion
        // This is an over simplified timing method, check DetectorSimulator and simpleDetector for better timing checks
        if (timeleft < 0.1)
        {
            /* We're done exposing */
            IDMessage(getDeviceName(), "Capture done, downloading image...");

            // Set exposure left to zero
            PrimaryDetector.setCaptureLeft(0);

            // We're no longer exposing...
            InCapture = false;

            /* grab and save image */
            grabFrame();
        }
        else
            // Just update time left in client
            PrimaryDetector.setCaptureLeft(timeleft);
    }

    // TemperatureNP is defined in INDI::Detector
    switch (TemperatureNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            break;

        case IPS_BUSY:
            /* If target temperature is higher, then increase current Detector temperature */
            if (currentDetectorTemperature < TemperatureRequest)
                currentDetectorTemperature++;
            /* If target temperature is lower, then decrese current Detector temperature */
            else if (currentDetectorTemperature > TemperatureRequest)
                currentDetectorTemperature--;
            /* If they're equal, stop updating */
            else
            {
                TemperatureNP.s = IPS_OK;
                IDSetNumber(&TemperatureNP, "Target temperature reached.");

                break;
            }

            IDSetNumber(&TemperatureNP, nullptr);

            break;

        case IPS_ALERT:
            break;
    }

    SetTimer(POLLMS);
}

/**************************************************************************************
** Create a random image and return it to client
***************************************************************************************/
void SimpleDetector::grabFrame()
{
    // Set length of continuum
    int len  = PrimaryDetector.getSampleRate() * PrimaryDetector.getCaptureDuration() * PrimaryDetector.getBPS() / 8;
    PrimaryDetector.setContinuumBufferSize(len);
 
   // Let's get a pointer to the frame buffer
    uint8_t *continuum = PrimaryDetector.getContinuumBuffer();

    // Fill buffer with random pattern
    for (int i = 0; i < len; i++)
        continuum[i] = rand() % 255;

    // Set length of spectrum
    len  = 1000;
    PrimaryDetector.setSpectrumBufferSize(len);
 
   // Let's get a pointer to the frame buffer
    double *spectrum = PrimaryDetector.getSpectrumBuffer();

    // Fill buffer with random pattern
    for (int i = 0; i < len; i++)
        spectrum[i] = rand() % 255;

    IDMessage(getDeviceName(), "Download complete.");

    // Let INDI::Detector know we're done filling the image buffer
    CaptureComplete(&PrimaryDetector);
}
