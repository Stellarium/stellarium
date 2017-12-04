/*
 Fishcamp INDI CCD Driver
 Copyright (C) 2013 Jasem Mutlaq (mutlaqja@ikarustech.com)

 Multiple device support Copyright (C) 2013 Peter Polakovic (peter.polakovic@cloudmakers.eu)

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

#include <memory>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include <indidevapi.h>
#include <eventloop.h>

#include "indi_fishcamp.h"
#include "config.h"

#define MAX_CCD_TEMP   45   /* Max CCD temperature */
#define MIN_CCD_TEMP   -55  /* Min CCD temperature */
#define MAX_X_BIN      16   /* Max Horizontal binning */
#define MAX_Y_BIN      16   /* Max Vertical binning */
#define MAX_PIXELS     4096 /* Max number of pixels in one dimension */
#define POLLMS         1000 /* Polling time (ms) */
#define TEMP_THRESHOLD .25  /* Differential temperature threshold (C)*/
#define MAX_DEVICES    20   /* Max device cameraCount */

static int cameraCount;
static FishCampCCD *cameras[MAX_DEVICES];

static void cleanup()
{
    for (int i = 0; i < cameraCount; i++)
    {
        delete cameras[i];
    }
}

void ISInit()
{
    static bool isInit = false;
    if (!isInit)
    {
        // initialize the driver framework
        IDLog("About to call fcUsb_init()\n");
        fcUsb_init();

        IDLog("About to call set logging\n");
        fcUsb_setLogging(true);

        IDLog("About to call find Cameras\n");
        cameraCount = fcUsb_FindCameras();

        IDLog("Found %d fishcamp cameras.\n", cameraCount);

        for (int i = 0; i < cameraCount; i++)
            cameras[i] = new FishCampCCD(i + 1);

        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (cameraCount == 0)
    {
        IDLog("Unable to find Fishcamp CCD. Power OK? Please check the log file under ~/.indi for more details.\n");
        return;
    }

    for (int i = 0; i < cameraCount; i++)
    {
        FishCampCCD *camera = cameras[i];
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
        FishCampCCD *camera = cameras[i];
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
        FishCampCCD *camera = cameras[i];
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
        FishCampCCD *camera = cameras[i];
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
    INDI_UNUSED(root);
}

FishCampCCD::FishCampCCD(int CamNum)
{
    cameraNum = CamNum;

    int rc = fcUsb_OpenCamera(cameraNum);

    IDLog("fcUsb_OpenCamera opening cam #%d, returns %d\n", cameraNum, rc);

    rc = fcUsb_cmd_getinfo(cameraNum, &camInfo);

    IDLog("fcUsb_cmd_getinfo opening cam #%d, returns %d\n", cameraNum, rc);

    strncpy(name, (char *)&camInfo.camNameStr, MAXINDINAME);

    IDLog("Cam #%d with name %s\n", cameraNum, name);

    setDeviceName(name);

    setVersion(FISHCAMP_VERSION_MAJOR, FISHCAMP_VERSION_MINOR);

    sim = false;
}

FishCampCCD::~FishCampCCD()
{
    fcUsb_CloseCamera(cameraNum);
}

const char *FishCampCCD::getDefaultName()
{
    return "Starfish CCD";
}

bool FishCampCCD::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    IUFillNumber(&GainN[0], "Range", "", "%g", 1, 15, 1., 4.);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "Gain", "", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&CoolerN[0], "Power %", "", "%g", 1, 100, 0, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "Cooler", "", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    char *strBuf = new char[MAXINDINAME];

    IUFillText(&CamInfoT[0], "Name", "", name);
    IUFillText(&CamInfoT[1], "Serial #", "", (char *)&camInfo.camSerialStr);

    snprintf(strBuf, MAXINDINAME, "%d", camInfo.boardVersion);
    IUFillText(&CamInfoT[2], "Board version", "", strBuf);

    snprintf(strBuf, MAXINDINAME, "%d", camInfo.boardRevision);
    IUFillText(&CamInfoT[3], "Board revision", "", strBuf);

    snprintf(strBuf, MAXINDINAME, "%d", camInfo.fpgaVersion);
    IUFillText(&CamInfoT[4], "FPGA version", "", strBuf);

    snprintf(strBuf, MAXINDINAME, "%d", camInfo.fpgaRevision);
    IUFillText(&CamInfoT[5], "FPGA revision", "", strBuf);

    IUFillTextVector(&CamInfoTP, CamInfoT, 6, getDeviceName(), "Camera Info", "", MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    SetCCDParams(camInfo.width, camInfo.height, 16, camInfo.pixelWidth / 10.0, camInfo.pixelHeight / 10.0);

    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel cameraCount
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    SetCCDCapability(CCD_CAN_ABORT | CCD_CAN_SUBFRAME | CCD_HAS_COOLER);

    delete[] strBuf;

    return true;
}

void FishCampCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();
}

bool FishCampCCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        defineText(&CamInfoTP);
        defineNumber(&CoolerNP);
        defineNumber(&GainNP);

        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(CamInfoTP.name);
        deleteProperty(CoolerNP.name);
        deleteProperty(GainNP.name);

        rmTimer(timerID);
    }

    return true;
}

