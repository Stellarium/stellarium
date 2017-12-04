/*
    Driver type: GPhoto Camera INDI Driver

    Copyright (C) 2009 Geoffrey Hausheer
    Copyright (C) 2013 Jasem Mutlaq (mutlaqja AT ikarustech DOT com)

    This library is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation; either version 2.1 of the License, or
    (at your option) any later version.

    This library is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
    License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this library; if not, write to the Free Software Foundation,
    Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA

*/

#include "gphoto_ccd.h"

#include "config.h"
#include "gphoto_driver.h"
#include "gphoto_readimage.h"

#include <stream/streammanager.h>

#include <math.h>
#include <unistd.h>

#define FOCUS_TAB    "Focus"
#define MAX_DEVICES  5 /* Max device cameraCount */
#define POLLMS       1000
#define STREAMPOLLMS 50
#define FOCUS_TIMER  50
#define MAX_RETRIES  3

extern char *me;

static int cameraCount = 0;
static GPhotoCCD *cameras[MAX_DEVICES];
static GPContext *context = gp_context_new();

typedef struct
{
    const char *exec;
    const char *driver;
    const char *model;
} CamDriverInfo;

static CamDriverInfo camInfos[] = { { "indi_gphoto_ccd", "GPhoto CCD", "GPhoto" },
                                    { "indi_canon_ccd", "Canon DSLR", "Canon" },
                                    { "indi_nikon_ccd", "Nikon DSLR", "Nikon" },
                                    { NULL, NULL, NULL } };

