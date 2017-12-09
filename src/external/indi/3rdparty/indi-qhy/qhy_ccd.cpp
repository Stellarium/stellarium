/*
 QHY INDI Driver

 Copyright (C) 2014 Jasem Mutlaq (mutlaqja@ikarustech.com)
 Copyright (C) 2014 Zhirong Li (lzr@qhyccd.com)
 Copyright (C) 2015 Peter Polakovic (peter.polakovic@cloudmakers.eu)

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

#include "qhy_ccd.h"

#include "config.h"
#include <stream/streammanager.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <log4z.h>
#pragma GCC diagnostic pop

#include <algorithm>
#include <math.h>

#define POLLMS               1000  /* Polling time (ms) */
#define TEMP_THRESHOLD       0.2   /* Differential temperature threshold (C)*/
#define MAX_DEVICES          4     /* Max device cameraCount */

//NB Disable for real driver
//#define USE_SIMULATION

static int cameraCount = 0;
static QHYCCD *cameras[MAX_DEVICES];

namespace
{
static void QhyCCDCleanup()
{
    for (int i = 0; i < cameraCount; i++)
    {
        delete cameras[i];
    }

    //ReleaseQHYCCDResource();
}

// Scan for the available devices
std::vector<std::string> GetDevicesIDs()
{
    char camid[MAXINDIDEVICE];
    int ret         = QHYCCD_ERROR;
    int deviceCount = 0;
    std::vector<std::string> devices;

#if defined(USE_SIMULATION)
    deviceCount = 2;
#else
    deviceCount = ScanQHYCCD();
#endif

    if (deviceCount > MAX_DEVICES)
    {
        deviceCount = MAX_DEVICES;
        IDLog("Devicescan found %d devices. The driver is compiled to support only up to %d devices.",
               deviceCount, MAX_DEVICES);
    }

    for (int i = 0; i < deviceCount; i++)
    {
        memset(camid, '\0', MAXINDIDEVICE);

#if defined(USE_SIMULATION)
        ret = QHYCCD_SUCCESS;
        snprintf(camid, MAXINDIDEVICE, "Model %d", i + 1);
#else
        ret = GetQHYCCDId(i, camid);
#endif
        if (ret == QHYCCD_SUCCESS)
        {
            devices.push_back(std::string(camid));
        }
        else
        {
            IDLog("#%d GetQHYCCDId error (%d)\n", i, ret);
        }
    }

    return devices;
}
}