bool FishCampCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, GainNP.name))
        {
            IUUpdateNumber(&GainNP, values, names, n);
            setGain(GainN[0].value);
            GainNP.s = IPS_OK;
            IDSetNumber(&GainNP, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool FishCampCCD::setGain(double gain)
{
    int rc = fcUsb_cmd_setCameraGain(cameraNum, ((int)gain));

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_setCameraGain returns %d", rc);

    return true;
}

int FishCampCCD::SetTemperature(double temperature)
{
    TemperatureRequest = temperature;

    int rc = fcUsb_cmd_setTemperature(cameraNum, TemperatureRequest);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_setTemperature returns %d", rc);

    if (fcUsb_cmd_getTECInPowerOK(cameraNum))
        CoolerNP.s = IPS_OK;
    else
        CoolerNP.s = IPS_IDLE;

    TemperatureNP.s = IPS_BUSY;
    IDSetNumber(&TemperatureNP, NULL);

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting CCD temperature to %+06.2f C", temperature);

    return 0;
}

void FishCampCCD::simulationTriggered(bool enable)
{
    sim = enable;
    fcUsb_setSimulation(enable);
}

bool FishCampCCD::Connect()
{
    sim = isSimulation();

    if (sim)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Simulated Fishcamp is online.");
        return true;
    }

    if (fcUsb_haveCamera())
    {
        fcUsb_cmd_setReadMode(cameraNum, fc_classicDataXfr, fc_16b_data);
        fcUsb_cmd_setCameraGain(cameraNum, GainN[0].value);
        fcUsb_cmd_setRoi(cameraNum, 0, 0, camInfo.width - 1, camInfo.height - 1);
        if (fcUsb_cmd_getTECInPowerOK(cameraNum))
            CoolerNP.s = IPS_OK;
        DEBUG(INDI::Logger::DBG_SESSION, "Fishcamp CCD is online.");
        return true;
    }
    else
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot find Fishcamp CCD. Please check the logfile and try again.");
        return false;
    }
}

bool FishCampCCD::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Fishcamp CCD is offline.");

    if (sim)
        return true;

    fcUsb_CloseCamera(cameraNum);

    return true;
}

bool FishCampCCD::StartExposure(float duration)
{
    PrimaryCCD.setExposureDuration(duration);
    ExposureRequest = duration;

    bool rc = false;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Exposure Time (s) is: %g", duration);

    // setup the exposure time in ms.
    rc = fcUsb_cmd_setIntegrationTime(cameraNum, duration);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_setIntegrationTime returns %d", rc);

    rc = fcUsb_cmd_startExposure(cameraNum);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_startExposure returns %d", rc);

    gettimeofday(&ExpStart, NULL);

    DEBUGF(INDI::Logger::DBG_SESSION, "Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;

    return (rc == 0);
}

bool FishCampCCD::AbortExposure()
{
    int rc = 0;

    rc = fcUsb_cmd_abortExposure(cameraNum);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_abortExposure returns %d", rc);

    InExposure = false;
    return true;
}

bool FishCampCCD::UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType)
{
    // We only support light frames
    if (fType != INDI::CCDChip::LIGHT_FRAME)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Only light frames are supported in this camera.");
        return false;
    }

    PrimaryCCD.setFrameType(fType);
    return true;
}

bool FishCampCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    int rc = 0;
    /* Add the X and Y offsets */
    long x_1 = x;
    long y_1 = y;

    long bin_width  = x_1 + (w / PrimaryCCD.getBinX());
    long bin_height = y_1 + (h / PrimaryCCD.getBinY());

    if (bin_width > PrimaryCCD.getXRes() / PrimaryCCD.getBinX())
    {
        IDMessage(getDeviceName(), "Error: invalid width requested %d", w);
        return false;
    }
    else if (bin_height > PrimaryCCD.getYRes() / PrimaryCCD.getBinY())
    {
        IDMessage(getDeviceName(), "Error: invalid height request %d", h);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "The Final image area is (%ld, %ld), (%ld, %ld)\n", x_1, y_1, bin_width,
           bin_height);

    rc = fcUsb_cmd_setRoi(cameraNum, x_1, y_1, w - 1, h - 1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_setRoi returns %d", rc);

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x_1, y_1, w, h);

    int nbuf;
    nbuf = (bin_width * bin_height * PrimaryCCD.getBPP() / 8); //  this is pixel count
    nbuf += 512;                                               //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Setting frame buffer size to %d bytes.\n", nbuf);

    return true;
}