/**********************************************************
 *
 *  IMPORRANT: List supported camera models in initializer of deviceTypes structure
 *
 **********************************************************/

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
        // Let's just create one camera for now
        if (!strcmp(me, "indi_gphoto_ccd"))
        {
            isInit      = true;
            cameraCount = 1;
            cameras[0]  = new GPhotoCCD();
            atexit(cleanup);
        }
        else
        {
            CameraList *list;
            /* Detect all the cameras that can be autodetected... */
            int ret = gp_list_new(&list);
            if (ret < GP_OK)
            {
                // Use Legacy Mode
                IDLog("Failed to initilize list in libgphoto2\n");
                return;
            }

            const char *model, *port;
            gp_list_reset(list);
            cameraCount = gp_camera_autodetect(list, context);
            /* Now open all cameras we autodected for usage */
            IDLog("Number of cameras detected: %d.\n", cameraCount);

            if (cameraCount == 0)
            {
                IDLog("Failed to detect any cameras. Check power and make sure camera is not mounted by other programs "
                      "and try again.\n");
// Use Legacy Mode
#if 0
             IDLog("No cameras detected. Using legacy mode...");
             cameraCount = 1;
             cameras[0] = new GPhotoCCD();
             atexit(cleanup);
             isInit = true;
#endif
                return;
            }

            int availableCameras = cameraCount;
            int cameraIndex = 0;
            cameraCount=0;

            while (availableCameras > 0)
            {
                gp_list_get_name(list, cameraIndex, &model);
                gp_list_get_value(list, cameraIndex, &port);

                cameraIndex++;
                availableCameras--;

                if (strcmp(me, "indi_gphoto_ccd"))
                {
                    char prefix[MAXINDINAME];
                    char name[MAXINDINAME];
                    bool modelFound = false;

                    for (int j = 0; camInfos[j].exec != NULL; j++)
                    {
                        if (strstr(model, camInfos[j].model))
                        {
                            strncpy(prefix, camInfos[j].driver, MAXINDINAME);
                            snprintf(name, MAXINDIDEVICE, "%s %s", prefix, model + strlen(camInfos[j].model) + 1);
                            cameras[cameraCount] = new GPhotoCCD(model, port);
                            cameras[cameraCount]->setDeviceName(name);
                            cameraCount++;
                            modelFound = true;
                            break;
                        }
                    }

                    if (modelFound == false)
                        IDLog("Failed to find model %s in supported cameras.", model);
                }
                else
                {
                    cameras[cameraCount++] = new GPhotoCCD(model, port);
                }
            }
            atexit(cleanup);
            isInit = true;
        }
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();

    if (cameraCount == 0)
    {
        IDMessage(nullptr, "No cameras detected.Check power and make sure camera is not mounted by other programs and try again.");
        return;
    }

    for (int i = 0; i < cameraCount; i++)
    {
        GPhotoCCD *camera = cameras[i];
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
        GPhotoCCD *camera = cameras[i];
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
        GPhotoCCD *camera = cameras[i];
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
        GPhotoCCD *camera = cameras[i];
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
        GPhotoCCD *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

//==========================================================================
GPhotoCCD::GPhotoCCD()
{
    memset(model, 0, MAXINDINAME);
    memset(port, 0, MAXINDINAME);

    gphotodrv        = NULL;
    frameInitialized = false;
    on_off[0]        = strdup("On");
    on_off[1]        = strdup("Off");

    setVersion(INDI_GPHOTO_VERSION_MAJOR, INDI_GPHOTO_VERSION_MINOR);    
}

GPhotoCCD::GPhotoCCD(const char *model, const char *port)
{
    strncpy(this->port, port, MAXINDINAME);
    strncpy(this->model, model, MAXINDINAME);

    gphotodrv        = NULL;
    frameInitialized = false;
    on_off[0]        = strdup("On");
    on_off[1]        = strdup("Off");

    setVersion(INDI_GPHOTO_VERSION_MAJOR, INDI_GPHOTO_VERSION_MINOR);
}

//==========================================================================
GPhotoCCD::~GPhotoCCD()
{
    free(on_off[0]);
    free(on_off[1]);
    expTID = 0;
}

const char *GPhotoCCD::getDefaultName()
{
    return (const char *)"GPhoto CCD";
}

bool GPhotoCCD::initProperties()
{
    /*if (strcmp(me, "indi_gphoto_ccd"))
    {
        char prefix[MAXINDINAME];
        modelFound = false;

        for (int i = 0; camInfos[i].exec != NULL; i++)
        {
            if (strstr(model, camInfos[i].model))
            {
                strncpy(prefix, camInfos[i].driver, MAXINDINAME);
                snprintf(this->name, MAXINDIDEVICE, "%s %s", prefix, model + strlen(camInfos[i].model) + 1);
                setDeviceName(this->name);
                modelFound = true;
            }
        }

        if (modelFound == false)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Failed to find model %s in %s", model, getDeviceName());
            return false;
        }
    }*/
    //else
    //{
        // For now let's set name to default name. In the future, we need to to support multiple devices per one driver
        if (*getDeviceName() == '\0')
            strncpy(name, getDefaultName(), MAXINDINAME);
        else
            strncpy(name, getDeviceName(), MAXINDINAME);
        setDeviceName(this->name);
        //modelFound = true;
    //}

    // Init parent properties first
    INDI::CCD::initProperties();

    initFocuserProperties(getDeviceName(), FOCUS_TAB);

    IUFillText(&mPortT[0], "PORT", "Port", "");
    IUFillTextVector(&PortTP, mPortT, NARRAY(mPortT), getDeviceName(), "DEVICE_PORT", "Shutter Release",
                     MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    IUFillNumber(&mMirrorLockN[0], "MIRROR_LOCK_SECONDS", "Seconds", "%1.0f", 0, 10, 1, 0);
    IUFillNumberVector(&mMirrorLockNP, mMirrorLockN, 1, getDeviceName(), "MIRROR_LOCK", "Mirror Lock", MAIN_CONTROL_TAB,
                       IP_RW, 60, IPS_IDLE);

    //We don't know how many items will be in the switch yet
    IUFillSwitchVector(&mIsoSP, NULL, 0, getDeviceName(), "CCD_ISO", "ISO", IMAGE_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60,
                       IPS_IDLE);
    IUFillSwitchVector(&mFormatSP, NULL, 0, getDeviceName(), "CAPTURE_FORMAT", "Capture Format", IMAGE_SETTINGS_TAB,
                       IP_RW, ISR_1OFMANY, 60, IPS_IDLE);
    IUFillSwitchVector(&mExposurePresetSP, NULL, 0, getDeviceName(), "CCD_EXPOSURE_PRESETS", "Presets",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&autoFocusS[0], "Set", "", ISS_OFF);
    IUFillSwitchVector(&autoFocusSP, autoFocusS, 1, getDeviceName(), "Auto Focus", "", FOCUS_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&transferFormatS[0], "FORMAT_FITS", "FITS", ISS_ON);
    IUFillSwitch(&transferFormatS[1], "FORMAT_NATIVE", "Native", ISS_OFF);
    IUFillSwitchVector(&transferFormatSP, transferFormatS, 2, getDeviceName(), "CCD_TRANSFER_FORMAT", "Transfer Format",
                       IMAGE_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&livePreviewS[0], "Enable", "", ISS_OFF);
    IUFillSwitch(&livePreviewS[1], "Disable", "", ISS_ON);
    IUFillSwitchVector(&livePreviewSP, livePreviewS, 2, getDeviceName(), "AUX_VIDEO_STREAM", "Preview",
                       MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&captureTargetS[CAPTURE_INTERNAL_RAM], "RAM", "", ISS_ON);
    IUFillSwitch(&captureTargetS[CAPTURE_SD_CARD], "SD Card", "", ISS_OFF);
    IUFillSwitchVector(&captureTargetSP, captureTargetS, 2, getDeviceName(), "CCD_CAPTURE_TARGET", "Capture Target",
                       IMAGE_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.001, 3600, 1, false);

    // Most cameras have this by default, so let's set it as default.
    IUSaveText(&BayerT[2], "RGGB");

    SetCCDCapability(CCD_CAN_SUBFRAME | CCD_HAS_BAYER | CCD_HAS_STREAMING);

    SetFocuserCapability(FOCUSER_HAS_VARIABLE_SPEED);

    FocusSpeedN[0].min   = 0;
    FocusSpeedN[0].max   = 3;
    FocusSpeedN[0].step  = 1;
    FocusSpeedN[0].value = 1;

    /* JM 2014-05-20 Make PrimaryCCD.ImagePixelSizeNP writable since we can't know for now the pixel size and bit depth from gphoto */
    PrimaryCCD.getCCDInfo()->p = IP_RW;

    setDriverInterface(getDriverInterface() | FOCUSER_INTERFACE);

    gphoto_set_debug(getDeviceName());
    gphoto_read_set_debug(getDeviceName());

    return true;
}

void GPhotoCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    defineText(&PortTP);
    loadConfig(true, "DEVICE_PORT");

    if (isConnected())
    {
        if (mExposurePresetSP.nsp > 0)
            defineSwitch(&mExposurePresetSP);
        if (mIsoSP.nsp > 0)
            defineSwitch(&mIsoSP);
        if (mFormatSP.nsp > 0)
            defineSwitch(&mFormatSP);

        defineSwitch(&livePreviewSP);
        defineSwitch(&transferFormatSP);
        defineSwitch(&autoFocusSP);
        defineSwitch(&FocusMotionSP);
        defineNumber(&FocusSpeedNP);
        defineNumber(&FocusTimerNP);

        if (captureTargetSP.s == IPS_OK)
            defineSwitch(&captureTargetSP);

        ShowExtendedOptions();

        if (strstr(gphoto_get_manufacturer(gphotodrv), "Canon"))
            defineNumber(&mMirrorLockNP);
    }

    // Add Debug, Simulator, and Configuration controls
    addAuxControls();
}

bool GPhotoCCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        if (mExposurePresetSP.nsp > 0)
            defineSwitch(&mExposurePresetSP);
        if (mIsoSP.nsp > 0)
            defineSwitch(&mIsoSP);
        if (mFormatSP.nsp > 0)
            defineSwitch(&mFormatSP);

        defineSwitch(&livePreviewSP);
        defineSwitch(&transferFormatSP);
        defineSwitch(&autoFocusSP);
        defineSwitch(&FocusMotionSP);
        defineNumber(&FocusSpeedNP);
        defineNumber(&FocusTimerNP);

        if (captureTargetSP.s == IPS_OK)
            defineSwitch(&captureTargetSP);

        imageBP = getBLOB("CCD1");
        imageB  = imageBP->bp;

        // Dummy values until first capture is done
        //SetCCDParams(1280, 1024, 8, 5.4, 5.4);

        if (sim == false)
        {
            ShowExtendedOptions();

            if (strstr(gphoto_get_manufacturer(gphotodrv), "Canon"))
                defineNumber(&mMirrorLockNP);
        }

        //timerID = SetTimer(POLLMS);
    }
    else
    {
        if (mExposurePresetSP.nsp > 0)
            deleteProperty(mExposurePresetSP.name);
        if (mIsoSP.nsp > 0)
            deleteProperty(mIsoSP.name);
        if (mFormatSP.nsp > 0)
            deleteProperty(mFormatSP.name);

        deleteProperty(mMirrorLockNP.name);
        deleteProperty(livePreviewSP.name);
        deleteProperty(autoFocusSP.name);
        deleteProperty(transferFormatSP.name);
        deleteProperty(FocusMotionSP.name);
        deleteProperty(FocusSpeedNP.name);
        deleteProperty(FocusTimerNP.name);

        if (captureTargetSP.s != IPS_IDLE)
            deleteProperty(captureTargetSP.name);

        HideExtendedOptions();
        //rmTimer(timerID);
    }

    return true;
}