void ISInit()
{
    static bool isInit = false;

    if (isInit)
        return;

    for (int i = 0; i < MAX_DEVICES; ++i)
        cameras[i] = nullptr;

#if !defined(USE_SIMULATION)
    int ret = InitQHYCCDResource();

    if (ret != QHYCCD_SUCCESS)
    {
        IDLog("Init QHYCCD SDK failed (%d)\n", ret);
        isInit = true;
        return;
    }
#endif

#if defined(OSX_EMBEDED_MODE)
    char firmwarePath[128];
    sprintf(firmwarePath, "%s/Contents/Resources", getenv("INDIPREFIX"));
    OSXInitQHYCCDFirmware(firmwarePath);
#elif defined(__APPLE__)
    char firmwarePath[128] = "/usr/local/lib/qhy";
    if (getenv("QHY_FIRMWARE_DIR") != NULL)
        strncpy(firmwarePath, getenv("QHY_FIRMWARE_DIR"), 128);
    OSXInitQHYCCDFirmware(firmwarePath);
#endif

    std::vector<std::string> devices = GetDevicesIDs();

    cameraCount = (int)devices.size();
    for (int i = 0; i < cameraCount; i++)
    {
        cameras[i] = new QHYCCD(devices[i].c_str());
    }
    if (cameraCount > 0)
    {
        atexit(QhyCCDCleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (cameraCount == 0)
    {
        IDMessage(nullptr, "No QHY cameras detected. Power on?");
        return;
    }

    for (int i = 0; i < cameraCount; i++)
    {
        QHYCCD *camera = cameras[i];
        if (dev == NULL || !strcmp(dev, camera->name))
        {
            camera->ISGetProperties(dev);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        QHYCCD *camera = cameras[i];
        if (dev == NULL || !strcmp(dev, camera->name))
        {
            camera->ISNewSwitch(dev, name, states, names, num);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        QHYCCD *camera = cameras[i];
        if (dev == NULL || !strcmp(dev, camera->name))
        {
            camera->ISNewText(dev, name, texts, names, num);
            if (dev != NULL)
                break;
        }
    }
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        QHYCCD *camera = cameras[i];
        if (dev == NULL || !strcmp(dev, camera->name))
        {
            camera->ISNewNumber(dev, name, values, names, num);
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

    for (int i = 0; i < cameraCount; i++)
    {
        QHYCCD *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

QHYCCD::QHYCCD(const char *name) : FilterInterface(this)
{
    // Filter Limits, can we call QHY API to find filter maximum?
    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 5;

    HasUSBTraffic = false;
    HasUSBSpeed   = false;
    HasGain       = false;
    HasOffset     = false;
    HasFilters    = false;
    coolerEnabled = false;

    snprintf(this->name, MAXINDINAME, "QHY CCD %.15s", name);
    snprintf(this->camid, MAXINDINAME, "%s", name);
    setDeviceName(this->name);

    setVersion(INDI_QHY_VERSION_MAJOR, INDI_QHY_VERSION_MINOR);

    sim = false;
}

QHYCCD::~QHYCCD()
{
    ReleaseQHYCCDResource();
}

const char *QHYCCD::getDefaultName()
{
    return ((char *)"QHY CCD");
}

bool QHYCCD::initProperties()
{
    INDI::CCD::initProperties();
    INDI::FilterInterface::initProperties(FILTER_TAB);

    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 9;

    // CCD Cooler Switch
    IUFillSwitch(&CoolerS[0], "COOLER_ON", "On", ISS_OFF);
    IUFillSwitch(&CoolerS[1], "COOLER_OFF", "Off", ISS_ON);
    IUFillSwitchVector(&CoolerSP, CoolerS, 2, getDeviceName(), "CCD_COOLER", "Cooler", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // CCD Regulation power
    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "Cooling Power (%)", "%+06.2f", 0., 1., .2, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooling Power", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    // CCD Gain
    IUFillNumber(&GainN[0], "GAIN", "Gain", "%3.0f", 0, 100, 1, 11);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "CCD_GAIN", "Gain", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    // CCD Offset
    IUFillNumber(&OffsetN[0], "Offset", "Offset", "%3.0f", 0, 0, 1, 0);
    IUFillNumberVector(&OffsetNP, OffsetN, 1, getDeviceName(), "CCD_OFFSET", "Offset", MAIN_CONTROL_TAB, IP_RW, 60,
                       IPS_IDLE);

    // USB Speed
    IUFillNumber(&SpeedN[0], "Speed", "Speed", "%3.0f", 0, 0, 1, 0);
    IUFillNumberVector(&SpeedNP, SpeedN, 1, getDeviceName(), "USB_SPEED", "USB Speed", MAIN_CONTROL_TAB, IP_RW, 60,
                       IPS_IDLE);

    // USB Traffic
    IUFillNumber(&USBTrafficN[0], "Speed", "Speed", "%3.0f", 0, 0, 1, 0);
    IUFillNumberVector(&USBTrafficNP, USBTrafficN, 1, getDeviceName(), "USB_TRAFFIC", "USB Traffic", MAIN_CONTROL_TAB,
                       IP_RW, 60, IPS_IDLE);

    // Set minimum exposure speed to 0.001 seconds
    //PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", MINIMUM_CCD_EXPOSURE, 3600, 1, false);

    addAuxControls();

    setDriverInterface(getDriverInterface() | FILTER_INTERFACE);

    return true;
}

void QHYCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    if (isConnected())
    {
        if (HasCooler())
        {
            defineSwitch(&CoolerSP);
            defineNumber(&CoolerNP);
        }

        if (HasUSBSpeed)
            defineNumber(&SpeedNP);

        if (HasGain)
            defineNumber(&GainNP);

        if (HasOffset)
            defineNumber(&OffsetNP);

        if (HasFilters)
        {
            //Define the Filter Slot and name properties
            defineNumber(&FilterSlotNP);
            if (FilterNameT != NULL)
                defineText(FilterNameTP);
        }

        if (HasUSBTraffic)
            defineNumber(&USBTrafficNP);
    }
}

bool QHYCCD::updateProperties()
{
    double min=0, max=0, step=0;

    // This must be executed BEFORE INDI::CCD::updateProperties()
    // Since Primary CCD Exposure will be defined in there so we have to get its limits now
    {
        // Exposure limits in microseconds
        int ret = GetQHYCCDParamMinMaxStep(camhandle, CONTROL_EXPOSURE, &min, &max, &step);
        if (ret == QHYCCD_SUCCESS)
            PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", min/1e6, max/1e6, step/1e6, false);
        else
            PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.001, 3600, 1, false);

            DEBUGF(INDI::Logger::DBG_DEBUG, "Exposure. Min: %g Max: %g Step %g", min, max, step);
    }

    // Define parent class properties
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        if (HasCooler())
        {
            defineSwitch(&CoolerSP);
            defineNumber(&CoolerNP);

            temperatureID = IEAddTimer(POLLMS, QHYCCD::updateTemperatureHelper, this);
        }

        if (HasUSBSpeed)
        {
            if (sim)
            {
                SpeedN[0].min   = 1;
                SpeedN[0].max   = 5;
                SpeedN[0].step  = 1;
                SpeedN[0].value = 1;
            }
            else
            {
                int ret = GetQHYCCDParamMinMaxStep(camhandle, CONTROL_SPEED, &min, &max, &step);
                if (ret == QHYCCD_SUCCESS)
                {
                    SpeedN[0].min  = min;
                    SpeedN[0].max  = max;
                    SpeedN[0].step = step;
                }

                SpeedN[0].value = GetQHYCCDParam(camhandle, CONTROL_SPEED);

                DEBUGF(INDI::Logger::DBG_DEBUG, "USB Speed. Value: %g Min: %g Max: %g Step %g", SpeedN[0].value,
                       SpeedN[0].min, SpeedN[0].max, SpeedN[0].step);
            }

            defineNumber(&SpeedNP);
        }

        if (HasGain)
        {
            if (sim)
            {
                GainN[0].min   = 0;
                GainN[0].max   = 100;
                GainN[0].step  = 10;
                GainN[0].value = 50;
            }
            else
            {
                int ret = GetQHYCCDParamMinMaxStep(camhandle, CONTROL_GAIN, &min, &max, &step);
                if (ret == QHYCCD_SUCCESS)
                {
                    GainN[0].min  = min;
                    GainN[0].max  = max;
                    GainN[0].step = step;
                }
                GainN[0].value = GetQHYCCDParam(camhandle, CONTROL_GAIN);

                DEBUGF(INDI::Logger::DBG_DEBUG, "Gain. Value: %g Min: %g Max: %g Step %g", GainN[0].value, GainN[0].min,
                       GainN[0].max, GainN[0].step);
            }

            defineNumber(&GainNP);
        }

        if (HasOffset)
        {
            if (sim)
            {
                OffsetN[0].min   = 1;
                OffsetN[0].max   = 10;
                OffsetN[0].step  = 1;
                OffsetN[0].value = 1;
            }
            else
            {
                int ret = GetQHYCCDParamMinMaxStep(camhandle, CONTROL_OFFSET, &min, &max, &step);
                if (ret == QHYCCD_SUCCESS)
                {
                    OffsetN[0].min  = min;
                    OffsetN[0].max  = max;
                    OffsetN[0].step = step;
                }
                OffsetN[0].value = GetQHYCCDParam(camhandle, CONTROL_OFFSET);

                DEBUGF(INDI::Logger::DBG_DEBUG, "Offset. Value: %g Min: %g Max: %g Step %g", OffsetN[0].value,
                       OffsetN[0].min, OffsetN[0].max, OffsetN[0].step);
            }

            //Define the Offset
            defineNumber(&OffsetNP);
        }

        if (HasFilters)
        {
            INDI::FilterInterface::updateProperties();
        }

        if (HasUSBTraffic)
        {
            if (sim)
            {
                USBTrafficN[0].min   = 1;
                USBTrafficN[0].max   = 100;
                USBTrafficN[0].step  = 5;
                USBTrafficN[0].value = 20;
            }
            else
            {
                int ret = GetQHYCCDParamMinMaxStep(camhandle, CONTROL_USBTRAFFIC, &min, &max, &step);
                if (ret == QHYCCD_SUCCESS)
                {
                    USBTrafficN[0].min  = min;
                    USBTrafficN[0].max  = max;
                    USBTrafficN[0].step = step;
                }
                USBTrafficN[0].value = GetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC);

                DEBUGF(INDI::Logger::DBG_DEBUG, "USB Traffic. Value: %g Min: %g Max: %g Step %g", USBTrafficN[0].value,
                       USBTrafficN[0].min, USBTrafficN[0].max, USBTrafficN[0].step);
            }
            defineNumber(&USBTrafficNP);
        }

        // Let's get parameters now from CCD
        setupParams();

        //timerID = SetTimer(POLLMS);
    }
    else
    {
        if (HasCooler())
        {
            deleteProperty(CoolerSP.name);
            deleteProperty(CoolerNP.name);
        }

        if (HasUSBSpeed)
        {
            deleteProperty(SpeedNP.name);
        }

        if (HasGain)
        {
            deleteProperty(GainNP.name);
        }

        if (HasOffset)
        {
            deleteProperty(OffsetNP.name);
        }

        if (HasFilters)
        {
            INDI::FilterInterface::updateProperties();
        }

        if (HasUSBTraffic)
            deleteProperty(USBTrafficNP.name);

        RemoveTimer(timerID);
    }

    return true;
}

bool QHYCCD::Connect()
{
    unsigned int ret = 0;
    uint32_t cap;

    sim = isSimulation();

    if (sim)
    {
        cap = CCD_CAN_SUBFRAME | CCD_CAN_ABORT | CCD_CAN_BIN | CCD_HAS_COOLER | CCD_HAS_ST4_PORT;
        SetCCDCapability(cap);

        HasUSBTraffic = true;
        HasUSBSpeed   = true;
        HasGain       = true;
        HasOffset     = true;
        HasFilters    = true;

        return true;
    }

    // Query the current CCD cameras. This method makes the driver more robust and
    // it fixes a crash with the new QHC SDK when the INDI driver reopens the same camera
    // with OpenQHYCCD() without a ScanQHYCCD() call before.
    std::vector<std::string> devices = GetDevicesIDs();

    // The CCD device is not available anymore
    if (std::find(devices.begin(), devices.end(), std::string(camid)) == devices.end())
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: Camera %s is not connected", camid);
        return false;
    }
    camhandle = OpenQHYCCD(camid);

    if (camhandle != NULL)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Connected to %s.", camid);

        cap = CCD_CAN_ABORT | CCD_CAN_SUBFRAME | CCD_HAS_STREAMING;

        // Disable the stream mode before connecting
        ret = SetQHYCCDStreamMode(camhandle, 0);
        if (ret != QHYCCD_SUCCESS)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: Can not disable stream mode (%d)", ret);
        }
        ret = InitQHYCCD(camhandle);
        if (ret != QHYCCD_SUCCESS)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: Init Camera failed (%d)", ret);
            return false;
        }

        ret = IsQHYCCDControlAvailable(camhandle, CAM_MECHANICALSHUTTER);
        if (ret == QHYCCD_SUCCESS)
        {
            cap |= CCD_HAS_SHUTTER;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Shutter Control: %s", cap & CCD_HAS_SHUTTER ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_COOLER);
        if (ret == QHYCCD_SUCCESS)
        {
            cap |= CCD_HAS_COOLER;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Cooler Control: %s", cap & CCD_HAS_COOLER ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_ST4PORT);
        if (ret == QHYCCD_SUCCESS)
        {
            cap |= CCD_HAS_ST4_PORT;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Guider Port Control: %s", cap & CCD_HAS_ST4_PORT ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_SPEED);
        if (ret == QHYCCD_SUCCESS)
        {
            HasUSBSpeed = true;

            // Force a certain speed on initialization of QHY5PII-C:
            // CONTROL_SPEED = 2 - Fastest, but the camera gets stuck with long exposure times.
            // CONTROL_SPEED = 1 - This is safe with the current driver.
            // CONTROL_SPEED = 0 - This is safe, but slower than 1.
            if (isQHY5PIIC())
                SetQHYCCDParam(camhandle, CONTROL_SPEED, 1);
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "USB Speed Control: %s", HasUSBSpeed ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_GAIN);
        if (ret == QHYCCD_SUCCESS)
        {
            HasGain = true;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Gain Control: %s", HasGain ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_OFFSET);
        if (ret == QHYCCD_SUCCESS)
        {
            HasOffset = true;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Offset Control: %s", HasOffset ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_CFWPORT);
        if (ret == QHYCCD_SUCCESS)
        {
            HasFilters = true;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Has Filters: %s", HasFilters ? "True" : "False");

        // Using software binning
        cap |= CCD_CAN_BIN;
        camxbin = 1;
        camybin = 1;

        // Always use INDI software binning
        //useSoftBin = true;


        ret = IsQHYCCDControlAvailable(camhandle,CAM_BIN1X1MODE);
        if(ret == QHYCCD_SUCCESS)
        {
            camxbin = 1;
            camybin = 1;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "Bin 1x1: %s", (ret == QHYCCD_SUCCESS) ? "True" : "False");

        ret &= IsQHYCCDControlAvailable(camhandle,CAM_BIN2X2MODE);
        ret &= IsQHYCCDControlAvailable(camhandle,CAM_BIN3X3MODE);
        ret &= IsQHYCCDControlAvailable(camhandle,CAM_BIN4X4MODE);

        // Only use software binning if NOT supported by hardware
        //useSoftBin = !(ret == QHYCCD_SUCCESS);

        DEBUGF(INDI::Logger::DBG_DEBUG, "Binning Control: %s", cap & CCD_CAN_BIN ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CONTROL_USBTRAFFIC);
        if (ret == QHYCCD_SUCCESS)
        {
            HasUSBTraffic = true;
            // Force the USB traffic value to 30 on initialization of QHY5PII-C otherwise
            // the camera has poor transfer speed.
            if (isQHY5PIIC())
                SetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC, 30);
        }

        DEBUGF(INDI::Logger::DBG_DEBUG, "USB Traffic Control: %s", HasUSBTraffic ? "True" : "False");

        ret = IsQHYCCDControlAvailable(camhandle, CAM_COLOR);
        //if(ret != QHYCCD_ERROR && ret != QHYCCD_ERROR_NOTSUPPORT)
        if (ret != QHYCCD_ERROR)
        {
            if (ret == BAYER_GB)
                IUSaveText(&BayerT[2], "GBRG");
            else if (ret == BAYER_GR)
                IUSaveText(&BayerT[2], "GRBG");
            else if (ret == BAYER_BG)
                IUSaveText(&BayerT[2], "BGGR");
            else
                IUSaveText(&BayerT[2], "RGGB");

            DEBUGF(INDI::Logger::DBG_DEBUG, "Color CCD: %s", BayerT[2].text);

            cap |= CCD_HAS_BAYER;
        }

        SetCCDCapability(cap);

        terminateThread = false;

        pthread_create(&primary_thread, NULL, &streamVideoHelper, this);
        return true;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Connecting to CCD failed (%s)", camid);

    return false;
}

bool QHYCCD::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "CCD is offline.");

    pthread_mutex_lock(&condMutex);
    streamPredicate = 0;
    terminateThread = true;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&condMutex);

    if (sim == false)
    {
        CloseQHYCCD(camhandle);
        //ReleaseQHYCCDResource();
    }

    return true;
}

bool QHYCCD::setupParams()
{
    uint32_t nbuf, ret, imagew, imageh, bpp;
    double chipw, chiph, pixelw, pixelh;

    if (sim)
    {
        chipw = imagew = 1280;
        chiph = imageh = 1024;
        pixelh = pixelw = 5.4;
        bpp             = 8;
    }
    else
    {
        ret = GetQHYCCDChipInfo(camhandle, &chipw, &chiph, &imagew, &imageh, &pixelw, &pixelh, &bpp);

        /* JM: We need GetQHYCCDErrorString(ret) to get the string description of the error, please implement this in the SDK */
        if (ret != QHYCCD_SUCCESS)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: GetQHYCCDChipInfo() (%d)", ret);
            return false;
        }

        DEBUGF(INDI::Logger::DBG_DEBUG,
               "GetQHYCCDChipInfo: chipW :%g chipH: %g imageW: %d imageH: %d pixelW: %g pixelH: %g bbp %d", chipw,
               chiph, imagew, imageh, pixelw, pixelh, bpp);

        camroix      = 0;
        camroiy      = 0;
        camroiwidth  = imagew;
        camroiheight = imageh;
    }

    SetCCDParams(imagew, imageh, bpp, pixelw, pixelh);

    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8;
    PrimaryCCD.setFrameBufferSize(nbuf);

    Streamer->setPixelFormat(INDI_MONO);
    Streamer->setSize(imagew, imageh);
    return true;
}

int QHYCCD::SetTemperature(double temperature)
{
    // If there difference, for example, is less than 0.1 degrees, let's immediately return OK.
    if (fabs(temperature - TemperatureN[0].value) < TEMP_THRESHOLD)
        return 1;

    DEBUGF(INDI::Logger::DBG_DEBUG, "SetTemperature %g", temperature);

    TemperatureRequest = temperature;

    // Enable cooler
    setCooler(true);

    // this muse be call every second to control
    //ControlQHYCCDTemp(camhandle,TemperatureRequest);

    return 0;
}

bool QHYCCD::StartExposure(float duration)
{
    unsigned int ret = QHYCCD_ERROR;

    if (Streamer->isBusy())
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot take exposure while streaming/recording is active.");
        return false;
    }
    //AbortPrimaryFrame = false;

    /*
    if (duration < MINIMUM_CCD_EXPOSURE)
    {
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Exposure shorter than minimum duration %g s requested. Setting exposure time to %g s.", duration,
               MINIMUM_CCD_EXPOSURE);
        duration = MINIMUM_CCD_EXPOSURE;
    }*/

    imageFrameType = PrimaryCCD.getFrameType();

    /*if (imageFrameType == CCDChip::BIAS_FRAME)
    {
        duration = MINIMUM_CCD_EXPOSURE;
        DEBUGF(INDI::Logger::DBG_SESSION, "Bias Frame (s) : %g", duration);
    }
    else*/
    if (GetCCDCapability() & CCD_HAS_SHUTTER)
    {
        if (imageFrameType == INDI::CCDChip::DARK_FRAME || imageFrameType == INDI::CCDChip::BIAS_FRAME)
            ControlQHYCCDShutter(camhandle, MACHANICALSHUTTER_CLOSE);
        else
            ControlQHYCCDShutter(camhandle, MACHANICALSHUTTER_FREE);
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Current exposure time is %f us", duration * 1000 * 1000);
    ExposureRequest = duration;
    PrimaryCCD.setExposureDuration(duration);

    if (sim)
        ret = QHYCCD_SUCCESS;
    else
    {
        if (LastExposureRequest != ExposureRequest)
        {
            ret = SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, ExposureRequest * 1000 * 1000);

            if (ret != QHYCCD_SUCCESS)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Set expose time failed (%d).", ret);
                return false;
            }

            LastExposureRequest = ExposureRequest;
        }
    }

    if (sim)
        ret = QHYCCD_SUCCESS;
    else
        ret = SetQHYCCDBinMode(camhandle, camxbin, camybin);
    if (ret != QHYCCD_SUCCESS)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Set QHYCCD Bin mode failed (%d)", ret);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "SetQHYCCDBinMode (%dx%d).", camxbin, camybin);

    if (sim)
        ret = QHYCCD_SUCCESS;
    else
        ret = SetQHYCCDResolution(camhandle, camroix, camroiy, camroiwidth, camroiheight);
    if (ret != QHYCCD_SUCCESS)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Set QHYCCD ROI resolution (%d,%d) (%d,%d) failed (%d)", camroix, camroiy,
               camroiwidth, camroiheight, ret);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "SetQHYCCDResolution camroix %d camroiy %d camroiwidth %d camroiheight %d", camroix,
           camroiy, camroiwidth, camroiheight);

    if (sim)
        ret = QHYCCD_SUCCESS;
    else
        ret = ExpQHYCCDSingleFrame(camhandle);
    if (ret == QHYCCD_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Begin QHYCCD expose failed (%d)", ret);
        return false;
    }

    gettimeofday(&ExpStart, NULL);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;

    // if (ExposureRequest*1000 < POLLMS)
    //     SetTimer(ExposureRequest*1000);
    // else
    SetTimer(POLLMS);

    return true;
}

