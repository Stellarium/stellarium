#if 0
    QSI CCD
    INDI Interface for Quantum Scientific Imaging CCDs
    Based on FLI Indi Driver by Jasem Mutlaq.
    Copyright (C) 2009 Sami Lehti (sami.lehti@helsinki.fi)

    (2011-12-10) Updated by Jasem Mutlaq:
        + Driver completely rewritten to be based on INDI::CCD
        + Fixed compiler warnings.
        + Reduced message traffic.
        + Added filter name property.
        + Added snooping on telescopes.
        + Added Guider support.
        + Added Readout speed option.

    2015:
        + Added Fan speed option
        + Added Gain option
        + Added AntiBlooming option (Miguel)

    2016:
        + Check if flushing is supported.
        + Always set BIAS exposure to zero.

    2017:
        + Save Gain, Fan, and Anti-blooming in configuration file.
        + Use FilterWheelInterface functions.

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

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>
#include <time.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <zlib.h>

#include <memory>

#include <fitsio.h>

#include "qsiapi.h"
#include "QSIError.h"
#include "indidevapi.h"
#include "eventloop.h"
#include "indicom.h"
#include "qsi_ccd.h"
#include "config.h"

void ISInit(void);
void ISPoll(void *);

double min(void);
double max(void);

#define FILTER_WHEEL_TAB "Filter Wheel"

#define POLLMS         1000 /* Polling time (ms) */
#define TEMP_THRESHOLD .25  /* Differential temperature threshold (C)*/
#define NFLUSHES       1    /* Number of times a CCD array is flushed before an exposure */

#define currentFilter FilterN[0].value

std::unique_ptr<QSICCD> qsiCCD(new QSICCD());

void ISGetProperties(const char *dev)
{
    qsiCCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    qsiCCD->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    qsiCCD->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    qsiCCD->ISNewNumber(dev, name, values, names, num);
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
    qsiCCD->ISSnoopDevice(root);
}

QSICCD::QSICCD() : FilterInterface(this)
{
    canSetGain            = false;
    canControlFan         = false;
    canSetAB              = false;
    canFlush              = false;
    canChangeReadoutSpeed = false;

    // Initial setting. Updated after connction to camera.
    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 5;

    QSICam.put_UseStructuredExceptions(true);

    setVersion(QSI_VERSION_MAJOR, QSI_VERSION_MINOR);
}

QSICCD::~QSICCD()
{
}

const char *QSICCD::getDefaultName()
{
    return (char *)"QSI CCD";
}

bool QSICCD::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    IUFillSwitch(&CoolerS[0], "COOLER_ON", "ON", ISS_OFF);
    IUFillSwitch(&CoolerS[1], "COOLER_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&CoolerSP, CoolerS, 2, getDeviceName(), "CCD_COOLER", "Cooler", MAIN_CONTROL_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&ShutterS[0], "SHUTTER_ON", "Manual open", ISS_OFF);
    IUFillSwitch(&ShutterS[1], "SHUTTER_OFF", "Manual close", ISS_OFF);
    IUFillSwitchVector(&ShutterSP, ShutterS, 2, getDeviceName(), "CCD_SHUTTER", "Shutter", MAIN_CONTROL_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "Cooling Power (%)", "%+06.2f", 0., 1., .2, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooling Power", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&ReadOutS[0], "QUALITY_HIGH", "High Quality", ISS_ON);
    IUFillSwitch(&ReadOutS[1], "QUALITY_LOW", "Fast", ISS_OFF);
    IUFillSwitchVector(&ReadOutSP, ReadOutS, 2, getDeviceName(), "READOUT_QUALITY", "Readout Speed", OPTIONS_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&FilterS[0], "FILTER_CW", "+", ISS_OFF);
    IUFillSwitch(&FilterS[1], "FILTER_CCW", "-", ISS_OFF);
    IUFillSwitchVector(&FilterSP, FilterS, 2, getDeviceName(), "FILTER_WHEEL_MOTION", "Turn Wheel", FILTER_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&GainS[GAIN_HIGH], "High", "", ISS_ON);
    IUFillSwitch(&GainS[GAIN_LOW], "Low", "", ISS_OFF);
    IUFillSwitch(&GainS[GAIN_AUTO], "Auto", "", ISS_OFF);
    IUFillSwitchVector(&GainSP, GainS, 3, getDeviceName(), "Gain", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&FanS[0], "Off", "", ISS_OFF);
    IUFillSwitch(&FanS[1], "Quiet", "", ISS_OFF);
    IUFillSwitch(&FanS[2], "Full", "", ISS_ON);
    IUFillSwitchVector(&FanSP, FanS, 3, getDeviceName(), "Fan", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&ABS[0], "Normal", "", ISS_ON);
    IUFillSwitch(&ABS[1], "High", "", ISS_OFF);
    IUFillSwitchVector(&ABSP, ABS, 2, getDeviceName(), "AntiBlooming", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60,
                       IPS_IDLE);

    INDI::FilterInterface::initProperties(FILTER_TAB);

    addDebugControl();

    setDriverInterface(getDriverInterface() | FILTER_INTERFACE);

    return true;
}

bool QSICCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        defineSwitch(&CoolerSP);
        defineSwitch(&ShutterSP);
        defineNumber(&CoolerNP);

        setupParams();

        if (filterCount > 0)
        {
            INDI::FilterInterface::updateProperties();
        }

        manageDefaults();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(CoolerSP.name);
        deleteProperty(ShutterSP.name);
        deleteProperty(CoolerNP.name);

        if (canSetGain)
            deleteProperty(GainSP.name);

        if (canSetAB)
            deleteProperty(ABSP.name);

        if (canControlFan)
            deleteProperty(FanSP.name);

        if (canChangeReadoutSpeed)
            deleteProperty(ReadOutSP.name);

        if (filterCount > 0)
        {
            INDI::FilterInterface::updateProperties();
        }

        rmTimer(timerID);
    }

    return true;
}