bool GPhotoCCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PortTP.name) == 0)
        {
            PortTP.s = IPS_OK;
            IUUpdateText(&PortTP, texts, names, n);
            IDSetText(&PortTP, NULL);
            return true;
        }

        if (CamOptions.find(name) != CamOptions.end())
        {
            cam_opt *opt = CamOptions[name];
            if (opt->widget->type != GP_WIDGET_TEXT)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "ERROR: Property '%s'is not a string", name);
                return false;
            }
            if (opt->widget->readonly)
            {
                DEBUGF(INDI::Logger::DBG_WARNING, "WARNING: Property %s is read-only", name);
                IDSetText(&opt->prop.text, NULL);
                return false;
            }

            if (IUUpdateText(&opt->prop.text, texts, names, n) < 0)
                return false;
            gphoto_set_widget_text(gphotodrv, opt->widget, texts[0]);
            opt->prop.num.s = IPS_OK;
            IDSetText(&opt->prop.text, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool GPhotoCCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, mIsoSP.name))
        {
            if (IUUpdateSwitch(&mIsoSP, states, names, n) < 0)
                return false;

            for (int i = 0; i < mIsoSP.nsp; i++)
            {
                if (mIsoS[i].s == ISS_ON)
                {
                    if (sim == false)
                        gphoto_set_iso(gphotodrv, i);
                    mIsoSP.s = IPS_OK;
                    IDSetSwitch(&mIsoSP, NULL);
                    break;
                }
            }
        }

        if (!strcmp(name, mExposurePresetSP.name))
        {
            if (IUUpdateSwitch(&mExposurePresetSP, states, names, n) < 0)
                return false;

            mExposurePresetSP.s = IPS_OK;
            IDSetSwitch(&mExposurePresetSP, NULL);

            ISwitch *currentSwitch = IUFindOnSwitch(&mExposurePresetSP);
            if (strcmp(currentSwitch->label, "bulb"))
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Preset %s seconds selected.", currentSwitch->label);

                float duration;
                int num, denom;
                if (sscanf(currentSwitch->label, "%d/%d", &num, &denom) == 2)
                {
                    duration = ((double)num) / ((double)denom);
                    StartExposure(duration);
                }
                else if (sscanf(currentSwitch->label, "%g", &duration) == 1)
                {
                    StartExposure(duration);
                }
            }

            return true;
        }

        // Formats
        if (!strcmp(name, mFormatSP.name))
        {
            int prevSwitch = IUFindOnSwitchIndex(&mFormatSP);
            if (IUUpdateSwitch(&mFormatSP, states, names, n) < 0)
                return false;

            ISwitch *sp = IUFindOnSwitch(&mFormatSP);
            if (sp)
            {
                if (strstr(sp->label, "+"))
                {
                    DEBUGF(INDI::Logger::DBG_ERROR, "%s format is not supported.", sp->label);
                    IUResetSwitch(&mFormatSP);
                    mFormatSP.s                = IPS_ALERT;
                    mFormatSP.sp[prevSwitch].s = ISS_ON;
                    IDSetSwitch(&mFormatSP, NULL);
                    return false;
                }
            }
            for (int i = 0; i < mFormatSP.nsp; i++)
            {
                if (mFormatS[i].s == ISS_ON)
                {
                    if (sim == false)
                        gphoto_set_format(gphotodrv, i);
                    mFormatSP.s = IPS_OK;
                    IDSetSwitch(&mFormatSP, NULL);
                    // We need to get frame W and H if format changes
                    frameInitialized = false;
                    break;
                }
            }
        }

        // How images are transferred to the client
        if (!strcmp(name, transferFormatSP.name))
        {
            IUUpdateSwitch(&transferFormatSP, states, names, n);
            transferFormatSP.s = IPS_OK;
            IDSetSwitch(&transferFormatSP, NULL);
            // We need to get frame W and H if transfer format changes
            // 2017-01-17: Do we? transform format change should not affect W and H
            //frameInitialized = false;
            return true;
        }

        // Autofocus
        if (!strcmp(name, autoFocusSP.name))
        {
            IUResetSwitch(&autoFocusSP);
            char errMsg[MAXRBUF];
            if (gphoto_auto_focus(gphotodrv, errMsg) == GP_OK)
                autoFocusSP.s = IPS_OK;
            else
            {
                autoFocusSP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "%s", errMsg);
            }

            IDSetSwitch(&autoFocusSP, NULL);
            return true;
        }

        // Live preview
#if 0
        if (!strcmp(name, livePreviewSP.name))
        {
            IUUpdateSwitch(&livePreviewSP, states, names, n);

            if (Streamer->isBusy())
            {
                livePreviewS[0].s = ISS_OFF;
                livePreviewS[1].s = ISS_ON;
                livePreviewSP.s   = IPS_ALERT;
                DEBUG(INDI::Logger::DBG_WARNING, "Cannot start live preview while video streaming is active.");
                IDSetSwitch(&livePreviewSP, NULL);
                return true;
            }

            if (livePreviewS[0].s == ISS_ON)
            {
                livePreviewSP.s = IPS_BUSY;
                SetTimer(STREAMPOLLMS);
            }
            else
            {
                stopLivePreview();
                livePreviewSP.s = IPS_IDLE;
            }
            IDSetSwitch(&livePreviewSP, NULL);
            return true;
        }