bool QHYCCD::AbortExposure()
{
    int ret;

    if (!InExposure || sim)
    {
        InExposure = false;
        return true;
    }

    ret = CancelQHYCCDExposingAndReadout(camhandle);
    if (ret == QHYCCD_SUCCESS)
    {
        //AbortPrimaryFrame = true;
        InExposure = false;
        DEBUG(INDI::Logger::DBG_SESSION, "Exposure aborted.");
        return true;
    }
    else
        DEBUGF(INDI::Logger::DBG_ERROR, "Abort exposure failed (%d)", ret);

    return false;
}

bool QHYCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    /*
  long x_1 = x / PrimaryCCD.getBinX();
  long y_1 = y / PrimaryCCD.getBinY();

  long x_2 = x_1 + (w / PrimaryCCD.getBinX());
  long y_2 = y_1 + (h / PrimaryCCD.getBinY());

  if (x_2 > PrimaryCCD.getXRes() / PrimaryCCD.getBinX())
  {
      DEBUGF(INDI::Logger::DBG_ERROR, "Error: invalid width requested %ld", x_2);
      return false;
  }
  else if (y_2 > PrimaryCCD.getYRes() / PrimaryCCD.getBinY())
  {
      DEBUGF(INDI::Logger::DBG_ERROR, "Error: invalid height request %ld", y_2);
      return false;
  }*/

    camroix      = x / PrimaryCCD.getBinX();
    camroiy      = y / PrimaryCCD.getBinY();
    camroiwidth  = w / PrimaryCCD.getBinX();
    camroiheight = h / PrimaryCCD.getBinY();

    DEBUGF(INDI::Logger::DBG_DEBUG, "Final binned (%dx%d) image area is (%d, %d), (%d, %d)", PrimaryCCD.getBinX(), PrimaryCCD.getBinY(),
           camroix, camroiy, camroiwidth, camroiheight);

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x, y, w, h);
    // Total bytes required for image buffer
    uint32_t nbuf = (camroiwidth * camroiheight * PrimaryCCD.getBPP() / 8);
    PrimaryCCD.setFrameBufferSize(nbuf);

    // Streamer is always updated with BINNED size.
    Streamer->setSize(camroiwidth, camroiheight);
    return true;
}