bool QSICCD::setupParams()
{
    string name, model;
    double temperature;
    double pixel_size_x, pixel_size_y;
    long sub_frame_x, sub_frame_y;
    try
    {
        QSICam.get_Name(name);
        QSICam.get_ModelNumber(model);
        QSICam.get_PixelSizeX(&pixel_size_x);
        QSICam.get_PixelSizeY(&pixel_size_y);
        QSICam.get_NumX(&sub_frame_x);
        QSICam.get_NumY(&sub_frame_y);
        QSICam.get_CCDTemperature(&temperature);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Setup Params failed. %s.", err.what());
        return false;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "The CCD Temperature is %f.", temperature);

    TemperatureN[0].value = temperature; /* CCD chip temperatre (degrees C) */
    IDSetNumber(&TemperatureNP, nullptr);

    SetCCDParams(sub_frame_x, sub_frame_y, 16, pixel_size_x, pixel_size_y);

    imageWidth  = PrimaryCCD.getSubW();
    imageHeight = PrimaryCCD.getSubH();

    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel count
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    try
    {
        QSICam.get_Name(name);
    }
    catch (std::runtime_error &err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_Name() failed. %s.", err.what());
        return false;
    }
    DEBUGF(INDI::Logger::DBG_SESSION, "%s", name.c_str());

    try
    {
        QSICam.get_FilterCount(filterCount);
        DEBUGF(INDI::Logger::DBG_SESSION, "The filter count is %d", filterCount);

        FilterSlotN[0].min = 1;
        FilterSlotN[0].max = filterCount;
        FilterSlotNP.s     = IPS_OK;
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "get_FilterCount() failed. %s.", err.what());
        return false;
    }

    // Only generate filter names if there are none initially
    //if (FilterNameT == nullptr)
        //GetFilterNames(FILTER_TAB);

    double minDuration = 0;

    try
    {
        QSICam.get_MinExposureTime(&minDuration);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_MinExposureTime() failed. %s.", err.what());
        return false;
    }

    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", minDuration, 3600, 1, true);

    bool coolerOn = false;

    try
    {
        QSICam.get_CoolerOn(&coolerOn);
    }
    catch (std::runtime_error err)
    {
        CoolerSP.s   = IPS_IDLE;
        CoolerS[0].s = ISS_OFF;
        CoolerS[1].s = ISS_ON;
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: CoolerOn() failed. %s.", err.what());
        IDSetSwitch(&CoolerSP, nullptr);
        return false;
    }

    CoolerS[0].s = coolerOn ? ISS_ON : ISS_OFF;
    CoolerS[1].s = coolerOn ? ISS_OFF : ISS_ON;
    CoolerSP.s   = IPS_OK;
    IDSetSwitch(&CoolerSP, nullptr);

    QSICamera::CameraGain cGain = QSICamera::CameraGainAuto;
    canSetGain                  = false;

    QSICam.get_CanSetGain(&canSetGain);

    if (canSetGain)
    {
        try
        {
            QSICam.get_CameraGain(&cGain);
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not support gain. %s.", err.what());
            canSetGain = false;
        }

        if (canSetGain)
        {
            IUResetSwitch(&GainSP);
            GainS[cGain].s = ISS_ON;
            defineSwitch(&GainSP);
        }
    }

    QSICamera::AntiBloom cAB = QSICamera::AntiBloomNormal;
    canSetAB                 = true;

    try
    {
        QSICam.get_AntiBlooming(&cAB);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not support AntiBlooming control. %s.", err.what());
        canSetAB = false;
    }

    if (canSetAB)
    {
        IUResetSwitch(&ABSP);
        ABS[cAB].s = ISS_ON;
        defineSwitch(&ABSP);
    }

    QSICamera::FanMode fMode = QSICamera::fanOff;
    canControlFan            = true;

    try
    {
        QSICam.get_FanMode(fMode);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not support fan control. %s.", err.what());
        canControlFan = false;
    }

    if (canControlFan)
    {
        IUResetSwitch(&FanSP);
        FanS[fMode].s = ISS_ON;
        defineSwitch(&FanSP);
    }

    canChangeReadoutSpeed = true;
    QSICamera::ReadoutSpeed cReadoutSpeed;

    try
    {
        QSICam.get_ReadoutSpeed(cReadoutSpeed);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not support changing readout speed. %s.", err.what());
        canChangeReadoutSpeed = false;
    }

    if (canChangeReadoutSpeed)
    {
        // get_ReadoutSpeed succeeds even if the camera does not support that, so the only way to make sure is to set it
        try
        {
            QSICam.put_ReadoutSpeed(cReadoutSpeed);
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not support changing readout speed. %s.", err.what());
            canChangeReadoutSpeed = false;
        }

        if (canChangeReadoutSpeed)
        {
            IUResetSwitch(&ReadOutSP);
            ReadOutS[cReadoutSpeed].s = ISS_ON;
            defineSwitch(&ReadOutSP);
        }
    }

    // Can flush
    canFlush = false;
    try
    {
        QSICam.put_PreExposureFlush(QSICamera::FlushNormal);
        canFlush = true;
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Camera does not pre-exposure flushing %s.", err.what());
        canFlush = false;
    }

    return true;
}

