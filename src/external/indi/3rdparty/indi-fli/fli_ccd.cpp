/*
    FLI CCD
    INDI Interface for Finger Lakes Instrument CCDs
    Copyright (C) 2003-2016 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

    2016.05.16: Added CCD Cooler Power (JM)

*/

#include <memory>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>

#include "config.h"

#include "indidevapi.h"
#include "eventloop.h"

#include "fli_ccd.h"

#define MAX_CCD_TEMP   45   /* Max CCD temperature */
#define MIN_CCD_TEMP   -55  /* Min CCD temperature */
#define MAX_X_BIN      16   /* Max Horizontal binning */
#define MAX_Y_BIN      16   /* Max Vertical binning */
#define MAX_PIXELS     4096 /* Max number of pixels in one dimension */
#define POLLMS         1000 /* Polling time (ms) */
#define TEMP_THRESHOLD .25  /* Differential temperature threshold (C)*/
#define NFLUSHES       1    /* Number of times a CCD array is flushed before an exposure */

std::unique_ptr<FLICCD> fliCCD(new FLICCD());

const flidomain_t Domains[] = { FLIDOMAIN_USB, FLIDOMAIN_SERIAL, FLIDOMAIN_PARALLEL_PORT, FLIDOMAIN_INET };

void ISGetProperties(const char *dev)
{
    fliCCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    fliCCD->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    fliCCD->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    fliCCD->ISNewNumber(dev, name, values, names, num);
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
    fliCCD->ISSnoopDevice(root);
}

FLICCD::FLICCD()
{
    sim = false;

    setVersion(FLI_CCD_VERSION_MAJOR, FLI_CCD_VERSION_MINOR);
}

FLICCD::~FLICCD()
{
}

const char *FLICCD::getDefaultName()
{
    return (char *)"FLI CCD";
}

bool FLICCD::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    IUFillSwitch(&PortS[0], "USB", "USB", ISS_ON);
    IUFillSwitch(&PortS[1], "SERIAL", "Serial", ISS_OFF);
    IUFillSwitch(&PortS[2], "PARALLEL", "Parallel", ISS_OFF);
    IUFillSwitch(&PortS[3], "INET", "INet", ISS_OFF);
    IUFillSwitchVector(&PortSP, PortS, 4, getDeviceName(), "PORTS", "Port", MAIN_CONTROL_TAB, IP_WO, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillText(&CamInfoT[0], "Model", "", "");
    IUFillText(&CamInfoT[1], "HW Rev", "", "");
    IUFillText(&CamInfoT[2], "FW Rev", "", "");
    IUFillTextVector(&CamInfoTP, CamInfoT, 3, getDeviceName(), "Model", "", IMAGE_INFO_TAB, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "Cooling Power (%)", "%+06.2f", 0., 100., .2, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooling Power", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    SetCCDCapability(CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_COOLER | CCD_HAS_SHUTTER);

    addSimulationControl();
    addDebugControl();
    return true;
}

void FLICCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    defineSwitch(&PortSP);

    addAuxControls();
}

bool FLICCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        defineText(&CamInfoTP);
        defineNumber(&CoolerNP);

        setupParams();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(CamInfoTP.name);
        deleteProperty(CoolerNP.name);

        rmTimer(timerID);
    }

    return true;
}

bool FLICCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        /* Ports */
        if (!strcmp(name, PortSP.name))
        {
            if (IUUpdateSwitch(&PortSP, states, names, n) < 0)
                return false;

            PortSP.s = IPS_OK;
            IDSetSwitch(&PortSP, NULL);
            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool FLICCD::Connect()
{
    int err = 0;

    DEBUG(INDI::Logger::DBG_DEBUG, "Attempting to find FLI CCD...");

    sim = isSimulation();

    if (sim)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Simulator used.");
        return true;
    }

    int portSwitchIndex = IUFindOnSwitchIndex(&PortSP);

    if (findFLICCD(Domains[portSwitchIndex]) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error: no cameras were detected.");
        return false;
    }

    if ((err = FLIOpen(&fli_dev, FLICam.name, FLIDEVICE_CAMERA | FLICam.domain)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: FLIOpen() failed. %s.", strerror((int)-err));
        return false;
    }

    /* Success! */
    DEBUGF(INDI::Logger::DBG_DEBUG, "CCD %s is online.", FLICam.name);
    return true;
}

bool FLICCD::Disconnect()
{
    int err;

    if (sim)
        return true;

    if ((err = FLIClose(fli_dev)))
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Error: FLIClose() failed. %s.", strerror((int)-err));
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "CCD is offline.");
    return true;
}