bool QHYCCD::UpdateCCDBin(int hor, int ver)
{
    int ret = QHYCCD_ERROR;

    if (hor != ver)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid binning mode. Asymmetrical binning not supported.");
        return false;
    }

    if (hor == 3)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Invalid binning mode. Only 1x1, 2x2, and 4x4 binning modes supported.");
        return false;
    }

    if (hor > 1 && GetCCDCapability() & CCD_HAS_BAYER)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Binning not supported for color CCDs.");
        return false;
    }

    /*if (useSoftBin)
        ret = QHYCCD_SUCCESS;
    else */
    if (hor == 1 && ver == 1)
    {
        ret = IsQHYCCDControlAvailable(camhandle, CAM_BIN1X1MODE);
    }
    else if (hor == 2 && ver == 2)
    {
        ret = IsQHYCCDControlAvailable(camhandle, CAM_BIN2X2MODE);
    }
    else if (hor == 3 && ver == 3)
    {
        ret = IsQHYCCDControlAvailable(camhandle, CAM_BIN3X3MODE);
    }
    else if (hor == 4 && ver == 4)
    {
        ret = IsQHYCCDControlAvailable(camhandle, CAM_BIN4X4MODE);
    }

    // Binning ALWAYS succeeds
#if 0
    if (ret != QHYCCD_SUCCESS)
    {
        useSoftBin = true;
    }

    // We always use software binning so QHY binning is always at 1x1
    camxbin = 1;
    camybin = 1;