int QSICCD::SetTemperature(double temperature)
{
    bool canSetTemp;
    try
    {
        QSICam.get_CanSetCCDTemperature(&canSetTemp);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "CanSetCCDTemperature() failed. %s.", err.what());
        return -1;
    }

    if (!canSetTemp)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Cannot set CCD temperature, CanSetCCDTemperature == false\n");
        return -1;
    }

    targetTemperature = temperature;

    // If less than 0.1 of a degree, let's just return OK
    if (fabs(temperature - TemperatureN[0].value) < 0.1)
        return 1;

    activateCooler(true);

    try
    {
        QSICam.put_SetCCDTemperature(temperature);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "put_SetCCDTemperature() failed. %s.", err.what());
        return -1;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting CCD temperature to %+06.2f C", temperature);
    return 0;
}

bool QSICCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        /* Readout Speed */
        if (!strcmp(name, ReadOutSP.name))
        {
            if (IUUpdateSwitch(&ReadOutSP, states, names, n) < 0)
                return false;

            if (ReadOutS[0].s == ISS_ON)
            {
                try
                {
                    QSICam.put_ReadoutSpeed(QSICamera::HighImageQuality);
                }
                catch (std::runtime_error err)
                {
                    IUResetSwitch(&ReadOutSP);
                    ReadOutSP.s = IPS_ALERT;
                    DEBUGF(INDI::Logger::DBG_ERROR, "put_ReadoutSpeed() failed. %s.", err.what());
                    IDSetSwitch(&ReadOutSP, nullptr);
                    return false;
                }
            }
            else
            {
                try
                {
                    QSICam.put_ReadoutSpeed(QSICamera::FastReadout);
                }
                catch (std::runtime_error err)
                {
                    IUResetSwitch(&ReadOutSP);
                    ReadOutSP.s = IPS_ALERT;
                    DEBUGF(INDI::Logger::DBG_ERROR, "put_ReadoutSpeed() failed. %s.", err.what());
                    IDSetSwitch(&ReadOutSP, nullptr);
                    return false;
                }

                ReadOutSP.s = IPS_OK;
                IDSetSwitch(&ReadOutSP, nullptr);
            }

            ReadOutSP.s = IPS_OK;
            IDSetSwitch(&ReadOutSP, nullptr);
            return true;
        }

        /* Cooler */
        if (!strcmp(name, CoolerSP.name))
        {
            if (IUUpdateSwitch(&CoolerSP, states, names, n) < 0)
                return false;

            if (CoolerS[0].s == ISS_ON)
                activateCooler(true);
            else
                activateCooler(false);

            return true;
        }

        /* Shutter */
        if (!strcmp(name, ShutterSP.name))
        {
            if (IUUpdateSwitch(&ShutterSP, states, names, n) < 0)
                return false;
            shutterControl();
            return true;
        }

        if (!strcmp(name, GainSP.name))
        {
            int prevGain = IUFindOnSwitchIndex(&GainSP);
            IUUpdateSwitch(&GainSP, states, names, n);
            int targetGain = IUFindOnSwitchIndex(&GainSP);

            if (prevGain == targetGain)
            {
                GainSP.s = IPS_OK;
                IDSetSwitch(&GainSP, nullptr);
                return true;
            }

            try
            {
                QSICam.put_CameraGain(((QSICamera::CameraGain)targetGain));
            }
            catch (std::runtime_error err)
            {
                IUResetSwitch(&GainSP);
                GainS[prevGain].s = ISS_ON;
                GainSP.s          = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "put_CameraGain failed. %s.", err.what());
                IDSetSwitch(&GainSP, nullptr);
                return false;
            }

            GainSP.s = IPS_OK;
            IDSetSwitch(&GainSP, nullptr);
            return true;
        }

        if (!strcmp(name, FanSP.name))
        {
            int prevFan = IUFindOnSwitchIndex(&FanSP);
            IUUpdateSwitch(&FanSP, states, names, n);
            int targetFan = IUFindOnSwitchIndex(&FanSP);

            if (prevFan == targetFan)
            {
                FanSP.s = IPS_OK;
                IDSetSwitch(&FanSP, nullptr);
                return true;
            }

            try
            {
                QSICam.put_FanMode(((QSICamera::FanMode)targetFan));
            }
            catch (std::runtime_error err)
            {
                IUResetSwitch(&FanSP);
                FanS[prevFan].s = ISS_ON;
                FanSP.s         = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "put_FanMode failed. %s.", err.what());
                IDSetSwitch(&FanSP, nullptr);
                return false;
            }

            FanSP.s = IPS_OK;
            IDSetSwitch(&FanSP, nullptr);
            return true;
        }

        if (!strcmp(name, ABSP.name))
        {
            int prevAB = IUFindOnSwitchIndex(&ABSP);
            IUUpdateSwitch(&ABSP, states, names, n);
            int targetAB = IUFindOnSwitchIndex(&ABSP);

            if (prevAB == targetAB)
            {
                ABSP.s = IPS_OK;
                IDSetSwitch(&ABSP, nullptr);
                return true;
            }

            try
            {
                QSICam.put_AntiBlooming(((QSICamera::AntiBloom)targetAB));
            }
            catch (std::runtime_error err)
            {
                IUResetSwitch(&ABSP);
                ABS[prevAB].s = ISS_ON;
                ABSP.s        = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "put_AntiBlooming failed. %s.", err.what());
                IDSetSwitch(&ABSP, nullptr);
                return false;
            }

            ABSP.s = IPS_OK;
            IDSetSwitch(&ABSP, nullptr);
            return true;
        }

        /* Filter Wheel */
        if (!strcmp(name, FilterSP.name))
        {
            if (IUUpdateSwitch(&FilterSP, states, names, n) < 0)
                return false;
            turnWheel();
            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool QSICCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //int maxFilters = (int)FilterSlotN[0].max;
    //std::string filterDesignation[maxFilters];

    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FilterNameTP->name) == 0)
        {
            INDI::FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool QSICCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FilterSlotNP.name) == 0)
        {
            INDI::FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool QSICCD::StartExposure(float duration)
{
    imageFrameType = PrimaryCCD.getFrameType();

    if (imageFrameType == INDI::CCDChip::BIAS_FRAME)
    {
        ExposureRequest = 0;
        PrimaryCCD.setExposureDuration(0);
        DEBUG(INDI::Logger::DBG_SESSION, "Bias Frame (s) : 0");
    }
    else
    {
        ExposureRequest = duration;
        PrimaryCCD.setExposureDuration(ExposureRequest);
    }

    if (canFlush)
    {
        try
        {
            QSICam.put_PreExposureFlush(QSICamera::FlushNormal);
        }
        catch (std::runtime_error &err)
        {
            DEBUGF(INDI::Logger::DBG_WARNING, "Warning! Pre-exposure flushing failed. %s.", err.what());
        }
    }

    /* BIAS frame is the same as DARK but with minimum period. i.e. readout from camera electronics.*/
    if (imageFrameType == INDI::CCDChip::BIAS_FRAME || imageFrameType == INDI::CCDChip::DARK_FRAME)
    {
        try
        {
            QSICam.StartExposure(ExposureRequest, false);
        }
        catch (std::runtime_error &err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "StartExposure() failed. %s.", err.what());
            return false;
        }
    }
    else if (imageFrameType == INDI::CCDChip::LIGHT_FRAME || imageFrameType == INDI::CCDChip::FLAT_FRAME)
    {
        try
        {
            QSICam.StartExposure(ExposureRequest, true);
        }
        catch (std::runtime_error &err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "StartExposure() failed. %s.", err.what());
            return false;
        }
    }

    gettimeofday(&ExpStart, nullptr);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;
    return true;
}

