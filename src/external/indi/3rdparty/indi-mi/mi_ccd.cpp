/*
 Moravian Instruments INDI Driver

 Copyright (C) 2014 Jasem Mutlaq (mutlaqja@ikarustech.com)
 Copyright (C) 2014 Zhirong Li (lzr@qhyccd.com)
 Copyright (C) 2015 Peter Polakovic (peter.polakovic@cloudmakers.eu)
 Copyright (C) 2016 Jakub Smutny (linux@gxccd.com)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "mi_ccd.h"

#include "config.h"

#include <math.h>

#define POLLMS         1000 /* Polling time (ms) */
#define TEMP_THRESHOLD 0.2  /* Differential temperature threshold (°C) */
#define MAX_DEVICES    4    /* Max device cameraCount */
#define MAX_ERROR_LEN  64   /* Max length of error buffer */

// There is _one_ binary for USB and ETH driver, but each binary is renamed
// to its variant (indi_mi_ccd_usb and indi_mi_ccd_eth). The main function will
// fetch from std args the binary name and ISInit will create the appropriate
// driver afterwards.
extern char *me;

static int cameraCount;
static int cameraIds[MAX_DEVICES];
static MICCD *cameras[MAX_DEVICES];

// Enumeration callback
static void enumCallback(int cameraId)
{
    cameraIds[cameraCount++] = cameraId;
}

static void cleanup()
{
    for (int i = 0; i < cameraCount; i++)
        delete cameras[i];
}