#endif

    if (ret != QHYCCD_SUCCESS)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%dx%d binning is not supported.", hor, ver);
        return false;
    }

    camxbin = hor;
    camybin = ver;

    PrimaryCCD.setBin(hor, ver);

    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

float QHYCCD::calcTimeLeft()
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

/* Downloads the image from the CCD. */
int QHYCCD::grabImage()
{
    if (sim)
    {
        unsigned char *image = (unsigned char *)PrimaryCCD.getFrameBuffer();
        int width            = PrimaryCCD.getSubW() / PrimaryCCD.getBinX() * PrimaryCCD.getBPP() / 8;
        int height           = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

        for (int i = 0; i < height; i++)
            for (int j = 0; j < width; j++)
                image[i * width + j] = rand() % 255;
    }
    else
    {
        uint32_t ret, w, h, bpp, channels;

        DEBUG(INDI::Logger::DBG_DEBUG, "Blocking read call.");
        ret = GetQHYCCDSingleFrame(camhandle, &w, &h, &bpp, &channels, PrimaryCCD.getFrameBuffer());
        DEBUG(INDI::Logger::DBG_DEBUG, "Blocking read call finished.");

        if (ret != QHYCCD_SUCCESS)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "GetQHYCCDSingleFrame error (%d)", ret);
            PrimaryCCD.setExposureFailed();
            return -1;
        }
    }

    // Perform software binning if necessary
    //if (useSoftBin)
    //    PrimaryCCD.binFrame();

    DEBUG(INDI::Logger::DBG_DEBUG, "Download complete.");
    if (ExposureRequest > POLLMS * 5)
        DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    ExposureComplete(&PrimaryCCD);

    return 0;
}