bool QSICCD::AbortExposure()
{
    if (canAbort)
    {
        try
        {
            QSICam.AbortExposure();
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "AbortExposure() failed. %s.", err.what());
            return false;
        }

        InExposure = false;
        return true;
    }

    return false;
}

float QSICCD::CalcTimeLeft(timeval start, float req)
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, nullptr);

    timesince =
        (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = req - timesince;
    return timeleft;
}

bool QSICCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    char errmsg[ERRMSG_SIZE];

    /* Add the X and Y offsets */
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
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "The Final image area is (%ld, %ld), (%ld, %ld)\n", x_1, y_1, x_2, y_2);

    imageWidth  = x_2 - x_1;
    imageHeight = y_2 - y_1;

    try
    {
        QSICam.put_StartX(x_1);
        QSICam.put_StartY(y_1);
        QSICam.put_NumX(imageWidth);
        QSICam.put_NumY(imageHeight);
    }
    catch (std::runtime_error err)
    {
        snprintf(errmsg, ERRMSG_SIZE, "Setting image area failed. %s.\n", err.what());
        DEBUGF(INDI::Logger::DBG_ERROR, "Setting image area failed. %s.", err.what());
        return false;
    }

    // Set UNBINNED coords
    PrimaryCCD.setFrame(x, y, w, h);
    int nbuf;
    nbuf = (imageWidth * imageHeight * PrimaryCCD.getBPP() / 8); //  this is pixel count
    nbuf += 512;                                                 //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

