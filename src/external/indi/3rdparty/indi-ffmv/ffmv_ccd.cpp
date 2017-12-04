/**
 * Copyright (C) 2013 Ben Gilsrud
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <sys/time.h>
#include <memory>
#include <stdint.h>
#include <arpa/inet.h>
#include <math.h>
#include <sys/time.h>
#include <dc1394/dc1394.h>
#include <indiapi.h>
#include <iostream>

#include "ffmv_ccd.h"
#include "config.h"

const int POLLMS = 250;

std::unique_ptr<FFMVCCD> ffmvCCD(new FFMVCCD());

/**
 * Write to registers in the MT9V022 chip.
 * This can be done by programming the address in 0x1A00 and writing to 0x1A04.
 */
dc1394error_t FFMVCCD::writeMicronReg(unsigned int offset, unsigned int val)
{
    dc1394error_t err;
    err = dc1394_set_control_register(dcam, 0x1A00, offset);
    if (err != DC1394_SUCCESS)
    {
        return err;
    }

    err = dc1394_set_control_register(dcam, 0x1A04, val);
    if (err != DC1394_SUCCESS)
    {
        return err;
    }

    return DC1394_SUCCESS;
}

/**
 * Read a register from the MT9V022 sensor.
 * This writes the MT9V022 register address to 0x1A00 and then reads from 0x1A04
 */
dc1394error_t FFMVCCD::readMicronReg(unsigned int offset, unsigned int *val)
{
    dc1394error_t err;
    err = dc1394_set_control_register(dcam, 0x1A00, offset);
    if (err != DC1394_SUCCESS)
    {
        return err;
    }

    err = dc1394_get_control_register(dcam, 0x1A04, val);
    if (err != DC1394_SUCCESS)
    {
        return err;
    }

    return DC1394_SUCCESS;
}

void ISGetProperties(const char *dev)
{
    ffmvCCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ffmvCCD->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ffmvCCD->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ffmvCCD->ISNewNumber(dev, name, values, names, num);
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
    ffmvCCD->ISSnoopDevice(root);
}

FFMVCCD::FFMVCCD()
{
    InExposure = false;
    capturing  = false;

    setVersion(FFMV_VERSION_MAJOR, FFMV_VERSION_MINOR);

    SetCCDCapability(CCD_CAN_ABORT);
}

/**************************************************************************************
** Client is asking us to establish connection to the device
***************************************************************************************/
bool FFMVCCD::Connect()
{
    dc1394camera_list_t *list;
    dc1394error_t err;
    bool supported;
    bool settings_valid;
    uint32_t val;
    dc1394format7mode_t fm7;
    dc1394feature_info_t feature;
    float min, max;

    dc1394 = dc1394_new();
    if (!dc1394)
    {
        return false;
    }

    err = dc1394_camera_enumerate(dc1394, &list);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not find DC1394 cameras!");
        return false;
    }
    if (!list->num)
    {
        IDMessage(getDeviceName(), "No DC1394 cameras found!");
        return false;
    }
    dcam = dc1394_camera_new(dc1394, list->ids[0].guid);
    if (!dcam)
    {
        IDMessage(getDeviceName(), "Unable to connect to camera!");
        return false;
    }

    /* Reset camera */
    err = dc1394_camera_reset(dcam);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to reset camera!");
        return false;
    }

    /* Set mode */
    err = dc1394_video_set_mode(dcam, DC1394_VIDEO_MODE_640x480_MONO16);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to connect to set videomode!");
        return false;
    }
    /* Disable Auto exposure control */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_EXPOSURE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable auto exposure control");
        return false;
    }

    /* Set frame rate to the lowest possible */
    err = dc1394_video_set_framerate(dcam, DC1394_FRAMERATE_7_5);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to connect to set framerate!");
        return false;
    }
    /* Turn frame rate control off to enable extended exposure (subs of 512ms) */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_FRAME_RATE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable framerate!");
        return false;
    }

    /* Get the longest possible exposure length */
    err = dc1394_feature_set_mode(dcam, DC1394_FEATURE_SHUTTER, DC1394_FEATURE_MODE_MANUAL);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable manual shutter control.");
    }
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_SHUTTER, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable absolute shutter control.");
    }
    err = dc1394_feature_get_absolute_boundaries(dcam, DC1394_FEATURE_SHUTTER, &min, &max);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not get max shutter length");
    }
    else
    {
        max_exposure = max;
    }

    /* Set gain to max. By setting the register directly, we can achieve a
     * gain of 24 dB...compared to a gain of 12 dB which is reported as the
     * max
     */
    err = dc1394_set_control_register(dcam, 0x820, 0x40);
    //err = dc1394_set_control_register(dcam, 0x820, 0x7f);
    if (err != DC1394_SUCCESS)
    {
        return err;
    }