void QHYCCD::TimerHit()
{
    if (isConnected() == false)
        return;

    if (InExposure)
    {
        long timeleft = calcTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    SetTimer(50);
                }
                else
                {
                    /* We're done exposing */
                    DEBUG(INDI::Logger::DBG_DEBUG, "Exposure done, downloading image...");
                    // Don't spam the session log unless it is a long exposure > 5 seconds
                    if (ExposureRequest > POLLMS * 5)
                        DEBUG(INDI::Logger::DBG_SESSION, "Exposure done, downloading image...");

                    PrimaryCCD.setExposureLeft(0);
                    InExposure = false;
                    /* grab and save image */
                    grabImage();
                }
            }
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Exposure in progress: Time left %ld", timeleft);
            SetTimer(POLLMS);
        }

        PrimaryCCD.setExposureLeft(timeleft);
    }
}

IPState QHYCCD::GuideNorth(float duration)
{
    ControlQHYCCDGuide(camhandle, 1, duration);
    return IPS_OK;
}

IPState QHYCCD::GuideSouth(float duration)
{
    ControlQHYCCDGuide(camhandle, 2, duration);
    return IPS_OK;
}

IPState QHYCCD::GuideEast(float duration)
{
    ControlQHYCCDGuide(camhandle, 0, duration);
    return IPS_OK;
}

IPState QHYCCD::GuideWest(float duration)
{
    ControlQHYCCDGuide(camhandle, 3, duration);
    return IPS_OK;
}

