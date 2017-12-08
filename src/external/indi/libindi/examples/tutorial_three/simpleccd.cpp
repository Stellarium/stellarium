/*
   INDI Developers Manual
   Tutorial #3

   "Simple CCD Driver"

   We develop a simple CCD driver.

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file simpleccd.cpp
    \brief Construct a basic INDI CCD device that simulates exposure & temperature settings. It also generates a random pattern and uploads it as a FITS file.
    \author Jasem Mutlaq

    \example simpleccd.cpp
    A simple CCD device that can capture images and control temperature. It returns a FITS image to the client. To build drivers for complex CCDs, please
    refer to the INDI Generic CCD driver template in INDI SVN (under 3rdparty).
*/

#include "simpleccd.h"

#include <memory>

const int POLLMS           = 500; /* Polling interval 500 ms */
//const int MAX_CCD_TEMP     = 45;  /* Max CCD temperature */
//const int MIN_CCD_TEMP     = -55; /* Min CCD temperature */
//const float TEMP_THRESHOLD = .25; /* Differential temperature threshold (C)*/

/* Macro shortcut to CCD temperature value */
#define currentCCDTemperature TemperatureN[0].value

std::unique_ptr<SimpleCCD> simpleCCD(new SimpleCCD());

void ISGetProperties(const char *dev)
{
    simpleCCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    simpleCCD->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    simpleCCD->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    simpleCCD->ISNewNumber(dev, name, values, names, n);
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
    simpleCCD->ISSnoopDevice(root);
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool SimpleCCD::Connect()
{
    IDMessage(getDeviceName(), "Simple CCD connected successfully!");

    // Let's set a timer that checks teleCCDs status every POLLMS milliseconds.
    SetTimer(POLLMS);
    return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool SimpleCCD::Disconnect()
{
    IDMessage(getDeviceName(), "Simple CCD disconnected successfully!");
    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *SimpleCCD::getDefaultName()
{
    return "Simple CCD";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool SimpleCCD::initProperties()
{
    // Must init parent properties first!
    INDI::CCD::initProperties();

    // We set the CCD capabilities
    uint32_t cap = CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_COOLER | CCD_HAS_SHUTTER;
    SetCCDCapability(cap);

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();

    return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool SimpleCCD::updateProperties()
{
    // Call parent update properties first
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Let's get parameters now from CCD
        setupParams();

        // Start the timer
        SetTimer(POLLMS);
    }

    return true;
}

/**************************************************************************************
** Setting up CCD parameters
***************************************************************************************/
void SimpleCCD::setupParams()
{
    // Our CCD is an 8 bit CCD, 1280x1024 resolution, with 5.4um square pixels.
    SetCCDParams(1280, 1024, 8, 5.4, 5.4);

    // Let's calculate how much memory we need for the primary CCD buffer
    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;
    nbuf += 512; //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);
}

/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool SimpleCCD::StartExposure(float duration)
{
    ExposureRequest = duration;

    // Since we have only have one CCD with one chip, we set the exposure duration of the primary CCD
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart, nullptr);

    InExposure = true;

    // We're done
    return true;
}

/**************************************************************************************
** Client is asking us to abort an exposure
***************************************************************************************/
bool SimpleCCD::AbortExposure()
{
    InExposure = false;
    return true;
}

/**************************************************************************************
** Client is asking us to set a new temperature
***************************************************************************************/
int SimpleCCD::SetTemperature(double temperature)
{
    TemperatureRequest = temperature;

    // 0 means it will take a while to change the temperature
    return 0;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float SimpleCCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now { 0, 0 };
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = ExposureRequest - timesince;
    return timeleft;
}

/**************************************************************************************
** Main device loop. We check for exposure and temperature progress here
***************************************************************************************/
void SimpleCCD::TimerHit()
{
    long timeleft;

    if (!isConnected())
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        timeleft = CalcTimeLeft();

        // Less than a 0.1 second away from exposure completion
        // This is an over simplified timing method, check CCDSimulator and simpleCCD for better timing checks
        if (timeleft < 0.1)
        {
            /* We're done exposing */
            IDMessage(getDeviceName(), "Exposure done, downloading image...");

            // Set exposure left to zero
            PrimaryCCD.setExposureLeft(0);

            // We're no longer exposing...
            InExposure = false;

            /* grab and save image */
            grabImage();
        }
        else
            // Just update time left in client
            PrimaryCCD.setExposureLeft(timeleft);
    }

    // TemperatureNP is defined in INDI::CCD
    switch (TemperatureNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            break;

        case IPS_BUSY:
            /* If target temperature is higher, then increase current CCD temperature */
            if (currentCCDTemperature < TemperatureRequest)
                currentCCDTemperature++;
            /* If target temperature is lower, then decrese current CCD temperature */
            else if (currentCCDTemperature > TemperatureRequest)
                currentCCDTemperature--;
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
void SimpleCCD::grabImage()
{
    // Let's get a pointer to the frame buffer
    uint8_t *image = PrimaryCCD.getFrameBuffer();

    // Get width and height
    int width  = PrimaryCCD.getSubW() / PrimaryCCD.getBinX() * PrimaryCCD.getBPP() / 8;
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

    // Fill buffer with random pattern
    for (int i = 0; i < height; i++)
        for (int j = 0; j < width; j++)
            image[i * width + j] = rand() % 255;

    IDMessage(getDeviceName(), "Download complete.");

    // Let INDI::CCD know we're done filling the image buffer
    ExposureComplete(&PrimaryCCD);
}