#if 0
    /* Set absolute gain to max */
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_GAIN, DC1394_ON);
    if (err != DC1394_SUCCESS) {
        IDMessage(getDeviceName(), "Failed to enable ansolute gain control.");
    } 
    err = dc1394_feature_get_absolute_boundaries(dcam, DC1394_FEATURE_GAIN, &min, &max);
    if (err != DC1394_SUCCESS) {
        IDMessage(getDeviceName(), "Could not get max gain value");
    } else {
        err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_GAIN, max);
        if (err != DC1394_SUCCESS) {
            IDMessage(getDeviceName(), "Could not set max gain value");
        }
    }
#endif

    /* Set brightness */
    err = dc1394_feature_set_mode(dcam, DC1394_FEATURE_BRIGHTNESS, DC1394_FEATURE_MODE_MANUAL);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable manual brightness control.");
    }
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_BRIGHTNESS, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Failed to enable ansolute brightness control.");
    }
    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_BRIGHTNESS, 1);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set max brightness value");
    }

    /* Turn gamma control off */
    err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_GAMMA, 1);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Could not set gamma value");
    }
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_GAMMA, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable gamma!");
        return false;
    }

    /* Turn off white balance */
    err = dc1394_feature_set_power(dcam, DC1394_FEATURE_WHITE_BALANCE, DC1394_OFF);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to disable white balance!");
        return false;
    }

    err = dc1394_capture_setup(dcam, 10, DC1394_CAPTURE_FLAGS_DEFAULT);

    return true;
}

/**************************************************************************************
** Client is asking us to terminate connection to the device
***************************************************************************************/
bool FFMVCCD::Disconnect()
{
    if (dcam)
    {
        dc1394_capture_stop(dcam);
        dc1394_camera_free(dcam);
    }

    IDMessage(getDeviceName(), "Point Grey FireFly MV disconnected successfully!");
    return true;
}