#endif

        // Capture target
        if (!strcmp(captureTargetSP.name, name))
        {
            const char *onSwitch = IUFindOnSwitchName(states, names, n);
            int captureTarget =
                (!strcmp(onSwitch, captureTargetS[CAPTURE_INTERNAL_RAM].name) ? CAPTURE_INTERNAL_RAM : CAPTURE_SD_CARD);
            int ret = gphoto_set_capture_target(gphotodrv, captureTarget);
            if (ret == GP_OK)
            {
                captureTargetSP.s = IPS_OK;
                IUUpdateSwitch(&captureTargetSP, states, names, n);
                DEBUGF(INDI::Logger::DBG_SESSION, "Capture target set to %s",
                       (captureTarget == CAPTURE_INTERNAL_RAM) ? "Internal RAM" : "SD Card");
            }
            else
            {
                captureTargetSP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_SESSION, "Failed to set capture target set to %s",
                       (captureTarget == CAPTURE_INTERNAL_RAM) ? "Internal RAM" : "SD Card");
            }

            IDSetSwitch(&captureTargetSP, NULL);
            return true;
        }

        if (strstr(name, "FOCUS"))
        {
            return processFocuserSwitch(dev, name, states, names, n);
        }

        if (CamOptions.find(name) != CamOptions.end())
        {
            cam_opt *opt = CamOptions[name];
            if (opt->widget->type != GP_WIDGET_RADIO && opt->widget->type != GP_WIDGET_MENU &&
                opt->widget->type != GP_WIDGET_TOGGLE)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "ERROR: Property '%s'is not a switch (%d)", name, opt->widget->type);
                return false;
            }

            if (opt->widget->readonly)
            {
                DEBUGF(INDI::Logger::DBG_WARNING, "WARNING: Property %s is read-only", name);
                IDSetSwitch(&opt->prop.sw, NULL);
                return false;
            }

            if (IUUpdateSwitch(&opt->prop.sw, states, names, n) < 0)
                return false;

            if (opt->widget->type == GP_WIDGET_TOGGLE)
            {
                gphoto_set_widget_num(gphotodrv, opt->widget, opt->item.sw[ON_S].s == ISS_ON);
            }
            else
            {
                for (int i = 0; i < opt->prop.sw.nsp; i++)
                {
                    if (opt->item.sw[i].s == ISS_ON)
                    {
                        gphoto_set_widget_num(gphotodrv, opt->widget, i);
                        break;
                    }
                }
            }

            opt->prop.sw.s = IPS_OK;
            IDSetSwitch(&opt->prop.sw, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool GPhotoCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strstr(name, "FOCUS_"))
            return processFocuserNumber(dev, name, values, names, n);

        if (!strcmp(name, mMirrorLockNP.name))
        {
            IUUpdateNumber(&mMirrorLockNP, values, names, n);
            mMirrorLockNP.s = IPS_OK;
            IDSetNumber(&mMirrorLockNP, NULL);
            return true;
        }

        if (CamOptions.find(name) != CamOptions.end())
        {
            cam_opt *opt = CamOptions[name];
            if (opt->widget->type != GP_WIDGET_RANGE)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "ERROR: Property '%s'is not a string", name);
                return false;
            }
            if (opt->widget->readonly)
            {
                DEBUGF(INDI::Logger::DBG_WARNING, "WARNING: Property %s is read-only", name);
                return false;
            }
            if (IUUpdateNumber(&opt->prop.num, values, names, n) < 0)
                return false;
            gphoto_set_widget_num(gphotodrv, opt->widget, values[0]);
            opt->prop.num.s = IPS_OK;
            IDSetNumber(&opt->prop.num, NULL);
            return true;
        }
    }

    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool GPhotoCCD::Connect()
{
    sim = isSimulation();

    int setidx;
    char **options;
    int max_opts;
    const char *shutter_release_port = NULL;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Mirror lock value: %f", mMirrorLockN[0].value);

    if (PortTP.tp[0].text && strlen(PortTP.tp[0].text))
    {
        shutter_release_port = PortTP.tp[0].text;
    }

    if (sim == false)
    {
        // Regular detect
        if (port[0] == '\0')
            gphotodrv = gphoto_open(camera, context, NULL, NULL, shutter_release_port);
        else
            gphotodrv = gphoto_open(camera, context, model, port, shutter_release_port);
        if (gphotodrv == NULL)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Can not open camera: Power OK? If camera is auto-mounted as external disk "
                                           "storage, please unmount it and disable auto-mount.");
            return false;
        }
    }

    if (sim)
    {
        PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.001, 3600, 1, true);
    }
    else
    {
        double min_exposure = 0.001, max_exposure = 3600;

        gphoto_get_minmax_exposure(gphotodrv, &min_exposure, &max_exposure);
        PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", min_exposure, max_exposure, 1, true);
    }

    if (mFormatS)
    {
        free(mFormatS);
        mFormatS = NULL;
    }

    if (sim)
    {
        setidx             = 0;
        max_opts           = 1;
        const char *fmts[] = { "Custom" };
        options            = (char **)fmts;
    }
    else
    {
        setidx  = gphoto_get_format_current(gphotodrv);
        options = gphoto_get_formats(gphotodrv, &max_opts);
    }

    if (!sim && max_opts > 0)
    {
        mFormatS      = create_switch("FORMAT", options, max_opts, setidx);
        mFormatSP.sp  = mFormatS;
        mFormatSP.nsp = max_opts;

        ISwitch *sp = IUFindOnSwitch(&mFormatSP);
        if (sp && strstr(sp->label, "+"))
        {
            IUResetSwitch(&mFormatSP);
            int i = 0;

            // Prefer RAW format in case selected format is not supported.
            for (i = 0; i < mFormatSP.nsp; i++)
            {
                if (!strcmp("RAW", mFormatSP.sp[i].label))
                {
                    mFormatS[i].s = ISS_ON;
                    break;
                }
            }

            if (i == mFormatSP.nsp)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "%s format is not supported. Please select another format.", sp->label);
                mFormatSP.s = IPS_ALERT;
            }

            IDSetSwitch(&mFormatSP, NULL);
        }
    }

    if (mIsoS)
        free(mIsoS);

    if (sim)
    {
        setidx             = 0;
        max_opts           = 4;
        const char *isos[] = { "100", "200", "400", "800" };
        options            = (char **)isos;
    }
    else
    {
        setidx  = gphoto_get_iso_current(gphotodrv);
        options = gphoto_get_iso(gphotodrv, &max_opts);
    }

    if (!sim)
    {
        mIsoS      = create_switch("ISO", options, max_opts, setidx);
        mIsoSP.sp  = mIsoS;
        mIsoSP.nsp = max_opts;
    }

    if (mExposurePresetS)
    {
        free(mExposurePresetS);
        mExposurePresetS = NULL;
    }

    if (sim)
    {
        setidx                     = 0;
        max_opts                   = 4;
        const char *exposureList[] = { "1/8", "1/4", "1/2", "bulb" };
        options                    = (char **)exposureList;
    }
    else
    {
        setidx   = 0;
        max_opts = 0;
        options  = gphoto_get_exposure_presets(gphotodrv, &max_opts);
    }

    if (!sim && max_opts > 0)
    {
        mExposurePresetS      = create_switch("EXPOSURE_PRESET", options, max_opts, setidx);
        mExposurePresetSP.sp  = mExposurePresetS;
        mExposurePresetSP.nsp = max_opts;
    }

    // Get Capture target
    int captureTarget = -1;

    if (!sim && gphoto_get_capture_target(gphotodrv, &captureTarget) == GP_OK)
    {
        IUResetSwitch(&captureTargetSP);
        captureTargetS[CAPTURE_INTERNAL_RAM].s = (captureTarget == 0) ? ISS_ON : ISS_OFF;
        captureTargetS[CAPTURE_SD_CARD].s      = (captureTarget == 1) ? ISS_ON : ISS_OFF;
        captureTargetSP.s                      = IPS_OK;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "%s is online.", getDeviceName());

    if (!sim && gphoto_get_manufacturer(gphotodrv) && gphoto_get_model(gphotodrv))
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Detected %s Model %s.", gphoto_get_manufacturer(gphotodrv),
               gphoto_get_model(gphotodrv));
    }

    frameInitialized = false;

    return true;
}

bool GPhotoCCD::Disconnect()
{
    if (sim)
        return true;
    gphoto_close(gphotodrv);
    gphotodrv        = NULL;
    frameInitialized = false;
    DEBUGF(INDI::Logger::DBG_SESSION, "%s is offline.", getDeviceName());
    return true;
}