bool QHYCCD::SelectFilter(int position)
{
    char targetpos = 0;
    char currentpos[64];
    int checktimes = 0;
    int ret = 0;

    if (sim)
        ret = QHYCCD_SUCCESS;
    else
    {
        // JM: THIS WILL CRASH! I am using another method with same result!
        //sprintf(&targetpos,"%d",position - 1);
        targetpos = '0' + (position - 1);
        ret       = SendOrder2QHYCCDCFW(camhandle, &targetpos, 1);
    }

    if (ret == QHYCCD_SUCCESS)
    {
        while (checktimes < 90)
        {
            ret = GetQHYCCDCFWStatus(camhandle, currentpos);
            if (ret == QHYCCD_SUCCESS)
            {
                if ((targetpos + 1) == currentpos[0])
                {
                    break;
                }
            }
            checktimes++;
        }

        CurrentFilter = position;
        SelectFilterDone(position);
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s: Filter changed to %d", camid, position);
        return true;
    }
    else
        DEBUGF(INDI::Logger::DBG_ERROR, "Changing filter failed (%d)", ret);

    return false;
}

int QHYCCD::QueryFilter()
{
    return CurrentFilter;
}

bool QHYCCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, CoolerSP.name) == 0)
        {
            IUUpdateSwitch(&CoolerSP, states, names, n);

            setCooler(CoolerS[0].s == ISS_ON);

            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool QHYCCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //  This is for our device
        //  Now lets see if it's something we process here
        if (strcmp(name, FilterNameTP->name) == 0)
        {
            INDI::FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool QHYCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    //IDLog("INDI::CCD::ISNewNumber %s\n",name);
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FilterSlotNP.name) == 0)
        {
            INDI::FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }

        if (strcmp(name, GainNP.name) == 0)
        {
            IUUpdateNumber(&GainNP, values, names, n);
            GainRequest = GainN[0].value;
            if (LastGainRequest != GainRequest)
            {
                SetQHYCCDParam(camhandle, CONTROL_GAIN, GainN[0].value);
                LastGainRequest = GainRequest;
            }
            DEBUGF(INDI::Logger::DBG_SESSION, "Current %s value %f", GainNP.name, GainN[0].value);
            GainNP.s = IPS_OK;
            IDSetNumber(&GainNP, NULL);
            return true;
        }

        if (strcmp(name, OffsetNP.name) == 0)
        {
            IUUpdateNumber(&OffsetNP, values, names, n);
            SetQHYCCDParam(camhandle, CONTROL_OFFSET, OffsetN[0].value);
            DEBUGF(INDI::Logger::DBG_SESSION, "Current %s value %f", OffsetNP.name, OffsetN[0].value);
            OffsetNP.s = IPS_OK;
            IDSetNumber(&OffsetNP, NULL);
            saveConfig(true, OffsetNP.name);
            return true;
        }

        if (strcmp(name, SpeedNP.name) == 0)
        {
            IUUpdateNumber(&SpeedNP, values, names, n);
            SetQHYCCDParam(camhandle, CONTROL_SPEED, SpeedN[0].value);
            DEBUGF(INDI::Logger::DBG_SESSION, "Current %s value %f", SpeedNP.name, SpeedN[0].value);
            SpeedNP.s = IPS_OK;
            IDSetNumber(&SpeedNP, NULL);
            saveConfig(true, SpeedNP.name);
            return true;
        }

        if (strcmp(name, USBTrafficNP.name) == 0)
        {
            IUUpdateNumber(&USBTrafficNP, values, names, n);
            SetQHYCCDParam(camhandle, CONTROL_USBTRAFFIC, USBTrafficN[0].value);
            DEBUGF(INDI::Logger::DBG_SESSION, "Current %s value %f", USBTrafficNP.name, USBTrafficN[0].value);
            USBTrafficNP.s = IPS_OK;
            IDSetNumber(&USBTrafficNP, NULL);
            saveConfig(true, USBTrafficNP.name);
            return true;
        }
    }
    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

void QHYCCD::setCooler(bool enable)
{
    if (enable && coolerEnabled == false)
    {
        CoolerS[0].s = ISS_ON;
        CoolerS[1].s = ISS_OFF;
        CoolerSP.s   = IPS_OK;
        IDSetSwitch(&CoolerSP, NULL);

        CoolerNP.s = IPS_BUSY;
        IDSetNumber(&CoolerNP, NULL);
        DEBUG(INDI::Logger::DBG_SESSION, "Cooler on.");

        coolerEnabled = true;
    }
    else if (!enable && coolerEnabled == true)
    {
        coolerEnabled = false;

        if (sim == false)
            SetQHYCCDParam(camhandle, CONTROL_MANULPWM, 0);

        CoolerSP.s   = IPS_IDLE;
        CoolerS[0].s = ISS_OFF;
        CoolerS[1].s = ISS_ON;
        IDSetSwitch(&CoolerSP, NULL);

        CoolerNP.s = IPS_IDLE;
        IDSetNumber(&CoolerNP, NULL);

        TemperatureNP.s = IPS_IDLE;
        IDSetNumber(&TemperatureNP, NULL);
        DEBUG(INDI::Logger::DBG_SESSION, "Cooler off.");
    }
}

bool QHYCCD::isQHY5PIIC()
{
    return std::string(camid, 9) == "QHY5PII-C";
}

void QHYCCD::updateTemperatureHelper(void *p)
{
    if (static_cast<QHYCCD *>(p)->isConnected())
        static_cast<QHYCCD *>(p)->updateTemperature();
}