bool FishCampCCD::UpdateCCDBin(int binx, int biny)
{
    if (binx != 1 || biny != 1)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Camera currently does not support binning.");
        return false;
    }

    return true;
}

float FishCampCCD::CalcTimeLeft()
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

/* Downloads the image from the CCD.*/
int FishCampCCD::grabImage()
{
    uint8_t *image = PrimaryCCD.getFrameBuffer();

    UInt16 *frameBuffer = (UInt16 *)image;
    fcUsb_cmd_getRawFrame(cameraNum, PrimaryCCD.getSubW(), PrimaryCCD.getSubH(), frameBuffer);
    DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    ExposureComplete(&PrimaryCCD);

    return 0;
}

void FishCampCCD::TimerHit()
{
    int timerHitID = -1, state = -1, rc = -1;
    long timeleft;
    double ccdTemp;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        timeleft = CalcTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                timerHitID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07)
                {
                    //  use an even tighter timer
                    timerHitID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    while (!sim && timeleft > 0)
                    {
                        state = fcUsb_cmd_getState(cameraNum);
                        if (state == 0)
                            timeleft = 0;

                        int slv;
                        slv = 100000 * timeleft;
                        usleep(slv);
                    }

                    /* We're done exposing */
                    DEBUG(INDI::Logger::DBG_DEBUG, "Exposure done, downloading image...");

                    PrimaryCCD.setExposureLeft(0);
                    InExposure = false;
                    /* grab and save image */
                    grabImage();
                }
            }
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Image not yet ready. With time left %ld\n", timeleft);
        }

        PrimaryCCD.setExposureLeft(timeleft);
    }

    switch (TemperatureNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            rc = fcUsb_cmd_getTemperature(cameraNum);

            DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_getTemperature returns %d", rc);

            ccdTemp = rc / 100.0;

            DEBUGF(INDI::Logger::DBG_DEBUG, "Temperature %g", ccdTemp);

            if (fabs(TemperatureN[0].value - ccdTemp) >= TEMP_THRESHOLD)
            {
                TemperatureN[0].value = ccdTemp;
                IDSetNumber(&TemperatureNP, NULL);
            }

            break;

        case IPS_BUSY:
            if (sim)
            {
                TemperatureN[0].value = TemperatureRequest;
            }
            else
            {
                rc = fcUsb_cmd_getTemperature(cameraNum);

                DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_getTemperature returns %d", rc);

                TemperatureN[0].value = rc / 100.0;
            }

            // If we're within threshold, let's make it BUSY ---> OK
            if (fabs(TemperatureRequest - TemperatureN[0].value) <= TEMP_THRESHOLD)
                TemperatureNP.s = IPS_OK;

            IDSetNumber(&TemperatureNP, NULL);
            break;

        case IPS_ALERT:
            break;
    }

    switch (CoolerNP.s)
    {
        case IPS_OK:
            CoolerN[0].value = fcUsb_cmd_getTECPowerLevel(cameraNum);
            IDSetNumber(&CoolerNP, NULL);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Cooler power level %g %", CoolerN[0].value);
            break;

        default:
            break;
    }

    if (timerHitID == -1)
        SetTimer(POLLMS);
    return;
}

IPState FishCampCCD::GuideNorth(float duration)
{
    if (sim)
        return IPS_OK;

    int rc = 0;

    rc = fcUsb_cmd_pulseRelay(cameraNum, fcRELAYNORTH, duration, 0, false);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_pulseRelay fcRELAYNORTH returns %d", rc);

    return IPS_OK;
}

IPState FishCampCCD::GuideSouth(float duration)
{
    if (sim)
        return IPS_OK;

    int rc = 0;

    rc = fcUsb_cmd_pulseRelay(cameraNum, fcRELAYSOUTH, duration, 0, false);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_pulseRelay fcRELAYSOUTH returns %d", rc);

    return IPS_OK;
}

IPState FishCampCCD::GuideEast(float duration)
{
    if (sim)
        return IPS_OK;

    int rc = 0;

    rc = fcUsb_cmd_pulseRelay(cameraNum, fcRELAYEAST, duration, 0, false);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_pulseRelay fcRELAYEAST returns %d", rc);

    return IPS_OK;
}

IPState FishCampCCD::GuideWest(float duration)
{
    if (sim)
        return IPS_OK;

    int rc = 0;

    rc = fcUsb_cmd_pulseRelay(cameraNum, fcRELAYWEST, duration, 0, false);

    DEBUGF(INDI::Logger::DBG_DEBUG, "fcUsb_cmd_pulseRelay fcRELAYWEST returns %d", rc);

    return IPS_OK;
}