bool GPhotoCCD::StartExposure(float duration)
{
    if (PrimaryCCD.getPixelSizeX() == 0)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Please update the CCD Information in the Image Info section before "
                                         "proceeding. The camera resolution shall be updated after the first exposure "
                                         "is complete.");
        return false;
    }

    if (PrimaryCCD.isExposing())
    {
        DEBUG(INDI::Logger::DBG_ERROR, "GPhoto driver is already exposing. Can not abort.");
        return false;
    }

    if (mFormatS != NULL)
    {
        ISwitch *sp = IUFindOnSwitch(&mFormatSP);
        if (sp == NULL)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Please select a format before capturing an image.");
            return false;
        }
    }

    /* start new exposure with last ExpValues settings.
     * ExpGo goes busy. set timer to read when done
     */

    // Microseconds
    int exp_us = (int)ceil(duration * 1e6);

    PrimaryCCD.setExposureDuration(duration);

    if (sim == false && gphoto_start_exposure(gphotodrv, exp_us, mMirrorLockN[0].value) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error starting exposure");
        return false;
    }

    ExposureRequest = duration;
    gettimeofday(&ExpStart, NULL);
    InExposure = true;

    DEBUGF(INDI::Logger::DBG_SESSION, "Starting %g sec exposure", duration);

    SetTimer(POLLMS);

    return true;
}

bool GPhotoCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    if (transferFormatS[0].s != ISS_ON)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Subframing is only supported in FITS transport mode.");
        return false;
    }

    PrimaryCCD.setFrame(x, y, w, h);
    return true;
}

float GPhotoCCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, NULL);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = ExposureRequest - timesince;
    return timeleft;
}

void GPhotoCCD::TimerHit()
{
    long timeleft = 1e6;
    int timerID   = -1;

    if (isConnected() == false)
        return;

    if (Streamer->isBusy())
    {
        bool rc = startLiveVideo();

        if (rc)
            timerID = SetTimer(STREAMPOLLMS);
        else
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Error capturing video stream.");
            Streamer->setStream(false);
        }
    }

/*
    if (livePreviewSP.s == IPS_BUSY)
    {
        bool rc = startLivePreview();

        if (rc)
            timerID = SetTimer(STREAMPOLLMS);
        else
        {
            livePreviewSP.s = IPS_ALERT;
            DEBUG(INDI::Logger::DBG_ERROR, "Error capturing preview.");
            livePreviewS[0].s = ISS_OFF;
            livePreviewS[1].s = ISS_ON;
            IDSetSwitch(&livePreviewSP, NULL);
        }
    }
*/

    if (FocusTimerNP.s == IPS_BUSY)
    {
        char errMsg[MAXRBUF];
        if (gphoto_manual_focus(gphotodrv, focusSpeed, errMsg) != GP_OK)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Focusing failed: %s", errMsg);
            FocusTimerNP.s       = IPS_ALERT;
            FocusTimerN[0].value = 0;
        }
        else
        {
            FocusTimerN[0].value -= FOCUS_TIMER;
            if (FocusTimerN[0].value <= 0)
            {
                FocusTimerN[0].value = 0;
                FocusTimerNP.s       = IPS_OK;
            }
            else if (timerID == -1)
                timerID = SetTimer(FOCUS_TIMER);
        }

        IDSetNumber(&FocusTimerNP, NULL);
    }

    if (InExposure)
    {
        timeleft = CalcTimeLeft();

        if (timeleft < 1.0)
        {
            if (timeleft > 0.25 && timerID == -1)
            {
                //  a quarter of a second or more
                //  just set a tighter timer
                timerID = SetTimer(250);
            }
            else
            {
                if (timeleft > 0.07 && timerID == -1)
                {
                    //  use an even tighter timer
                    timerID = SetTimer(50);
                }
                else
                {
                    //  it's real close now, so spin on it
                    while (!sim && timeleft > 0)
                    {
                        int slv;
                        slv = 100000 * timeleft;
                        usleep(slv);
                    }

                    PrimaryCCD.setExposureLeft(0);
                    InExposure = false;
                    /* grab and save image */
                    bool rc = grabImage();
                    if (rc == false)
                    {
                        PrimaryCCD.setExposureFailed();
                    }
                }
            }
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Capture in progress. Time left %ld", timeleft);
            if (timerID == -1)
                SetTimer(POLLMS);
        }

        PrimaryCCD.setExposureLeft(timeleft);
    }
}

void GPhotoCCD::UpdateExtendedOptions(void *p)
{
    GPhotoCCD *cam = (GPhotoCCD *)p;
    cam->UpdateExtendedOptions();
}

void GPhotoCCD::UpdateExtendedOptions(bool force)
{
    if (!expTID)
    {
        for (auto& option : CamOptions)
        {
            cam_opt *opt = option.second;

            if (force || gphoto_widget_changed(opt->widget))
            {
                gphoto_read_widget(opt->widget);
                UpdateWidget(opt);
            }
        }
    }

    optTID = IEAddTimer(1000, GPhotoCCD::UpdateExtendedOptions, this);
}