bool FLICCD::setupParams()
{
    int err = 0;

    DEBUG(INDI::Logger::DBG_DEBUG, "Retieving camera parameters...");

    char hw_rev[16], fw_rev[16];

    //////////////////////
    // 1. Get Camera Model
    //////////////////////

    if (sim)
        IUSaveText(&CamInfoT[0], getDeviceName());
    else if ((err = FLIGetModel(fli_dev, FLICam.model, 32)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetModel() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetModel() succeed -> %s", FLICam.model);
        IUSaveText(&CamInfoT[0], FLICam.model);
    }

    ///////////////////////////
    // 2. Get Hardware revision
    ///////////////////////////
    if (sim)
        FLICam.HWRevision = 1;
    else if ((err = FLIGetHWRevision(fli_dev, &FLICam.HWRevision)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetHWRevision() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        snprintf(hw_rev, 16, "%ld", FLICam.HWRevision);
        IUSaveText(&CamInfoT[1], hw_rev);
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetHWRevision() succeed -> %s", hw_rev);
    }

    ///////////////////////////
    // 3. Get Firmware revision
    ///////////////////////////
    if (sim)
        FLICam.FWRevision = 1;
    else if ((err = FLIGetFWRevision(fli_dev, &FLICam.FWRevision)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetFWRevision() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        snprintf(fw_rev, 16, "%ld", FLICam.FWRevision);
        IUSaveText(&CamInfoT[2], fw_rev);
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetFWRevision() succeed -> %s", fw_rev);
    }

    IDSetText(&CamInfoTP, NULL);
    ///////////////////////////
    // 4. Get Pixel size
    ///////////////////////////
    if (sim)
    {
        FLICam.x_pixel_size = 5.4 / 1e6;
        FLICam.y_pixel_size = 5.4 / 1e6;
    }
    else if ((err = FLIGetPixelSize(fli_dev, &FLICam.x_pixel_size, &FLICam.y_pixel_size)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetPixelSize() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        FLICam.x_pixel_size *= 1e6;
        FLICam.y_pixel_size *= 1e6;
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetPixelSize() succeed -> %g x %g", FLICam.x_pixel_size,
               FLICam.y_pixel_size);
    }

    ///////////////////////////
    // 5. Get array area
    ///////////////////////////
    if (sim)
    {
        FLICam.Array_Area[0] = FLICam.Array_Area[1] = 0;
        FLICam.Array_Area[2]                        = 1280;
        FLICam.Array_Area[3]                        = 1024;
    }
    else if ((err = FLIGetArrayArea(fli_dev, &FLICam.Array_Area[0], &FLICam.Array_Area[1], &FLICam.Array_Area[2],
                                    &FLICam.Array_Area[3])))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetArrayArea() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetArrayArea() succeed -> %ld x %ld + %ld x %ld", FLICam.Array_Area[0],
               FLICam.Array_Area[1], FLICam.Array_Area[2], FLICam.Array_Area[3]);
    }

    ///////////////////////////
    // 6. Get visible area
    ///////////////////////////
    if (sim)
    {
        FLICam.Visible_Area[0] = FLICam.Visible_Area[1] = 0;
        FLICam.Visible_Area[2]                          = 1280;
        FLICam.Visible_Area[3]                          = 1024;
    }
    else if ((err = FLIGetVisibleArea(fli_dev, &FLICam.Visible_Area[0], &FLICam.Visible_Area[1],
                                      &FLICam.Visible_Area[2], &FLICam.Visible_Area[3])))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetVisibleArea() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetVisibleArea() succeed -> %ld x %ld + %ld x %ld", FLICam.Visible_Area[0],
               FLICam.Visible_Area[1], FLICam.Visible_Area[2], FLICam.Visible_Area[3]);
    }

    ///////////////////////////
    // 7. Get temperature
    ///////////////////////////
    if (sim)
        FLICam.temperature = 25.0;
    else if ((err = FLIGetTemperature(fli_dev, &FLICam.temperature)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetTemperature() failed. %s.", strerror((int)-err));
        return false;
    }
    else
    {
        TemperatureN[0].value = FLICam.temperature; /* CCD chip temperatre (degrees C) */
        TemperatureN[0].min   = MIN_CCD_TEMP;
        TemperatureN[0].max   = MAX_CCD_TEMP;
        IUUpdateMinMax(&TemperatureNP);
        IDSetNumber(&TemperatureNP, NULL);
        DEBUGF(INDI::Logger::DBG_SESSION, "FLIGetVisibleArea() succeed -> %f", FLICam.temperature);
    }

    SetCCDParams(FLICam.Visible_Area[2] - FLICam.Visible_Area[0], FLICam.Visible_Area[3] - FLICam.Visible_Area[1], 16,
                 FLICam.x_pixel_size, FLICam.y_pixel_size);

    /* 50 ms */
    minDuration = 0.05;

    if (!sim)
    {
        /* Default frame type is NORMAL */
        if ((err = FLISetFrameType(fli_dev, FLI_FRAME_TYPE_NORMAL)))
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "FLISetFrameType() failed. %s.", strerror((int)-err));
            return false;
        }

        /* X horizontal binning */
        if ((err = FLISetHBin(fli_dev, PrimaryCCD.getBinX())))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLISetBin() failed. %s.", strerror((int)-err));
            return false;
        }

        /* Y vertical binning */
        if ((err = FLISetVBin(fli_dev, PrimaryCCD.getBinY())))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLISetVBin() failed. %s.", strerror((int)-err));
            return false;
        }
    }

    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel count
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