bool QSICCD::UpdateCCDBin(int binx, int biny)
{
    try
    {
        QSICam.put_BinX(binx);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "put_BinX() failed. %s.", err.what());
        return false;
    }

    try
    {
        QSICam.put_BinY(biny);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "put_BinY() failed. %s.", err.what());
        return false;
    }

    PrimaryCCD.setBin(binx, biny);

    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

/* Downloads the image from the CCD.
 N.B. No processing is done on the image */
int QSICCD::grabImage()
{
    unsigned short *image = (unsigned short *)PrimaryCCD.getFrameBuffer();

    int x, y, z;
    try
    {
        bool imageReady = false;
        QSICam.get_ImageReady(&imageReady);
        while (!imageReady)
        {
            usleep(500);
            QSICam.get_ImageReady(&imageReady);
        }

        QSICam.get_ImageArraySize(x, y, z);
        QSICam.get_ImageArray(image);
        imageWidth  = x;
        imageHeight = y;
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_ImageArray() failed. %s.", err.what());
        return -1;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    ExposureComplete(&PrimaryCCD);

    return 0;
}

void QSICCD::addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip)
{
    INDI::CCD::addFITSKeywords(fptr, targetChip);

    int status = 0;
    double electronsPerADU;

    try
    {
        QSICam.get_ElectronsPerADU(&electronsPerADU);
    }
    catch (std::runtime_error &err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_ElectronsPerADU failed. %s.", err.what());
        return;
    }

    // 2017-09-17 JM: electronsPerADU is wrong in auto mode. So we have to change it manually here.
    if (IUFindOnSwitchIndex(&GainSP) == GAIN_AUTO && PrimaryCCD.getBinX() > 1)
        electronsPerADU = 1.1;

    fits_update_key_s(fptr, TDOUBLE, "EPERADU", &electronsPerADU, "Electrons per ADU", &status);
}

bool QSICCD::manageDefaults()
{
    /* X horizontal binning */
    try
    {
        QSICam.put_BinX(PrimaryCCD.getBinX());
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: put_BinX() failed. %s.", err.what());
        return false;
    }

    /* Y vertical binning */
    try
    {
        QSICam.put_BinY(PrimaryCCD.getBinY());
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: put_BinY() failed. %s.", err.what());
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Setting default binning %d x %d.\n", PrimaryCCD.getBinX(), PrimaryCCD.getBinY());

    UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getXRes(), PrimaryCCD.getYRes());

    /* Success */
    return true;
}

bool QSICCD::Connect()
{
    bool connected;

    DEBUG(INDI::Logger::DBG_SESSION, "Attempting to find QSI CCD...");

    try
    {
        QSICam.get_Connected(&connected);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: get_Connected() failed. %s.", err.what());
        return false;
    }

    if (!connected)
    {
        try
        {
            QSICam.put_Connected(true);
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: put_Connected(true) failed. %s.", err.what());
            return false;
        }
    }

    bool hasST4Port = false;
    try
    {
        QSICam.get_CanPulseGuide(&hasST4Port);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_canPulseGuide() failed. %s.", err.what());
        return false;
    }

    try
    {
        QSICam.get_CanAbortExposure(&canAbort);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_CanAbortExposure() failed. %s.", err.what());
        return false;
    }

    uint32_t cap = CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_COOLER | CCD_HAS_SHUTTER;

    if (canAbort)
        cap |= CCD_CAN_ABORT;

    if (hasST4Port)
        cap |= CCD_HAS_ST4_PORT;

    SetCCDCapability(cap);

    /* Success! */
    DEBUG(INDI::Logger::DBG_SESSION, "CCD is online. Retrieving basic data.");
    return true;
}