bool GPhotoCCD::grabImage()
{
    //char ext[16];
    uint8_t *memptr = PrimaryCCD.getFrameBuffer();
    size_t memsize = 0;
    int fd = 0, naxis = 2, w = 0, h = 0, bpp = 8;

    if (sim)
    {
        w                   = PrimaryCCD.getXRes();
        h                   = PrimaryCCD.getYRes();
        size_t fullbuf_size = w * h + 512;
        uint8_t *fullbuf    = (uint8_t *)malloc(fullbuf_size);
        for (int i = 0; i < h; i++)
            for (int j = 0; j < w; j++)
                fullbuf[i * w + j] = rand() % 255;

        // Starting address if subframing
        memptr  = fullbuf + (PrimaryCCD.getSubY() * PrimaryCCD.getXRes()) + PrimaryCCD.getSubX();
        memsize = PrimaryCCD.getSubW() * PrimaryCCD.getSubH() * PrimaryCCD.getBPP() / 8;

        PrimaryCCD.setFrameBuffer(memptr);
        PrimaryCCD.setFrameBufferSize(memsize, false);
        //PrimaryCCD.setResolution(w, h);
        //PrimaryCCD.setFrame(0, 0, w, h);
        PrimaryCCD.setNAxis(naxis);
        PrimaryCCD.setBPP(bpp);

        ExposureComplete(&PrimaryCCD);
        return true;
    }

    // If only save to SD Card, let's not upload back to client
    /*if (UploadS[GP_UPLOAD_SDCARD].s == ISS_ON)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "Exposure complete. Image saved to SD Card.");
        ExposureComplete(&PrimaryCCD);
        return true;
    }*/

    if (transferFormatS[0].s == ISS_ON)
    {
        char tmpfile[] = "/tmp/indi_XXXXXX";

        //dcraw can't read from stdin, so we need to write to disk then read it back
        fd = mkstemp(tmpfile);

        int ret = gphoto_read_exposure_fd(gphotodrv, fd);

        if (ret != GP_OK || fd == -1)
        {
            if (fd == -1)
                DEBUGF(INDI::Logger::DBG_ERROR, "Exposure failed to save image. Cannot create temp file %s", tmpfile);
            else
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Exposure failed to save image... %s", gp_result_as_string(ret));
                // As suggested on INDI forums, this result could be misleading.
                if (ret == GP_ERROR_DIRECTORY_NOT_FOUND)
                    DEBUG(INDI::Logger::DBG_SESSION, "Make sure BULB switch is ON in the camera. Try setting AF switch to OFF.");
            }
            unlink(tmpfile);
            return false;
        }

        if (!strcmp(gphoto_get_file_extension(gphotodrv), "unknown"))
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Exposure failed.");
            return false;
        }

        /* We're done exposing */
        DEBUG(INDI::Logger::DBG_SESSION, "Exposure done, downloading image...");

        if (strcasecmp(gphoto_get_file_extension(gphotodrv), "jpg") == 0 ||
            strcasecmp(gphoto_get_file_extension(gphotodrv), "jpeg") == 0)
        {
            if (read_jpeg(tmpfile, &memptr, &memsize, &naxis, &w, &h))
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Exposure failed to parse jpeg.");
                unlink(tmpfile);
                return false;
            }

            DEBUGF(INDI::Logger::DBG_DEBUG, "read_jpeg: memsize (%d) naxis (%d) w (%d) h (%d) bpp (%d)", memsize, naxis,
                   w, h, bpp);

            SetCCDCapability(GetCCDCapability() & ~CCD_HAS_BAYER);
        }
        else
        {
            /*if (read_dcraw(tmpfile, &memptr, &memsize, &naxis, &w, &h, &bpp))
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Exposure failed to parse raw image.");
                    unlink(tmpfile);
                    return false;
                }

                DEBUGF(INDI::Logger::DBG_DEBUG, "read_dcraw: memsize (%d) naxis (%d) w (%d) h (%d) bpp (%d)", memsize, naxis, w, h, bpp);*/

            char bayer_pattern[8] = {};

            if (read_libraw(tmpfile, &memptr, &memsize, &naxis, &w, &h, &bpp, bayer_pattern))
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Exposure failed to parse raw image.");
                unlink(tmpfile);
                return false;
            }

            DEBUGF(INDI::Logger::DBG_DEBUG, "read_libraw: memsize (%d) naxis (%d) w (%d) h (%d) bpp (%d) pattern (%s)",
                   memsize, naxis, w, h, bpp, bayer_pattern);

            unlink(tmpfile);

            IUSaveText(&BayerT[2], bayer_pattern);
            IDSetText(&BayerTP, NULL);
            SetCCDCapability(GetCCDCapability() | CCD_HAS_BAYER);
        }

        PrimaryCCD.setImageExtension("fits");

        // If subframing is requested
        /*if (frameInitialized &&
                PrimaryCCD.getSubH() < PrimaryCCD.getYRes() || PrimaryCCD.getSubW() < PrimaryCCD.getXRes())*/
        if (PrimaryCCD.getSubH() < w || PrimaryCCD.getSubW() < h)
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Subframing...");

            int subFrameSize     = PrimaryCCD.getSubW() * PrimaryCCD.getSubH() * bpp / 8 * ((naxis == 3) ? 3 : 1);
            int oneFrameSize     = PrimaryCCD.getSubW() * PrimaryCCD.getSubH() * bpp / 8;
            uint8_t *subframeBuf = (uint8_t *)malloc(subFrameSize);

            int startY = PrimaryCCD.getSubY();
            int endY   = startY + PrimaryCCD.getSubH();
            int lineW  = PrimaryCCD.getSubW() * bpp / 8;
            int subX   = PrimaryCCD.getSubX();

            if (naxis == 2)
            {
                for (int i = startY; i < endY; i++)
                    memcpy(subframeBuf + (i - startY) * lineW, memptr + (i * w + subX) * bpp / 8, lineW);
            }
            else
            {
                uint8_t *subR = subframeBuf;
                uint8_t *subG = subframeBuf + oneFrameSize;
                uint8_t *subB = subframeBuf + oneFrameSize * 2;

                uint8_t *startR = memptr;
                uint8_t *startG = memptr + (w * h * bpp / 8);
                uint8_t *startB = memptr + (w * h * bpp / 8 * 2);

                for (int i = startY; i < endY; i++)
                {
                    memcpy(subR + (i - startY) * lineW, startR + (i * w + subX) * bpp / 8, lineW);
                    memcpy(subG + (i - startY) * lineW, startG + (i * w + subX) * bpp / 8, lineW);
                    memcpy(subB + (i - startY) * lineW, startB + (i * w + subX) * bpp / 8, lineW);
                }
            }

            PrimaryCCD.setFrameBuffer(subframeBuf);
            PrimaryCCD.setFrameBufferSize(subFrameSize, false);
            PrimaryCCD.setResolution(w, h);
            PrimaryCCD.setNAxis(naxis);
            PrimaryCCD.setBPP(bpp);

            ExposureComplete(&PrimaryCCD);

            // Restore old pointer and release memory
            PrimaryCCD.setFrameBuffer(memptr);
            PrimaryCCD.setFrameBufferSize(memsize, false);
            free(subframeBuf);
        }
        else
        {
            // We need to initially set the frame dimensions for the first time since it is unknown at the time of connection.
            //frameInitialized = true;

            PrimaryCCD.setFrame(0, 0, w, h);
            PrimaryCCD.setFrameBuffer(memptr);
            PrimaryCCD.setFrameBufferSize(memsize, false);
            PrimaryCCD.setResolution(w, h);
            PrimaryCCD.setNAxis(naxis);
            PrimaryCCD.setBPP(bpp);

            ExposureComplete(&PrimaryCCD);
        }
    }
    else
    {
        int rc = gphoto_read_exposure(gphotodrv);

        if (rc != 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to expose.");
            if (strstr(gphoto_get_manufacturer(gphotodrv), "Canon") && mMirrorLockN[0].value == 0)
                DEBUG(INDI::Logger::DBG_WARNING,
                      "If your camera mirror lock is enabled, you must set a value for the mirror locking duration.");
            return false;
        }

        /* We're done exposing */
        DEBUG(INDI::Logger::DBG_DEBUG, "Exposure done, downloading image...");
        uint8_t *newMemptr = NULL;
        gphoto_get_buffer(gphotodrv, (const char **)&newMemptr, &memsize);
        memptr = (uint8_t *)realloc(memptr,
                                    memsize); // We copy the obtained memory pointer to avoid freeing some gphoto memory
        memcpy(memptr, newMemptr, memsize);   //

        gphoto_get_dimensions(gphotodrv, &w, &h);

        PrimaryCCD.setImageExtension(gphoto_get_file_extension(gphotodrv));
        if (w > 0 && h > 0)
            PrimaryCCD.setFrame(0, 0, w, h);
        PrimaryCCD.setFrameBuffer(memptr);
        PrimaryCCD.setFrameBufferSize(memsize, false);
        if (w > 0 && h > 0)
            PrimaryCCD.setResolution(w, h);
        PrimaryCCD.setNAxis(naxis);
        PrimaryCCD.setBPP(bpp);

        ExposureComplete(&PrimaryCCD);
    }

    return true;
}