void ISInit()
{
    static bool isInit = false;
    if (isInit)
        return;

    isInit      = true;
    cameraCount = 0;
    bool eth    = false;

    if (strstr(me, "indi_mi_ccd_eth"))
    {
        eth = true;
        gxccd_enumerate_eth(enumCallback);
    }
    else
    {
        // "me" shoud be indi_mi_ccd_usb, however accept all names as USB
        gxccd_enumerate_usb(enumCallback);
    }

    for (int i = 0; i < cameraCount; i++)
    {
        cameras[i] = new MICCD(cameraIds[i], eth);
    }

    atexit(cleanup);
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (cameraCount == 0)
    {
        IDMessage(nullptr, "No Moravian cameras detected. Power on?");
        return;
    }

    for (int i = 0; i < cameraCount; i++)
    {
        MICCD *camera = cameras[i];
        if (!dev || !strcmp(dev, camera->name))
        {
            camera->ISGetProperties(dev);
            if (dev)
                break;
        }
    }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        MICCD *camera = cameras[i];
        if (!dev || !strcmp(dev, camera->name))
        {
            camera->ISNewSwitch(dev, name, states, names, num);
            if (dev)
                break;
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        MICCD *camera = cameras[i];
        if (!dev || !strcmp(dev, camera->name))
        {
            camera->ISNewText(dev, name, texts, names, num);
            if (dev)
                break;
        }
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        MICCD *camera = cameras[i];
        if (!dev || !strcmp(dev, camera->name))
        {
            camera->ISNewNumber(dev, name, values, names, num);
            if (dev)
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
    for (int i = 0; i < cameraCount; i++)
    {
        MICCD *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

MICCD::MICCD(int camId, bool eth) : FilterInterface(this)
{
    cameraId = camId;
    isEth    = eth;

    if (isEth)
        cameraHandle = gxccd_initialize_eth(cameraId);
    else
        cameraHandle = gxccd_initialize_usb(cameraId);
    if (!cameraHandle)
        IDLog("Error connecting MI camera!\n");

    char sp[MAXINDINAME];
    if (gxccd_get_string_parameter(cameraHandle, GSP_CAMERA_DESCRIPTION, sp, sizeof(sp)) < 0)
    {
        gxccd_get_last_error(cameraHandle, sp, sizeof(sp));
        IDLog("Error getting MI camera info: %s.\n", sp);
        strncpy(name, "MI CCD", MAXINDIDEVICE);
    }
    else
    {
        // trim trailing spaces
        char *end = sp + strlen(sp) - 1;
        while (end > sp && isspace(*end))
            end--;
        *(end + 1) = '\0';

        snprintf(name, MAXINDINAME, "MI CCD %s", sp);
        IDLog("Detected camera: %s.\n", name);
    }

    gxccd_get_integer_parameter(cameraHandle, GIP_READ_MODES, &numReadModes);
    gxccd_get_integer_parameter(cameraHandle, GIP_FILTERS, &numFilters);
    gxccd_get_integer_parameter(cameraHandle, GIP_MAX_FAN, &maxFanValue);
    gxccd_get_integer_parameter(cameraHandle, GIP_MAX_WINDOW_HEATING, &maxHeatingValue);

    gxccd_release(cameraHandle);
    cameraHandle = NULL;

    hasGain    = false;
    useShutter = true;

    setDeviceName(name);
    setVersion(INDI_MI_VERSION_MAJOR, INDI_MI_VERSION_MINOR);
}

MICCD::~MICCD()
{
    gxccd_release(cameraHandle);
}

const char *MICCD::getDefaultName()
{
    return name;
}

bool MICCD::initProperties()
{
    INDI::CCD::initProperties();
    INDI::FilterInterface::initProperties(FILTER_TAB);

    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = numFilters;

    // Temp ram
    IUFillNumber(&TemperatureRampN[0], "TEMP_RAMP", "Max. dT (C/min)", "%2.0f", 0, 30, 1, 2);
    IUFillNumberVector(&TemperatureRampNP, TemperatureRampN, 1, getDeviceName(), "CCD_TEMP_RAMP", "Temp. Ramp",
                       MAIN_CONTROL_TAB, IP_WO, 60, IPS_IDLE);

    // CCD Regulation power
    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "Cooling Power (%)", "%+6.2f", 0.0, 1.0, 0.01, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooling Power", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    // CCD Fan
    IUFillNumber(&FanN[0], "FAN", "Fan speed", "%2.0f", 0, maxFanValue, 1, 0);
    IUFillNumberVector(&FanNP, FanN, 1, getDeviceName(), "CCD_FAN", "Fan", MAIN_CONTROL_TAB, IP_WO, 60, IPS_IDLE);

    // CCD Window heating
    IUFillNumber(&WindowHeatingN[0], "WINDOW_HEATING", "Heating Intensity", "%2.0f", 0, maxHeatingValue, 1, 0);
    IUFillNumberVector(&WindowHeatingNP, WindowHeatingN, 1, getDeviceName(), "CCD_WINDOW_HEATING", "Window Heating",
                       MAIN_CONTROL_TAB, IP_WO, 60, IPS_IDLE);

    // CCD Gain
    IUFillNumber(&GainN[0], "GAIN", "Gain (e-/ADU)", "%2.2f", 0, 100, 1, 0);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "CCD_GAIN", "Gain", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Noise mode
    IUFillSwitch(&ReadModeS[0], "PREVIEW", "Preview", ISS_OFF);
    IUFillSwitch(&ReadModeS[1], "LOW_NOISE", "Low noise", numReadModes == 2 ? ISS_ON : ISS_OFF);
    IUFillSwitch(&ReadModeS[2], "ULTA_LOW_NOISE", "Ultra low noise", numReadModes == 3 ? ISS_ON : ISS_OFF);
    IUFillSwitchVector(&ReadModeSP, ReadModeS, numReadModes, getDeviceName(), "CCD_READ_MODE", "Read Mode",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    addAuxControls();

    setDriverInterface(getDriverInterface() | FILTER_INTERFACE);

    return true;
}

void MICCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    if (isConnected())
    {
        if (HasCooler())
        {
            defineNumber(&TemperatureRampNP);
            defineNumber(&CoolerNP);
        }

        defineSwitch(&ReadModeSP);

        if (maxFanValue > 0)
            defineNumber(&FanNP);

        if (maxHeatingValue > 0)
            defineNumber(&WindowHeatingNP);

        if (hasGain)
            defineNumber(&GainNP);

        if (numFilters > 0)
        {
           INDI::FilterInterface::updateProperties();
        }
    }
}

bool MICCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        if (HasCooler())
        {
            defineNumber(&TemperatureRampNP);
            defineNumber(&CoolerNP);
            temperatureID = IEAddTimer(POLLMS, MICCD::updateTemperatureHelper, this);
        }

        defineSwitch(&ReadModeSP);

        if (maxFanValue > 0)
            defineNumber(&FanNP);

        if (maxHeatingValue > 0)
            defineNumber(&WindowHeatingNP);

        if (hasGain)
            defineNumber(&GainNP);

        if (numFilters > 0)
        {
            INDI::FilterInterface::updateProperties();
        }

        // Let's get parameters now from CCD
        setupParams();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        if (HasCooler())
        {
            deleteProperty(TemperatureRampNP.name);
            deleteProperty(CoolerNP.name);
            RemoveTimer(temperatureID);
        }

        deleteProperty(ReadModeSP.name);

        if (maxFanValue > 0)
            deleteProperty(FanNP.name);

        if (maxHeatingValue > 0)
            deleteProperty(WindowHeatingNP.name);

        if (hasGain)
            deleteProperty(GainNP.name);

        if (numFilters > 0)
        {
            INDI::FilterInterface::updateProperties();
        }
        RemoveTimer(timerID);
    }

    return true;
}

bool MICCD::Connect()
{
    uint32_t cap = 0;

    if (isSimulation())
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Connected to %s", name);

        cap = CCD_CAN_SUBFRAME | CCD_CAN_ABORT | CCD_CAN_BIN | CCD_HAS_SHUTTER | CCD_HAS_COOLER;
        SetCCDCapability(cap);

        numFilters = 5;

        return true;
    }

    if (!cameraHandle)
    {
        if (isEth)
            cameraHandle = gxccd_initialize_eth(cameraId);
        else
            cameraHandle = gxccd_initialize_usb(cameraId);
    }
    if (!cameraHandle)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error connecting to %s.", name);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "Connected to %s.", name);

    bool value;
    cap = CCD_CAN_ABORT | CCD_CAN_BIN;

    gxccd_get_boolean_parameter(cameraHandle, GBP_SUB_FRAME, &value);
    if (value)
        cap |= CCD_CAN_SUBFRAME;

    gxccd_get_boolean_parameter(cameraHandle, GBP_GUIDE, &value);
    if (value)
        cap |= CCD_HAS_ST4_PORT;

    gxccd_get_boolean_parameter(cameraHandle, GBP_SHUTTER, &value);
    if (value)
        cap |= CCD_HAS_SHUTTER;

    gxccd_get_boolean_parameter(cameraHandle, GBP_COOLER, &value);
    if (value)
        cap |= CCD_HAS_COOLER;

    gxccd_get_boolean_parameter(cameraHandle, GBP_GAIN, &hasGain);

    SetCCDCapability(cap);
    return true;
}

bool MICCD::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "CCD is offline.");
    gxccd_release(cameraHandle);
    cameraHandle = NULL;
    return true;
}