bool QSICCD::Disconnect()
{
    bool connected;

    try
    {
        QSICam.get_Connected(&connected);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: get_Connected() failed. %s.", err.what());
        return false;
    }

    if (connected)
    {
        try
        {
            QSICam.put_Connected(false);
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: put_Connected(false) failed. %s.", err.what());
            return false;
        }
    }

    DEBUG(INDI::Logger::DBG_SESSION, "CCD is offline.");
    return true;
}

void QSICCD::activateCooler(bool enable)
{
    bool coolerOn;

    if (enable)
    {
        try
        {
            QSICam.get_CoolerOn(&coolerOn);
        }
        catch (std::runtime_error err)
        {
            CoolerSP.s   = IPS_IDLE;
            CoolerS[0].s = ISS_OFF;
            CoolerS[1].s = ISS_ON;
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: CoolerOn() failed. %s.", err.what());
            IDSetSwitch(&CoolerSP, nullptr);
            return;
        }

        if (!coolerOn)
        {
            try
            {
                QSICam.put_CoolerOn(true);
            }
            catch (std::runtime_error err)
            {
                CoolerSP.s   = IPS_IDLE;
                CoolerS[0].s = ISS_OFF;
                CoolerS[1].s = ISS_ON;
                DEBUGF(INDI::Logger::DBG_ERROR, "Error: put_CoolerOn(true) failed. %s.", err.what());
                return;
            }
        }

        /* Success! */
        CoolerS[0].s = ISS_ON;
        CoolerS[1].s = ISS_OFF;
        CoolerSP.s   = IPS_OK;
        DEBUG(INDI::Logger::DBG_SESSION, "Cooler ON");
        IDSetSwitch(&CoolerSP, nullptr);
        CoolerNP.s = IPS_BUSY;
    }
    else
    {
        CoolerS[0].s = ISS_OFF;
        CoolerS[1].s = ISS_ON;
        CoolerSP.s   = IPS_IDLE;

        try
        {
            QSICam.get_CoolerOn(&coolerOn);
            if (coolerOn)
                QSICam.put_CoolerOn(false);
        }
        catch (std::runtime_error err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: CoolerOn() failed. %s.", err.what());
            IDSetSwitch(&CoolerSP, nullptr);
            return;
        }
        DEBUG(INDI::Logger::DBG_SESSION, "Cooler is OFF.");
        IDSetSwitch(&CoolerSP, nullptr);
    }
}

void QSICCD::shutterControl()
{
    bool hasShutter;
    try
    {
        QSICam.get_HasShutter(&hasShutter);
    }
    catch (std::runtime_error err)
    {
        ShutterSP.s   = IPS_IDLE;
        ShutterS[0].s = ISS_OFF;
        ShutterS[1].s = ISS_OFF;
        DEBUGF(INDI::Logger::DBG_ERROR, "QSICamera::get_HasShutter() failed. %s.", err.what());
        return;
    }

    if (hasShutter)
    {
        switch (ShutterS[0].s)
        {
            case ISS_ON:

                try
                {
                    QSICam.put_ManualShutterMode(true);
                    QSICam.put_ManualShutterOpen(true);
                }
                catch (std::runtime_error err)
                {
                    ShutterSP.s   = IPS_IDLE;
                    ShutterS[0].s = ISS_OFF;
                    ShutterS[1].s = ISS_ON;
                    DEBUGF(INDI::Logger::DBG_ERROR, "Error: ManualShutterOpen() failed. %s.", err.what());
                    IDSetSwitch(&ShutterSP, nullptr);
                    return;
                }

                /* Success! */
                ShutterS[0].s = ISS_ON;
                ShutterS[1].s = ISS_OFF;
                ShutterSP.s   = IPS_OK;
                DEBUG(INDI::Logger::DBG_SESSION, "Shutter opened manually.");
                IDSetSwitch(&ShutterSP, nullptr);
                break;

            case ISS_OFF:

                try
                {
                    QSICam.put_ManualShutterOpen(false);
                    QSICam.put_ManualShutterMode(false);
                }
                catch (std::runtime_error err)
                {
                    ShutterSP.s   = IPS_IDLE;
                    ShutterS[0].s = ISS_ON;
                    ShutterS[1].s = ISS_OFF;
                    DEBUGF(INDI::Logger::DBG_ERROR, "Error: ManualShutterOpen() failed. %s.", err.what());
                    IDSetSwitch(&ShutterSP, nullptr);
                    return;
                }

                /* Success! */
                ShutterS[0].s = ISS_OFF;
                ShutterS[1].s = ISS_ON;
                ShutterSP.s   = IPS_IDLE;
                DEBUG(INDI::Logger::DBG_SESSION, "Shutter closed manually.");
                IDSetSwitch(&ShutterSP, nullptr);
                break;
        }
    }
}