/**************************************************************************************
** INDI is asking us for our default device name
***************************************************************************************/
const char *FFMVCCD::getDefaultName()
{
    return "FireFly MV";
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool FFMVCCD::initProperties()
{
    // Must init parent properties first!
    INDI::CCD::initProperties();

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();

    /* Add Gain Vref switch */
    IUFillSwitch(&GainS[0], "GAINVREF", "Vref Boost", ISS_OFF);
    IUFillSwitch(&GainS[1], "GAIN2X", "2x Digital Boost", ISS_OFF);
    IUFillSwitchVector(&GainSP, GainS, 2, getDeviceName(), "GAIN", "Gain", IMAGE_SETTINGS_TAB, IP_WO, ISR_NOFMANY, 0,
                       IPS_IDLE);

    return true;
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool FFMVCCD::updateProperties()
{
    // Call parent update properties first
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Let's get parameters now from CCD
        setupParams();

        // Start the timer
        SetTimer(POLLMS);
        defineSwitch(&GainSP);
    }
    else
    {
        deleteProperty(GainSP.name);
    }

    return true;
}

/**************************************************************************************
** Setting up CCD parameters
***************************************************************************************/
void FFMVCCD::setupParams()
{
    // The FireFly MV has a Micron MT9V022 CMOS sensor
    SetCCDParams(640, 480, 16, 6.0, 6.0);

    // Let's calculate how much memory we need for the primary CCD buffer
    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;
    PrimaryCCD.setFrameBufferSize(nbuf);
}

#define IMAGE_FILE_NAME "testimage.pgm"
/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool FFMVCCD::StartExposure(float duration)
{
    FILE *imagefile;
    dc1394error_t err;
    dc1394video_frame_t *frame;
    int i;
    int ms;
    unsigned int val;
    float gain = 1.0;
    uint32_t uwidth, uheight;
    float sub_length;
    float fval;

    ms = duration * 1000;

    //IDMessage(getDeviceName(), "Doing %d sub exposures at %f %s each", sub_count, absShutter, prop_info.pUnits);

    ExposureRequest = duration;

    // Since we have only have one CCD with one chip, we set the exposure duration of the primary CCD
    PrimaryCCD.setBPP(16);
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart, NULL);

    InExposure = true;
    IDMessage(getDeviceName(), "Exposure has begun.");

    // Let's get a pointer to the frame buffer
    uint8_t *image = PrimaryCCD.getFrameBuffer();

    // Get width and height
    int width  = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

    memset(image, 0, PrimaryCCD.getFrameBufferSize());

    if (duration != last_exposure_length)
    {
        /* Calculate the number of exposures needed */
        sub_count = duration / max_exposure;
        if (ms % ((int)(max_exposure * 1000)))
        {
            ++sub_count;
        }
        sub_length = duration / sub_count;

        IDMessage(getDeviceName(), "Triggering a %f second exposure using %d subs of %f seconds", duration, sub_count,
                  sub_length);
/* Set sub length */
#if 0
    err = dc1394_feature_set_absolute_control(dcam, DC1394_FEATURE_SHUTTER, DC1394_ON);
    if (err != DC1394_SUCCESS) {
        IDMessage(getDeviceName(), "Failed to enable ansolute shutter control.");
    }
#endif
        err = dc1394_feature_set_absolute_value(dcam, DC1394_FEATURE_SHUTTER, sub_length);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Unable to set shutter value.");
        }
        err = dc1394_feature_get_absolute_value(dcam, DC1394_FEATURE_SHUTTER, &fval);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Unable to get shutter value.");
        }
        IDMessage(getDeviceName(), "Shutter value is %f.", fval);
    }

    /* Flush the DMA buffer */
    while (1)
    {
        err = dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_POLL, &frame);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Flushing DMA buffer failed!");
            break;
        }
        if (!frame)
        {
            break;
        }
        dc1394_capture_enqueue(dcam, frame);
    }

    /*-----------------------------------------------------------------------
     *  have the camera start sending us data
     *-----------------------------------------------------------------------*/
    IDMessage(getDeviceName(), "start transmission");
    err = dc1394_video_set_transmission(dcam, DC1394_ON);
    if (err != DC1394_SUCCESS)
    {
        IDMessage(getDeviceName(), "Unable to start transmission");
        return false;
    }

    // We're done
    return true;
}

/**************************************************************************************
** Client is asking us to abort an exposure
***************************************************************************************/
bool FFMVCCD::AbortExposure()
{
    InExposure = false;
    return true;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float FFMVCCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, NULL);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = ExposureRequest - timesince;
    return timeleft;
}

/**
 * Set the digital gain in the MT9V022.
 * Sets the tiled digital gain to 2x what the default is.
 */
dc1394error_t FFMVCCD::setDigitalGain(ISState state)
{
    dc1394error_t err;
    unsigned int val;

    if (state == ISS_OFF)
    {
        err = writeMicronReg(0x80, 0xF4);
        err = readMicronReg(0x80, &val);
        if (err == DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Turned off digital gain boost. Tiled Digital Gain = 0x%x", val);
        }
        else
        {
            IDMessage(getDeviceName(), "readMicronReg failed!");
        }
    }
    else
    {
        err = writeMicronReg(0x80, 0xF8);
        err = readMicronReg(0x80, &val);
        if (err == DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Turned on digital gain boost. Tiled Digital Gain = 0x%x", val);
        }
        else
        {
            IDMessage(getDeviceName(), "readMicronReg failed!");
        }
    }
    return DC1394_SUCCESS;
}

/**
 * Set the ADC reference voltage for the MT9V022.
 * Decreasing the reference voltage will, in effect, increase the gain.
 * The default Vref_ADC is 1.4V (4). We will bump it down to 1.0V (0).
 */