bool MICCD::setupParams()
{
    bool sim = isSimulation();
    if (sim)
    {
        SetCCDParams(4032, 2688, 16, 9, 9);
    }
    else
    {
        int chipW, chipD, pixelW, pixelD;
        gxccd_get_integer_parameter(cameraHandle, GIP_CHIP_W, &chipW);
        gxccd_get_integer_parameter(cameraHandle, GIP_CHIP_D, &chipD);
        gxccd_get_integer_parameter(cameraHandle, GIP_PIXEL_W, &pixelW);
        gxccd_get_integer_parameter(cameraHandle, GIP_PIXEL_D, &pixelD);

        SetCCDParams(chipW, chipD, 16, pixelW / 1000.0, pixelD / 1000.0);
    }

    int nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;
    nbuf += 512;
    PrimaryCCD.setFrameBufferSize(nbuf);

    int expTime = 0;
    gxccd_get_integer_parameter(cameraHandle, GIP_MINIMAL_EXPOSURE, &expTime);
    minExpTime = expTime / 1000000.0; // convert to seconds
    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", minExpTime, 3600, 1, false);

    gxccd_get_integer_parameter(cameraHandle, GIP_MAX_BINNING_X, &maxBinX);
    gxccd_get_integer_parameter(cameraHandle, GIP_MAX_BINNING_Y, &maxBinY);

    if (!sim && hasGain)
    {
        float gain = 0;
        if (gxccd_get_value(cameraHandle, GV_ADC_GAIN, &gain) < 0)
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Getting gain failed: %s.", errorStr);
            GainN[0].value = 0;
            GainNP.s       = IPS_ALERT;
            IDSetNumber(&GainNP, NULL);
            return false;
        }
        else
        {
            GainN[0].value = gain;
            GainNP.s       = IPS_OK;
            IDSetNumber(&GainNP, NULL);
        }
    }

    return true;
}