void QSICCD::TimerHit()
{
    long timeleft = 0;
    double ccdTemp = 0;
    double coolerPower = 0;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        bool imageReady;

        timeleft = CalcTimeLeft(ExpStart, ExposureRequest);

        if (timeleft < 1)
        {
            QSICam.get_ImageReady(&imageReady);

            while (!imageReady)
            {
                usleep(100);
                QSICam.get_ImageReady(&imageReady);
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
            DEBUGF(INDI::Logger::DBG_DEBUG, "Image not ready, time left %ld\n", timeleft);
            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    switch (TemperatureNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            try
            {
                QSICam.get_CCDTemperature(&ccdTemp);
            }
            catch (std::runtime_error err)
            {
                TemperatureNP.s = IPS_IDLE;
                DEBUGF(INDI::Logger::DBG_ERROR, "get_CCDTemperature() failed. %s.", err.what());
                IDSetNumber(&TemperatureNP, nullptr);
                return;
            }

            if (fabs(TemperatureN[0].value - ccdTemp) >= TEMP_THRESHOLD)
            {
                TemperatureN[0].value = ccdTemp;
                IDSetNumber(&TemperatureNP, nullptr);
            }
            break;

        case IPS_BUSY:
            try
            {
                QSICam.get_CCDTemperature(&TemperatureN[0].value);
            }
            catch (std::runtime_error err)
            {
                TemperatureNP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "get_CCDTemperature() failed. %s.", err.what());
                IDSetNumber(&TemperatureNP, nullptr);
                return;
            }

            if (fabs(TemperatureN[0].value - targetTemperature) <= TEMP_THRESHOLD)
                TemperatureNP.s = IPS_OK;

            IDSetNumber(&TemperatureNP, nullptr);
            break;

        case IPS_ALERT:
            break;
    }

    switch (CoolerNP.s)
    {
        case IPS_IDLE:
        case IPS_OK:
            try
            {
                QSICam.get_CoolerPower(&coolerPower);
            }
            catch (std::runtime_error err)
            {
                CoolerNP.s = IPS_IDLE;
                DEBUGF(INDI::Logger::DBG_ERROR, "get_CoolerPower() failed. %s.", err.what());
                IDSetNumber(&CoolerNP, nullptr);
                return;
            }

            if (CoolerN[0].value != coolerPower)
            {
                CoolerN[0].value = coolerPower;
                IDSetNumber(&CoolerNP, nullptr);
            }

            if (coolerPower > 0)
                CoolerNP.s = IPS_BUSY;

            break;

        case IPS_BUSY:
            try
            {
                QSICam.get_CoolerPower(&coolerPower);
            }
            catch (std::runtime_error err)
            {
                CoolerNP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "get_CoolerPower() failed. %s.", err.what());
                IDSetNumber(&CoolerNP, nullptr);
                return;
            }

            if (coolerPower == 0)
                CoolerNP.s = IPS_IDLE;

            CoolerN[0].value = coolerPower;
            IDSetNumber(&CoolerNP, nullptr);
            break;

        case IPS_ALERT:
            break;
    }

    SetTimer(POLLMS);
    return;
}

void QSICCD::turnWheel()
{
    short current_filter = 0;

    switch (FilterS[0].s)
    {
        case ISS_ON:
            try
            {
                current_filter = QueryFilter();
                if (current_filter < filterCount)
                    current_filter++;
                else
                    current_filter = 1;

                SelectFilter(current_filter);
            }
            catch (std::runtime_error err)
            {
                FilterSP.s   = IPS_IDLE;
                FilterS[0].s = ISS_OFF;
                FilterS[1].s = ISS_OFF;
                DEBUGF(INDI::Logger::DBG_ERROR, "QSICamera::get_FilterPos() failed. %s.", err.what());
                return;
            }

            FilterSlotN[0].value = current_filter;
            FilterS[0].s         = ISS_OFF;
            FilterS[1].s         = ISS_OFF;
            FilterSP.s           = IPS_OK;
            DEBUGF(INDI::Logger::DBG_DEBUG, "The current filter is %d", current_filter);
            IDSetSwitch(&FilterSP, nullptr);
            break;

        case ISS_OFF:
            try
            {
                current_filter = QueryFilter();
                if (current_filter > 1)
                    current_filter--;
                else
                    current_filter = filterCount;
                SelectFilter(current_filter);
            }
            catch (std::runtime_error err)
            {
                FilterSP.s   = IPS_IDLE;
                FilterS[0].s = ISS_OFF;
                FilterS[1].s = ISS_OFF;
                DEBUGF(INDI::Logger::DBG_ERROR, "QSICamera::get_FilterPos() failed. %s.", err.what());
                return;
            }

            FilterSlotN[0].value = current_filter;
            FilterS[0].s         = ISS_OFF;
            FilterS[1].s         = ISS_OFF;
            FilterSP.s           = IPS_OK;
            DEBUGF(INDI::Logger::DBG_DEBUG, "The current filter is %d", current_filter);
            IDSetSwitch(&FilterSP, nullptr);
            IDSetNumber(&FilterSlotNP, nullptr);
            break;
    }
}