ISwitch *GPhotoCCD::create_switch(const char *basestr, char **options, int max_opts, int setidx)
{
    int i;
    ISwitch *sw     = (ISwitch *)calloc(sizeof(ISwitch), max_opts);
    ISwitch *one_sw = sw;

    char sw_name[MAXINDINAME];
    char sw_label[MAXINDILABEL];
    ISState sw_state;

    for (i = 0; i < max_opts; i++)
    {
        snprintf(sw_name, MAXINDINAME, "%s%d", basestr, i);
        strncpy(sw_label, options[i], MAXINDILABEL);
        sw_state = (i == setidx) ? ISS_ON : ISS_OFF;

        IUFillSwitch(one_sw++, sw_name, sw_label, sw_state);
    }
    return sw;
}

void GPhotoCCD::UpdateWidget(cam_opt *opt)
{
    struct tm *tm;

    switch (opt->widget->type)
    {
        case GP_WIDGET_RADIO:
        case GP_WIDGET_MENU:
            for (int i = 0; i < opt->widget->choice_cnt; i++)
                opt->item.sw[i].s = opt->widget->value.index == i ? ISS_ON : ISS_OFF;
            IDSetSwitch(&opt->prop.sw, NULL);
            break;
        case GP_WIDGET_TEXT:
            free(opt->item.text.text);
            opt->item.text.text = strdup(opt->widget->value.text);
            IDSetText(&opt->prop.text, NULL);
            break;
        case GP_WIDGET_TOGGLE:
            if (opt->widget->value.toggle)
            {
                opt->item.sw[0].s = ISS_ON;
                opt->item.sw[0].s = ISS_OFF;
            }
            else
            {
                opt->item.sw[0].s = ISS_OFF;
                opt->item.sw[0].s = ISS_ON;
            }
            IDSetSwitch(&opt->prop.sw, NULL);
            break;
        case GP_WIDGET_RANGE:
            opt->item.num.value = opt->widget->value.num;
            IDSetNumber(&opt->prop.num, NULL);
            break;
        case GP_WIDGET_DATE:
            free(opt->item.text.text);
            tm                  = gmtime((time_t *)&opt->widget->value.date);
            opt->item.text.text = strdup(asctime(tm));
            IDSetText(&opt->prop.text, NULL);
            break;
        default:
            delete opt;
            return;
    }
}

void GPhotoCCD::AddWidget(gphoto_widget *widget)
{
    IPerm perm;
    struct tm *tm;

    if (!widget)
        return;

    perm = widget->readonly ? IP_RO : IP_RW;

    cam_opt *opt = new cam_opt();
    opt->widget  = widget;

    switch (widget->type)
    {
        case GP_WIDGET_RADIO:
        case GP_WIDGET_MENU:
            opt->item.sw = create_switch(widget->name, widget->choices, widget->choice_cnt, widget->value.index);
            IUFillSwitchVector(&opt->prop.sw, opt->item.sw, widget->choice_cnt, getDeviceName(), widget->name,
                               widget->name, widget->parent, perm, ISR_1OFMANY, 60, IPS_IDLE);
            IDDefSwitch(&opt->prop.sw, NULL);
            break;
        case GP_WIDGET_TEXT:
            IUFillText(&opt->item.text, widget->name, widget->name, widget->value.text);
            IUFillTextVector(&opt->prop.text, &opt->item.text, 1, getDeviceName(), widget->name, widget->name,
                             widget->parent, perm, 60, IPS_IDLE);
            IDDefText(&opt->prop.text, NULL);
            break;
        case GP_WIDGET_TOGGLE:
            opt->item.sw = create_switch(widget->name, (char **)on_off, 2, widget->value.toggle ? 0 : 1);
            IUFillSwitchVector(&opt->prop.sw, opt->item.sw, 2, getDeviceName(), widget->name, widget->name,
                               widget->parent, perm, ISR_1OFMANY, 60, IPS_IDLE);
            IDDefSwitch(&opt->prop.sw, NULL);
            break;
        case GP_WIDGET_RANGE:
            IUFillNumber(&opt->item.num, widget->name, widget->name, "%5.2f", widget->min, widget->max, widget->step,
                         widget->value.num);
            IUFillNumberVector(&opt->prop.num, &opt->item.num, 1, getDeviceName(), widget->name, widget->name,
                               widget->parent, perm, 60, IPS_IDLE);
            break;
        case GP_WIDGET_DATE:
            tm = gmtime((time_t *)&widget->value.date);
            IUFillText(&opt->item.text, widget->name, widget->name, asctime(tm));
            IUFillTextVector(&opt->prop.text, &opt->item.text, 1, getDeviceName(), widget->name, widget->name,
                             widget->parent, perm, 60, IPS_IDLE);
            IDDefText(&opt->prop.text, NULL);
            break;
        default:
            delete opt;
            return;
    }

    CamOptions[widget->name] = opt;
}

void GPhotoCCD::ShowExtendedOptions(void)
{
    gphoto_widget_list *iter;

    iter = gphoto_find_all_widgets(gphotodrv);
    while (iter)
    {
        gphoto_widget *widget = gphoto_get_widget_info(gphotodrv, &iter);
        AddWidget(widget);
    }

    gphoto_show_options(gphotodrv);

    optTID = IEAddTimer(1000, GPhotoCCD::UpdateExtendedOptions, this);
}

void GPhotoCCD::HideExtendedOptions(void)
{
    if (optTID)
    {
        IERmTimer(optTID);
        optTID = 0;
    }

    while (CamOptions.begin() != CamOptions.end())
    {
        cam_opt *opt = (*CamOptions.begin()).second;
        IDDelete(getDeviceName(), (*CamOptions.begin()).first.c_str(), NULL);

        switch (opt->widget->type)
        {
            case GP_WIDGET_RADIO:
            case GP_WIDGET_MENU:
            case GP_WIDGET_TOGGLE:
                free(opt->item.sw);
                break;
            case GP_WIDGET_TEXT:
            case GP_WIDGET_DATE:
                free(opt->item.text.text);
                break;
            default:
                break;
        }

        delete opt;
        CamOptions.erase(CamOptions.begin());
    }
}

IPState GPhotoCCD::MoveFocuser(FocusDirection dir, int speed, uint16_t duration)
{
    INDI_UNUSED(duration);

    /* gphoto works with steps */

    if (sim || speed == 0)
        return IPS_OK;

    if (dir == FOCUS_INWARD)
        focusSpeed = speed * -1;
    else
        focusSpeed = speed;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Setting focuser speed to %d", focusSpeed);

    /*while (duration-->0)
   {
       if ( gphoto_manual_focus(gphotodrv, focusSpeed, errMsg) != GP_OK)
       {
           DEBUGF(INDI::Logger::DBG_ERROR, "Focusing failed: %s", errMsg);
           return IPS_ALERT;
       }
   }*/

    SetTimer(FOCUS_TIMER);

    return IPS_BUSY;
}