int MICCD::SetTemperature(double temperature)
{
    // If there difference, for example, is less than TEMP_THRESHOLD degrees, let's immediately return OK.
    if (fabs(temperature - TemperatureN[0].value) < TEMP_THRESHOLD)
        return 1;

    TemperatureRequest = temperature;

    if (!isSimulation() && gxccd_set_temperature(cameraHandle, temperature) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "Setting temperature failed: %s.", errorStr);
        return -1;
    }

    return 0;
}

bool MICCD::StartExposure(float duration)
{
    useShutter = true;

    if (duration < minExpTime)
    {
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Exposure shorter than minimum duration %g s requested. Setting exposure time to %g s.", duration,
               minExpTime);
        duration = minExpTime;
    }

    imageFrameType = PrimaryCCD.getFrameType();

    if (imageFrameType == INDI::CCDChip::BIAS_FRAME)
    {
        duration = minExpTime;
    }
    else if (imageFrameType == INDI::CCDChip::DARK_FRAME)
    {
        useShutter = false;
    }

    if (!isSimulation())
    {
        // read modes in G2/G3/G4 cameras are in inverse order, we have to calculate correct value
        int mode, prm;
        gxccd_get_integer_parameter(cameraHandle, GIP_PREVIEW_READ_MODE, &prm);
        if (prm == 0) // preview mode == 0 means smaller G0/G1 cameras
            mode = IUFindOnSwitchIndex(&ReadModeSP);
        else
            mode = prm - IUFindOnSwitchIndex(&ReadModeSP);
        gxccd_set_read_mode(cameraHandle, mode);
        // send binned coords
        int x = PrimaryCCD.getSubX() / PrimaryCCD.getBinX();
        int y = PrimaryCCD.getSubY() / PrimaryCCD.getBinY();
        int w = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
        int d = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();
        gxccd_start_exposure(cameraHandle, duration, useShutter, x, y, w, d);
    }

    ExposureRequest = duration;
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart, NULL);
    InExposure  = true;
    downloading = false;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Taking a %g seconds frame...", ExposureRequest);
    return true;
}

bool MICCD::AbortExposure()
{
    if (InExposure && !isSimulation())
    {
        if (gxccd_abort_exposure(cameraHandle, false) < 0)
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Aborting exposure failed: %s.", errorStr);
            return false;
        }
    }

    InExposure  = false;
    downloading = false;
    DEBUG(INDI::Logger::DBG_SESSION, "Exposure aborted.");
    return true;
}

bool MICCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    /* Add the X and Y offsets */
    long x_1 = x / PrimaryCCD.getBinX();
    long y_1 = y / PrimaryCCD.getBinY();

    long x_2 = x_1 + (w / PrimaryCCD.getBinX());
    long y_2 = y_1 + (h / PrimaryCCD.getBinY());

    if (x_2 > PrimaryCCD.getXRes())
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: Requested width out of bounds %ld", x_2);
        return false;
    }
    else if (y_2 > PrimaryCCD.getYRes())
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: Requested height out of bounds %ld", y_2);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "The Final image area is (%ld, %ld), (%ld, %ld)\n", x_1, y_1, x_2, y_2);

    int imageWidth  = x_2 - x_1;
    int imageHeight = y_2 - y_1;

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x, y, w, h);
    int nbuf = imageWidth * imageHeight * PrimaryCCD.getBPP() / 8; //  this is pixel count
    nbuf += 512;                                                   //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

bool MICCD::UpdateCCDBin(int hor, int ver)
{
    if (hor < 1 || hor > maxBinX || ver < 1 || ver > maxBinY)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Binning (%dx%d) are out of range. Range from 1x1 to (%dx%d)", maxBinX,
               maxBinY);
        return false;
    }
    if (gxccd_set_binning(cameraHandle, hor, ver) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "Setting binning failed: %s.", errorStr);
        return false;
    }
    PrimaryCCD.setBin(hor, ver);
    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

float MICCD::calcTimeLeft()
{
    double timesince;
    struct timeval now;
    gettimeofday(&now, NULL);

    timesince = (now.tv_sec * 1000.0 + now.tv_usec / 1000.0) - (ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000.0);
    return ExposureRequest - timesince / 1000.0;
}