void QHYCCD::updateTemperature()
{
    double ccdtemp = 0, coolpower = 0;
    double nextPoll = POLLMS;

    if (sim)
    {
        ccdtemp = TemperatureN[0].value;
        if (TemperatureN[0].value < TemperatureRequest)
            ccdtemp += TEMP_THRESHOLD;
        else if (TemperatureN[0].value > TemperatureRequest)
            ccdtemp -= TEMP_THRESHOLD;

        coolpower = 128;
    }
    else
    {
        ccdtemp   = GetQHYCCDParam(camhandle, CONTROL_CURTEMP);
        coolpower = GetQHYCCDParam(camhandle, CONTROL_CURPWM);
        ControlQHYCCDTemp(camhandle, TemperatureRequest);
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CCD Temp: %g CCD RAW Cooling Power: %g, CCD Cooling percentage: %g", ccdtemp,
           coolpower, coolpower / 255.0 * 100);

    TemperatureN[0].value = ccdtemp;
    CoolerN[0].value      = coolpower / 255.0 * 100;

    if (coolpower > 0 && CoolerS[0].s == ISS_OFF)
    {
        CoolerNP.s   = IPS_BUSY;
        CoolerSP.s   = IPS_OK;
        CoolerS[0].s = ISS_ON;
        CoolerS[1].s = ISS_OFF;
        IDSetSwitch(&CoolerSP, NULL);
    }
    else if (coolpower <= 0 && CoolerS[0].s == ISS_ON)
    {
        CoolerNP.s   = IPS_IDLE;
        CoolerSP.s   = IPS_IDLE;
        CoolerS[0].s = ISS_OFF;
        CoolerS[1].s = ISS_ON;
        IDSetSwitch(&CoolerSP, NULL);
    }

    if (TemperatureNP.s == IPS_BUSY && fabs(TemperatureN[0].value - TemperatureRequest) <= TEMP_THRESHOLD)
    {
        TemperatureN[0].value = TemperatureRequest;
        TemperatureNP.s       = IPS_OK;
    }

    /*
    //we need call ControlQHYCCDTemp every second to control temperature
    if (TemperatureNP.s == IPS_BUSY)
        nextPoll = TEMPERATURE_BUSY_MS;
*/

    IDSetNumber(&TemperatureNP, NULL);
    IDSetNumber(&CoolerNP, NULL);

    temperatureID = IEAddTimer(nextPoll, QHYCCD::updateTemperatureHelper, this);
}

bool QHYCCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);

    if (HasFilters)
    {
        INDI::FilterInterface::saveConfigItems(fp);
    }

    if (HasGain)
        IUSaveConfigNumber(fp, &GainNP);

    if (HasOffset)
        IUSaveConfigNumber(fp, &OffsetNP);

    if (HasUSBSpeed)
        IUSaveConfigNumber(fp, &SpeedNP);

    if (HasUSBTraffic)
        IUSaveConfigNumber(fp, &USBTrafficNP);

    return true;
}

bool QHYCCD::StartStreaming()
{
    /*if (PrimaryCCD.getBPP() != 8)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Streaming only supported for 8 bit images.");
        return false;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "Starting streaming with a period of %g seconds.",
           PrimaryCCD.getExposureDuration());*/

    ExposureRequest = 1.0 / Streamer->getTargetFPS();

    // N.B. There is no corresponding value for GBGR. It is odd that QHY selects this as the default as no one seems to process it
    const std::map<const char *, INDI_PIXEL_FORMAT> formats = { { "GBGR", INDI_MONO },
                                                                { "GRGB", INDI_BAYER_GRBG },
                                                                { "BGGR", INDI_BAYER_BGGR },
                                                                { "RGGB", INDI_BAYER_RGGB } };

    INDI_PIXEL_FORMAT qhyFormat = INDI_MONO;
    if (BayerT[2].text && formats.count(BayerT[2].text) != 0)
        qhyFormat = formats.at(BayerT[2].text);

    Streamer->setPixelFormat(qhyFormat, PrimaryCCD.getBPP());

    SetQHYCCDParam(camhandle, CONTROL_EXPOSURE, ExposureRequest * 1000 * 1000);
    SetQHYCCDStreamMode(camhandle, 0x01);
    BeginQHYCCDLive(camhandle);
    pthread_mutex_lock(&condMutex);
    streamPredicate = 1;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);

    return true;
}

bool QHYCCD::StopStreaming()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Streaming disabled.");

    pthread_mutex_lock(&condMutex);
    streamPredicate = 0;
    pthread_mutex_unlock(&condMutex);
    pthread_cond_signal(&cv);
    SetQHYCCDStreamMode(camhandle, 0x0);
    StopQHYCCDLive(camhandle);

    return true;
}

void *QHYCCD::streamVideoHelper(void *context)
{
    return ((QHYCCD *)context)->streamVideo();
}

void *QHYCCD::streamVideo()
{
    uint32_t ret = 0, w, h, bpp, channels;

    while (true)
    {
        pthread_mutex_lock(&condMutex);

        while (streamPredicate == 0)
            pthread_cond_wait(&cv, &condMutex);

        if (terminateThread)
            break;

        // release condMutex
        pthread_mutex_unlock(&condMutex);

        if ((ret = GetQHYCCDLiveFrame(camhandle, &w, &h, &bpp, &channels, PrimaryCCD.getFrameBuffer())) ==
            QHYCCD_SUCCESS)
            Streamer->newFrame(PrimaryCCD.getFrameBuffer(), PrimaryCCD.getFrameBufferSize());
    }

    pthread_mutex_unlock(&condMutex);
    return 0;
}

void QHYCCD::debugTriggered(bool enable)
{
    if (enable)
        SetQHYCCDLogLevel(LOG_LEVEL_DEBUG);
    else
        SetQHYCCDLogLevel(LOG_LEVEL_ERROR);
}