int FLICCD::SetTemperature(double temperature)
{
    int err = 0;

    if (!sim && (err = FLISetTemperature(fli_dev, temperature)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLISetTemperature() failed. %s.", strerror((int)-err));
        return -1;
    }

    FLICam.temperature = temperature;

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting CCD temperature to %+06.2f C", temperature);
    return 0;
}

bool FLICCD::StartExposure(float duration)
{
    int err = 0;

    if (duration < minDuration)
    {
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Exposure shorter than minimum duration %g s requested.  Setting exposure time to %g s.", duration,
               minDuration);
        duration = minDuration;
    }

    if (PrimaryCCD.getFrameType() == INDI::CCDChip::BIAS_FRAME)
    {
        duration = minDuration;
        DEBUGF(INDI::Logger::DBG_DEBUG, "Bias Frame (s) : %g", minDuration);
    }

    if (!sim)
    {
        if ((err = FLISetExposureTime(fli_dev, (long)(duration * 1000))))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLISetExposureTime() failed. %s.", strerror((int)-err));
            return false;
        }
        if ((err = FLIExposeFrame(fli_dev)))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLIExposeFrame() failed. %s.", strerror((int)-err));
            return false;
        }
    }

    PrimaryCCD.setExposureDuration(duration);
    ExposureRequest = duration;

    gettimeofday(&ExpStart, NULL);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;
    return true;
}

bool FLICCD::AbortExposure()
{
    int err = 0;

    if (!sim && (err = FLICancelExposure(fli_dev)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLICancelExposure() failed. %s.", strerror((int)-err));
        return false;
    }

    InExposure = false;
    return true;
}

bool FLICCD::UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType)
{
    INDI_UNUSED(fType);
    int err                           = 0;
    INDI::CCDChip::CCD_FRAME imageFrameType = PrimaryCCD.getFrameType();
    // in indiccd.cpp imageFrameType is already set
    if (sim)
        return true;

    switch (imageFrameType)
    {
        case INDI::CCDChip::BIAS_FRAME:
        case INDI::CCDChip::DARK_FRAME:
            if ((err = FLISetFrameType(fli_dev, FLI_FRAME_TYPE_DARK)))
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "FLISetFrameType() failed. %s.", strerror((int)-err));
                return -1;
            }
            break;

        case INDI::CCDChip::LIGHT_FRAME:
        case INDI::CCDChip::FLAT_FRAME:
            if ((err = FLISetFrameType(fli_dev, FLI_FRAME_TYPE_NORMAL)))
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "FLISetFrameType() failed. %s.", strerror((int)-err));
                return -1;
            }
            break;
    }

    return true;
}