IPState QSICCD::GuideNorth(float duration)
{
    try
    {
        QSICam.PulseGuide(QSICamera::guideNorth, duration);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "PulseGuide() failed. %s.", err.what());
        return IPS_ALERT;
    }

    return IPS_OK;
}

IPState QSICCD::GuideSouth(float duration)
{
    try
    {
        QSICam.PulseGuide(QSICamera::guideSouth, duration);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "PulseGuide() failed. %s.", err.what());
        return IPS_ALERT;
    }

    return IPS_OK;
}

IPState QSICCD::GuideEast(float duration)
{
    try
    {
        QSICam.PulseGuide(QSICamera::guideEast, duration);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "PulseGuide() failed. %s.", err.what());
        return IPS_ALERT;
    }

    return IPS_OK;
}

IPState QSICCD::GuideWest(float duration)
{
    try
    {
        QSICam.PulseGuide(QSICamera::guideWest, duration);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "PulseGuide() failed. %s.", err.what());
        return IPS_ALERT;
    }

    return IPS_OK;
}

bool QSICCD::GetFilterNames()
{
    char filterName[MAXINDINAME];
    char filterLabel[MAXINDILABEL];

    if (FilterNameT != nullptr)
    {
        delete [] FilterNameT;
        FilterNameT = nullptr;
    }

    auto* filterDesignation = new std::string[filterCount];

    try
    {
        QSICam.get_Names(filterDesignation);
    }
    catch (std::runtime_error err)
    {
        delete [] filterDesignation;
        DEBUGF(INDI::Logger::DBG_ERROR, "QSICamera::get_Names() failed. %s.", err.what());
        return false;
    }

    FilterNameT = new IText[filterCount];

    for (int i = 0; i < filterCount; i++)
    {
        snprintf(filterName, MAXINDINAME, "FILTER_SLOT_NAME_%d", i + 1);
        snprintf(filterLabel, MAXINDILABEL, "Filter #%d", i + 1);
        IUFillText(&FilterNameT[i], filterName, filterLabel, filterDesignation[i].c_str());
    }
    IUFillTextVector(FilterNameTP, FilterNameT, filterCount, getDeviceName(), "FILTER_NAME", "Filter", FilterSlotNP.group, IP_RW, 1, IPS_IDLE);
    delete [] filterDesignation;
    return true;
}

bool QSICCD::SetFilterNames()
{
    auto* filterDesignation = new std::string[filterCount];

    for (int i = 0; i < filterCount; i++)
        filterDesignation[i] = FilterNameT[i].text;

    try
    {
        QSICam.put_Names(filterDesignation);
    }
    catch (std::runtime_error err)
    {
        delete [] filterDesignation;
        DEBUGF(INDI::Logger::DBG_ERROR, "put_Names() failed. %s.", err.what());
        IDSetText(FilterNameTP, nullptr);
        return false;
    }
    delete [] filterDesignation;

    saveConfig(true, FilterNameTP->name);

    return true;
}

bool QSICCD::SelectFilter(int targetFilter)
{
    short filter = targetFilter - 1;
    try
    {
        QSICam.put_Position(filter);
    }
    catch (std::runtime_error err)
    {
        FilterSlotNP.s = IPS_ALERT;
        DEBUGF(INDI::Logger::DBG_ERROR, "put_Position() failed. %s.", err.what());
        return false;
    }

    // Check current filter position
    short newFilter = QueryFilter();

    if (newFilter == targetFilter)
    {
        FilterSlotN[0].value = targetFilter;
        FilterSlotNP.s       = IPS_OK;
        DEBUGF(INDI::Logger::DBG_DEBUG, "Filter set to slot #%d", targetFilter);
        IDSetNumber(&FilterSlotNP, nullptr);
        return true;
    }

    IDSetNumber(&FilterSlotNP, nullptr);
    FilterSlotNP.s = IPS_ALERT;
    return false;
}

int QSICCD::QueryFilter()
{
    short newFilter=0;
    try
    {
        QSICam.get_Position(&newFilter);
    }
    catch (std::runtime_error err)
    {
        FilterSlotNP.s = IPS_ALERT;
        DEBUGF(INDI::Logger::DBG_ERROR, "get_Position() failed. %s.", err.what());
        IDSetNumber(&FilterSlotNP, nullptr);
        return -1;
    }

    return newFilter + 1;
}

bool QSICCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);
    if (filterCount > 0)
        INDI::FilterInterface::saveConfigItems(fp);

    if (canSetGain)
        IUSaveConfigSwitch(fp, &GainSP);
    if (canControlFan)
        IUSaveConfigSwitch(fp, &FanSP);
    if (canSetAB)
        IUSaveConfigSwitch(fp, &ABSP);

    return true;
}