dc1394error_t FFMVCCD::setGainVref(ISState state)
{
    dc1394error_t err;
    unsigned int val;

    if (state == ISS_OFF)
    {
        err = writeMicronReg(0x2C, 4);
        err = readMicronReg(0x2C, &val);
        if (err == DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Turned off Gain boost. VREF_ADC = 0x%x", val);
        }
        else
        {
            IDMessage(getDeviceName(), "readMicronReg failed!");
        }
    }
    else
    {
        err = writeMicronReg(0x2C, 0);
        err = readMicronReg(0x2C, &val);
        if (err == DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Turned on Gain boost. VREF_ADC = 0x%x", val);
        }
        else
        {
            IDMessage(getDeviceName(), "readMicronReg failed!");
        }
    }
    return DC1394_SUCCESS;
}

bool FFMVCCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        /* Reset */
        if (!strcmp(name, GainSP.name))
        {
            if (IUUpdateSwitch(&GainSP, states, names, n) < 0)
            {
                return false;
            }
            setGainVref(GainS[0].s);
            setDigitalGain(GainS[1].s);
            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

/**************************************************************************************
** Main device loop. We check for exposure progress
***************************************************************************************/
void FFMVCCD::TimerHit()
{
    long timeleft;

    if (isConnected() == false)
    {
        return; //  No need to reset timer if we are not connected anymore
    }

    if (InExposure)
    {
        timeleft = CalcTimeLeft();

        // Less than a 0.1 second away from exposure completion
        // This is an over simplified timing method, check CCDSimulator and ffmvCCD for better timing checks
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
        {
            // Just update time left in client
            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    SetTimer(POLLMS);
    return;
}

/**
 * Download image from FireFly
 */
void FFMVCCD::grabImage()
{
    dc1394error_t err;
    dc1394video_frame_t *frame;
    uint32_t uheight, uwidth;
    int sub;
    uint16_t val;
    struct timeval start, end;

    // Let's get a pointer to the frame buffer
    uint8_t *image = PrimaryCCD.getFrameBuffer();

    // Get width and height
    int width  = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

    memset(image, 0, PrimaryCCD.getFrameBufferSize());

    /*-----------------------------------------------------------------------
    *  stop data transmission
    *-----------------------------------------------------------------------*/

    gettimeofday(&start, NULL);
    for (sub = 0; sub < sub_count; ++sub)
    {
        IDMessage(getDeviceName(), "Getting sub %d of %d", sub, sub_count);
        err = dc1394_capture_dequeue(dcam, DC1394_CAPTURE_POLICY_WAIT, &frame);
        if (err != DC1394_SUCCESS)
        {
            IDMessage(getDeviceName(), "Could not capture frame");
        }
        dc1394_get_image_size_from_video_mode(dcam, DC1394_VIDEO_MODE_640x480_MONO16, &uwidth, &uheight);

        if (DC1394_TRUE == dc1394_capture_is_frame_corrupt(dcam, frame))
        {
            IDMessage(getDeviceName(), "Corrupt frame!");
            continue;
        }
        // Fill buffer with random pattern
        for (int i = 0; i < height; i++)
        {
            for (int j = 0; j < width; j++)
            {
                /* Detect unsigned overflow */
                val = ((uint16_t *)image)[i * width + j] + ntohs(((uint16_t *)(frame->image))[i * width + j]);
                if (val > ((uint16_t *)image)[i * width + j])
                {
                    ((uint16_t *)image)[i * width + j] = val;
                }
                else
                {
                    ((uint16_t *)image)[i * width + j] = 0xFFFF;
                }
            }
        }

        dc1394_capture_enqueue(dcam, frame);
    }
    err = dc1394_video_set_transmission(dcam, DC1394_OFF);
    IDMessage(getDeviceName(), "Download complete.");
    gettimeofday(&end, NULL);
    IDMessage(getDeviceName(), "Download took %d uS",
              (int)((end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec)));

    // Let INDI::CCD know we're done filling the image buffer
    ExposureComplete(&PrimaryCCD);
}