bool FLICCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    int err = 0;

    long bin_right  = x + (w / PrimaryCCD.getBinX());
    long bin_bottom = y + (h / PrimaryCCD.getBinY());

    if (bin_right > PrimaryCCD.getXRes() / PrimaryCCD.getBinX())
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: invalid width requested %d", w);
        return false;
    }
    else if (bin_bottom > PrimaryCCD.getYRes() / PrimaryCCD.getBinY())
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: invalid height request %d", h);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "The Final image area is (%ld, %ld), (%ld, %ld)", x, y, bin_right, bin_bottom);

    if (!sim && (err = FLISetImageArea(fli_dev, x, y, bin_right, bin_bottom)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLISetImageArea() failed. %s.", strerror((int)-err));
        return false;
    }

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x, y, w, h);

    int nbuf;
    nbuf = (w / PrimaryCCD.getBinX()) * (h / PrimaryCCD.getBinY()) * (PrimaryCCD.getBPP() / 8); //  this is pixel count
    nbuf += 512; //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

bool FLICCD::UpdateCCDBin(int binx, int biny)
{
    int err = 0;

    /* X horizontal binning */
    if (!sim && (err = FLISetHBin(fli_dev, binx)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLISetBin() failed. %s.", strerror((int)-err));
        return false;
    }

    /* Y vertical binning */
    if (!sim && (err = FLISetVBin(fli_dev, biny)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLISetVBin() failed. %s.", strerror((int)-err));
        return false;
    }

    PrimaryCCD.setBin(binx, biny);

    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

// Downloads the image from the CCD.
bool FLICCD::grabImage()
{
    int err        = 0;
    uint8_t *image = PrimaryCCD.getFrameBuffer();
    int row_size   = PrimaryCCD.getSubW() / PrimaryCCD.getBinX() * PrimaryCCD.getBPP() / 8;
    int width      = PrimaryCCD.getSubW() / PrimaryCCD.getBinX();
    int height     = PrimaryCCD.getSubH() / PrimaryCCD.getBinY();

    if (sim)
    {
        for (int i = 0; i < height; i++)
            for (int j = 0; j < row_size; j++)
                image[i * row_size + j] = rand() % 255;
    }
    else
    {
        bool success = true;
        for (int i = 0; i < height; i++)
        {
            if ((err = FLIGrabRow(fli_dev, image + (i * row_size), width)))
            {
                /* print this error once but read to the end to flush the array */
                if (success)
                    DEBUGF(INDI::Logger::DBG_ERROR, "FLIGrabRow() failed at row %d. %s.", i, strerror((int)-err));
                success = false;
            }
        }
        if (!success)
            return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    ExposureComplete(&PrimaryCCD);

    return true;
}

void FLICCD::TimerHit()
{
    int timerID     = -1;
    int err         = 0;
    long timeleft   = 0;
    double ccdTemp  = 0;
    double ccdPower = 0;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        if (sim)
        {
            PrimaryCCD.setExposureLeft(0);
            InExposure = false;
            grabImage();
        }
        else
        {
            //timeleft=calcTimeLeft();
            if ((err = FLIGetExposureStatus(fli_dev, &timeleft)))
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetExposureStatus() failed. %s.", strerror((int)-err));
                SetTimer(POLLMS);
                return;
            }
            DEBUGF(INDI::Logger::DBG_DEBUG, "FLIGetExposureStatus() succeed -> %ld", timeleft);
            if (timeleft < 1000)
            {
                while (!sim && timeleft > 0)
                {
                    if ((err = FLIGetExposureStatus(fli_dev, &timeleft)))
                    {
                        DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetExposureStatus() failed. %s.", strerror((int)-err));
                        SetTimer(POLLMS);
                        return;
                    }
                    usleep(1000 * timeleft);
                }

                /* We're done exposing */
                DEBUG(INDI::Logger::DBG_SESSION, "Exposure done, downloading image...");

                PrimaryCCD.setExposureLeft(0);
                InExposure = false;
                /* grab and save image */
                grabImage();
            }
            else
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "Exposure in progress. Time left: %ld seconds", timeleft / 1000);
                PrimaryCCD.setExposureLeft(timeleft / 1000.0);
            }
        }
    }

    switch (TemperatureNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            if (sim == false)
            {
                if ((err = FLIGetTemperature(fli_dev, &ccdTemp)))
                {
                    TemperatureNP.s = IPS_IDLE;
                    IDSetNumber(&TemperatureNP, NULL);
                    DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetTemperature() failed. %s.", strerror((int)-err));
                    break;
                }
                if ((err = FLIGetCoolerPower(fli_dev, &ccdPower)))
                {
                    CoolerNP.s = IPS_IDLE;
                    IDSetNumber(&TemperatureNP, NULL);
                    IDSetNumber(&TemperatureNP, "FLIGetCoolerPower() failed. %s.", strerror((int)-err));
                    break;
                }
            }

            if (fabs(TemperatureN[0].value - ccdTemp) >= TEMP_THRESHOLD)
            {
                TemperatureN[0].value = ccdTemp;
                IDSetNumber(&TemperatureNP, NULL);
            }

            if (fabs(CoolerN[0].value - ccdPower) >= TEMP_THRESHOLD)
            {
                CoolerN[0].value = ccdPower;
                CoolerNP.s       = TemperatureNP.s;
                IDSetNumber(&CoolerNP, NULL);
            }
            break;

        case IPS_BUSY:
            if (sim)
            {
                ccdTemp               = FLICam.temperature;
                TemperatureN[0].value = ccdTemp;
            }
            else
            {
                if ((err = FLIGetTemperature(fli_dev, &ccdTemp)))
                {
                    TemperatureNP.s = IPS_IDLE;
                    IDSetNumber(&TemperatureNP, NULL);
                    DEBUGF(INDI::Logger::DBG_ERROR, "FLIGetTemperature() failed. %s.", strerror((int)-err));
                    break;
                }

                if ((err = FLIGetCoolerPower(fli_dev, &ccdPower)))
                {
                    CoolerNP.s = IPS_IDLE;
                    IDSetNumber(&TemperatureNP, "FLIGetCoolerPower() failed. %s.", strerror((int)-err));
                    break;
                }
            }

            if (fabs(FLICam.temperature - ccdTemp) <= TEMP_THRESHOLD)
            {
                TemperatureNP.s = IPS_OK;
                IDSetNumber(&TemperatureNP, NULL);
            }

            if (fabs(CoolerN[0].value - ccdPower) >= TEMP_THRESHOLD)
            {
                CoolerN[0].value = ccdPower;
                CoolerNP.s       = TemperatureNP.s;
                IDSetNumber(&CoolerNP, NULL);
            }

            TemperatureN[0].value = ccdTemp;
            IDSetNumber(&TemperatureNP, NULL);
            break;

        case IPS_ALERT:
            break;
    }

    if (timerID == -1)
        SetTimer(POLLMS);
    return;
}