static void mirror_image(void *buf, size_t w, size_t d)
{
    size_t w2     = w * 2;
    size_t half_d = d / 2;

    for (size_t line = 1; line <= half_d; line++)
    {
        uint16_t *sa = (uint16_t *)((char *)buf + (line - 1) * w2);
        uint16_t *da = (uint16_t *)((char *)buf + (d - line) * w2);
        for (size_t index = 1; index <= w; index++)
        {
            uint16_t tmp = *sa;
            *sa          = *da;
            *da          = tmp;
            ++sa;
            ++da;
        }
    }
}

/* Downloads the image from the CCD. */
int MICCD::grabImage()
{
    int ret              = 0;
    unsigned char *image = (unsigned char *)PrimaryCCD.getFrameBuffer();

    int width  = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

    if (isSimulation())
    {
        uint16_t *buffer = (uint16_t *)image;

        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                buffer[i * width + j] = rand() % UINT16_MAX;
    }
    else
    {
        ret = gxccd_read_image(cameraHandle, image, PrimaryCCD.getFrameBufferSize());
        if (ret < 0)
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Error getting image: %s.", errorStr);
        }
        else
        {
            mirror_image(image, width, height);
        }
    }

    if (ExposureRequest > POLLMS * 5 && !ret)
        DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    downloading = false;
    ExposureComplete(&PrimaryCCD);

    return ret;
}

void MICCD::TimerHit()
{
    if (!isConnected())
        return; // No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        float timeleft = calcTimeLeft();
        bool ready     = false;

        if (!downloading && (gxccd_image_ready(cameraHandle, &ready) < 0))
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Getting image ready failed: %s.", errorStr);
        }
        if (ready)
        {
            PrimaryCCD.setExposureLeft(0);
            InExposure  = false;
            downloading = true;

            // Don't spam the session log unless it is a long exposure > 5 seconds
            if (ExposureRequest > POLLMS * 5)
                DEBUG(INDI::Logger::DBG_SESSION, "Exposure done, downloading image...");

            // grab and save image
            grabImage();
        }
        // camera may need some time for image download -> update client only for positive values
        else if (timeleft >= 0)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Exposure in progress: Time left %.2fs", timeleft);
            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    SetTimer(POLLMS);
}

int MICCD::QueryFilter()
{
    return CurrentFilter;
}

bool MICCD::SelectFilter(int position)
{
    if (!isSimulation() && gxccd_set_filter(cameraHandle, position - 1) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "Setting filter failed: %s.", errorStr);
        return false;
    }

    CurrentFilter = position;
    SelectFilterDone(position);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Filter changed to %d", position);
    return true;
}

IPState MICCD::GuideNorth(float duration)
{
    if (gxccd_move_telescope(cameraHandle, 0, duration) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "GuideNorth() failed: %s.", errorStr);
        return IPS_ALERT;
    }
    return IPS_OK;
}

IPState MICCD::GuideSouth(float duration)
{
    if (gxccd_move_telescope(cameraHandle, 0, -duration) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "GuideSouth() failed: %s.", errorStr);
        return IPS_ALERT;
    }
    return IPS_OK;
}

IPState MICCD::GuideEast(float duration)
{
    if (gxccd_move_telescope(cameraHandle, -duration, 0) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "GuideEast() failed: %s.", errorStr);
        return IPS_ALERT;
    }
    return IPS_OK;
}

IPState MICCD::GuideWest(float duration)
{
    if (gxccd_move_telescope(cameraHandle, duration, 0) < 0)
    {
        char errorStr[MAX_ERROR_LEN];
        gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
        DEBUGF(INDI::Logger::DBG_ERROR, "GuideWest() failed: %s.", errorStr);
        return IPS_ALERT;
    }
    return IPS_OK;
}