bool GPhotoCCD::SetFocuserSpeed(int speed)
{
    if (speed >= FocusSpeedN[0].min && speed <= FocusSpeedN[0].max)
        return true;

    return false;
}

bool GPhotoCCD::StartStreaming()
{
    if (livePreviewSP.s == IPS_BUSY)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot start live video streaming while live preview is on.");
        return false;
    }

    Streamer->setPixelFormat(INDI_JPG);
    SetTimer(STREAMPOLLMS);
    return true;
}

bool GPhotoCCD::StopStreaming()
{
    stopLiveVideo();
    return true;
}

bool GPhotoCCD::startLiveVideo()
{
    //static int last_naxis = -1;, last_w = -1, last_h = -1;

    if (sim)
        return false;

    int rc = GP_OK;
    char errMsg[MAXRBUF];

    const char *previewData = nullptr;
    unsigned long int previewSize = 0;

    CameraFile *previewFile = NULL;

    rc = gp_file_new(&previewFile);
    if (rc != GP_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error creating gphoto file: %s", gp_result_as_string(rc));
        return false;
    }

    for (int i = 0; i < MAX_RETRIES; i++)
    {
        rc = gphoto_capture_preview(gphotodrv, previewFile, errMsg);
        if (rc == true)
            break;
    }

    if (rc != GP_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errMsg);
        return false;
    }

    if (rc >= GP_OK)
    {
        rc = gp_file_get_data_and_size(previewFile, &previewData, &previewSize);
        if (rc != GP_OK)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error getting preview image data and size: %s", gp_result_as_string(rc));
            return false;
        }
    }

    /*if (PrimaryCCD.getSubW() != w || PrimaryCCD.getSubH() != h)
    {
        Streamer->setSize(w, h);
        PrimaryCCD.setFrame(0, 0, w, h);
    }*/

    int w=0,h=0;
    unsigned char *inBuffer = (unsigned char *)(const_cast<char *>(previewData));
    read_jpeg_size(inBuffer, previewSize, &w, &h);
    Streamer->setSize(w,h);
    Streamer->newFrame(inBuffer, previewSize);

#if 0

    uint8_t *ccdBuffer      = PrimaryCCD.getFrameBuffer();
    unsigned char *inBuffer = (unsigned char *)(const_cast<char *>(previewData));
    size_t size             = 0;
    int w=0, h=0, naxis=0;

    // Read jpeg from memory
    rc = read_jpeg_mem(inBuffer, previewSize, &ccdBuffer, &size, &naxis, &w, &h);

    if (rc != 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error getting live video frame.");

        if (previewFile)
        {
            gp_file_unref(previewFile);
            previewFile = NULL;
        }
        return false;
    }

    PrimaryCCD.setFrameBuffer(ccdBuffer);

    if (naxis != PrimaryCCD.getNAxis())
    {
        if (naxis == 3)
            Streamer->setPixelFormat(INDI_JPG);
        else
            Streamer->setPixelFormat(INDI_MONO);

        PrimaryCCD.setNAxis(naxis);
    }

    //if (last_w != w || last_h != h)
    if (PrimaryCCD.getSubW() != w || PrimaryCCD.getSubH() != h)
    {
        Streamer->setSize(w, h);
        PrimaryCCD.setFrame(0, 0, w, h);
    }

    if (PrimaryCCD.getFrameBufferSize() != static_cast<int>(size))
        PrimaryCCD.setFrameBufferSize(size, false);

    if (previewFile)
    {
        gp_file_unref(previewFile);
        previewFile = NULL;
    }

    Streamer->newFrame();
#endif

    return true;
}

#if 0
bool GPhotoCCD::startLivePreview()
{
    if (sim)
        return false;

    int rc = GP_OK;
    char errMsg[MAXRBUF];
    const char *previewData = nullptr;
    unsigned long int previewSize = 0;

    CameraFile *previewFile = NULL;

    rc = gp_file_new(&previewFile);
    if (rc != GP_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error creating gphoto file: %s", gp_result_as_string(rc));
        return false;
    }

    for (int i = 0; i < MAX_RETRIES; i++)
    {
        rc = gphoto_capture_preview(gphotodrv, previewFile, errMsg);
        if (rc == true)
            break;
    }

    if (rc != GP_OK)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errMsg);
        return false;
    }

    if (rc >= GP_OK)
    {
        rc = gp_file_get_data_and_size(previewFile, &previewData, &previewSize);
        if (rc != GP_OK)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error getting preview image data and size: %s", gp_result_as_string(rc));
            return false;
        }
    }

    //DEBUGF(INDI::Logger::DBG_DEBUG, "Preview capture size %d bytes.", previewSize);

    char *previewBlob = (char *)previewData;

    imageB->blob    = previewBlob;
    imageB->bloblen = previewSize;
    imageB->size    = previewSize;
    strncpy(imageB->format, "stream_jpeg", MAXINDIBLOBFMT);

    IDSetBLOB(imageBP, NULL);

    if (previewFile)
    {
        gp_file_unref(previewFile);
        previewFile = NULL;
    }

    return true;
}
#endif

bool GPhotoCCD::stopLiveVideo()
{
    if (sim)
        return false;

    return (gphoto_stop_preview(gphotodrv) == GP_OK);
}

bool GPhotoCCD::saveConfigItems(FILE *fp)
{
    // First save Device Port
    IUSaveConfigText(fp, &PortTP);

    // Second save the CCD Info property
    IUSaveConfigNumber(fp, PrimaryCCD.getCCDInfo());

    // Save regular CCD properties
    INDI::CCD::saveConfigItems(fp);

    // Mirror Locking
    IUSaveConfigNumber(fp, &mMirrorLockNP);

    // Capture Target
    if (captureTargetSP.s == IPS_OK)
        IUSaveConfigSwitch(fp, &captureTargetSP);

    // ISO Settings
    if (mIsoSP.nsp > 0)
        IUSaveConfigSwitch(fp, &mIsoSP);

    // Format Settings
    if (mFormatSP.nsp > 0)
        IUSaveConfigSwitch(fp, &mFormatSP);

    // Transfer Format
    IUSaveConfigSwitch(fp, &transferFormatSP);

    return true;
}

void GPhotoCCD::addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip)
{
    INDI::CCD::addFITSKeywords(fptr, targetChip);

    int status = 0;

    if (mIsoSP.nsp > 0)
    {
        ISwitch *onISO = IUFindOnSwitch(&mIsoSP);
        if (onISO)
        {
            int isoSpeed = -1;
            isoSpeed     = atoi(onISO->label);
            if (isoSpeed > 0)
                fits_update_key_s(fptr, TUINT, "ISOSPEED", &isoSpeed, "ISO Speed", &status);
        }
    }
}

bool GPhotoCCD::UpdateCCDUploadMode(CCD_UPLOAD_MODE mode)
{
    if (!sim)
        gphoto_set_upload_settings(gphotodrv, mode);
    return true;
}