bool FLICCD::findFLICCD(flidomain_t domain)
{
    char **names;
    long err;

    DEBUGF(INDI::Logger::DBG_DEBUG, "In find Camera, the domain is %ld", domain);

    if ((err = FLIList(domain | FLIDEVICE_CAMERA, &names)))
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "FLIList() failed. %s", strerror((int)-err));
        return false;
    }

    if (names != NULL && names[0] != NULL)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Devices list: %s", names);

        for (int i = 0; names[i] != NULL; i++)
        {
            for (int j = 0; names[i][j] != '\0'; j++)
                if (names[i][j] == ';')
                {
                    names[i][j] = '\0';
                    break;
                }
        }

        FLICam.domain = domain;

        switch (domain)
        {
            case FLIDOMAIN_PARALLEL_PORT:
                FLICam.dname = strdup("parallel port");
                break;

            case FLIDOMAIN_USB:
                FLICam.dname = strdup("USB");
                break;

            case FLIDOMAIN_SERIAL:
                FLICam.dname = strdup("serial");
                break;

            case FLIDOMAIN_INET:
                FLICam.dname = strdup("inet");
                break;

            default:
                FLICam.dname = strdup("Unknown domain");
        }

        FLICam.name = strdup(names[0]);

        if ((err = FLIFreeList(names)))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLIFreeList() failed. %s.", strerror((int)-err));
            return false;
        }

    } /* end if */
    else
    {
        DEBUG(INDI::Logger::DBG_ERROR, "FLIList returned empty result!");

        if ((err = FLIFreeList(names)))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "FLIFreeList() failed. %s.", strerror((int)-err));
            return false;
        }

        return false;
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "FindFLICCD() finished successfully.");

    return true;
}

void FLICCD::debugTriggered(bool enable)
{
    if (enable)
        FLISetDebugLevel(NULL, FLIDEBUG_INFO);
    else
        FLISetDebugLevel(NULL, FLIDEBUG_WARN);
}