bool MICCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, ReadModeSP.name))
        {
            IUUpdateSwitch(&ReadModeSP, states, names, n);
            ReadModeSP.s = IPS_OK;
            IDSetSwitch(&ReadModeSP, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool MICCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, FilterNameTP->name))
        {
            INDI::FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool MICCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, FilterSlotNP.name))
        {
            INDI::FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }

        if (!strcmp(name, FanNP.name))
        {
            IUUpdateNumber(&FanNP, values, names, n);

            if (!isSimulation() && gxccd_set_fan(cameraHandle, FanN[0].value) < 0)
            {
                char errorStr[MAX_ERROR_LEN];
                gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
                DEBUGF(INDI::Logger::DBG_ERROR, "Setting fan failed: %s.", errorStr);
                FanNP.s = IPS_ALERT;
            }
            else
            {
                FanNP.s = IPS_OK;
            }

            IDSetNumber(&FanNP, NULL);
            return true;
        }

        if (!strcmp(name, WindowHeatingNP.name))
        {
            IUUpdateNumber(&WindowHeatingNP, values, names, n);

            if (!isSimulation() && gxccd_set_window_heating(cameraHandle, WindowHeatingN[0].value) < 0)
            {
                char errorStr[MAX_ERROR_LEN];
                gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
                DEBUGF(INDI::Logger::DBG_ERROR, "Setting heating failed: %s.", errorStr);
                WindowHeatingNP.s = IPS_ALERT;
            }
            else
            {
                WindowHeatingNP.s = IPS_OK;
            }

            IDSetNumber(&WindowHeatingNP, NULL);
            return true;
        }

        if (!strcmp(name, TemperatureRampNP.name))
        {
            IUUpdateNumber(&TemperatureRampNP, values, names, n);

            if (!isSimulation() && gxccd_set_temperature_ramp(cameraHandle, TemperatureRampN[0].value) < 0)
            {
                char errorStr[MAX_ERROR_LEN];
                gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
                DEBUGF(INDI::Logger::DBG_ERROR, "Setting temp. ramp failed: %s.", errorStr);
                TemperatureRampNP.s = IPS_ALERT;
            }
            else
            {
                TemperatureRampNP.s = IPS_OK;
            }

            IDSetNumber(&TemperatureRampNP, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

void MICCD::updateTemperatureHelper(void *p)
{
    if (static_cast<MICCD *>(p)->isConnected())
        static_cast<MICCD *>(p)->updateTemperature();
}

void MICCD::updateTemperature()
{
    float ccdtemp  = 0;
    float ccdpower = 0;
    int err        = 0;

    if (isSimulation())
    {
        ccdtemp = TemperatureN[0].value;
        if (TemperatureN[0].value < TemperatureRequest)
            ccdtemp += TEMP_THRESHOLD;
        else if (TemperatureN[0].value > TemperatureRequest)
            ccdtemp -= TEMP_THRESHOLD;

        ccdpower = 30;
    }
    else
    {
        if (gxccd_get_value(cameraHandle, GV_CHIP_TEMPERATURE, &ccdtemp) < 0)
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Getting temperature failed: %s.", errorStr);
            err |= 1;
        }
        if (gxccd_get_value(cameraHandle, GV_POWER_UTILIZATION, &ccdpower) < 0)
        {
            char errorStr[MAX_ERROR_LEN];
            gxccd_get_last_error(cameraHandle, errorStr, sizeof(errorStr));
            DEBUGF(INDI::Logger::DBG_ERROR, "Getting voltage failed: %s.", errorStr);
            err |= 2;
        }
    }

    TemperatureN[0].value = ccdtemp;
    CoolerN[0].value      = ccdpower * 100.0;

    if (TemperatureNP.s == IPS_BUSY && fabs(TemperatureN[0].value - TemperatureRequest) <= TEMP_THRESHOLD)
    {
        // end of temperature ramp
        TemperatureN[0].value = TemperatureRequest;
        TemperatureNP.s       = IPS_OK;
    }

    if (err)
    {
        if (err & 1)
            TemperatureNP.s = IPS_ALERT;
        if (err & 2)
            CoolerNP.s = IPS_ALERT;
    }
    else
    {
        CoolerNP.s = IPS_OK;
    }

    IDSetNumber(&TemperatureNP, NULL);
    IDSetNumber(&CoolerNP, NULL);
    temperatureID = IEAddTimer(POLLMS, MICCD::updateTemperatureHelper, this);
}

bool MICCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &TemperatureRampNP);
    IUSaveConfigSwitch(fp, &ReadModeSP);

    if (numFilters > 0)
    {
        INDI::FilterInterface::saveConfigItems(fp);
    }

    if (maxFanValue > 0)
        IUSaveConfigNumber(fp, &FanNP);

    if (maxHeatingValue > 0)
        IUSaveConfigNumber(fp, &WindowHeatingNP);

    return true;
}
