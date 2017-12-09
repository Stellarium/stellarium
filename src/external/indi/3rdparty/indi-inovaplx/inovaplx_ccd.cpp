/*
   INDI Driver for i-Nova PLX series
   Copyright 2013/2014 i-Nova Technologies - Ilia Platone

   Copyright (C) 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)
*/

#include <stdlib.h>
#include <sys/file.h>
#include <memory>
#include "inovaplx_ccd.h"

int timerNS = -1;
int timerWE = -1;
unsigned char DIR		  = 0xF;
//unsigned char OLD_DIR	  = 0xF;
const int POLLMS		   = 500;	   /* Polling interval 500 ms */
//const int MAX_CCD_GAIN	 = 1023;		/* Max CCD gain */
//const int MIN_CCD_GAIN	 = 0;		/* Min CCD gain */
//const int MAX_CCD_KLEVEL   = 255;		/* Max CCD black level */
//const int MIN_CCD_KLEVEL   = 0;		/* Min CCD black level */

/* Macro shortcut to CCD values */
//#define TEMP_FILE "/tmp/inovaInstanceNumber.tmp"
//INovaCCD *inova = NULL;
//static int isInit = 0;
//extern char *__progname;

std::unique_ptr<INovaCCD> inova(new INovaCCD());

static void * capture_Thread(void * arg)
{
    INDI_UNUSED(arg);
    inova->CaptureThread();
    return nullptr;
}

static void timerWestEast(void * arg)
{
    INDI_UNUSED(arg);
    DIR |= 0x09;
    iNovaSDK_SendST4(DIR);
    IERmCallback(timerWE);
}

static void timerNorthSouth(void * arg)
{
    INDI_UNUSED(arg);
    DIR |= 0x06;
    iNovaSDK_SendST4(DIR);
    IERmCallback(timerNS);
}

void ISGetProperties(const char *dev)
{    
    inova->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    inova->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(	const char *dev, const char *name, char *texts[], char *names[], int num)
{
    inova->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    inova->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB (const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
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

void ISSnoopDevice (XMLEle *root)
{
    inova->ISSnoopDevice(root);
}

INovaCCD::INovaCCD()
{
    ExposureRequest = 0.0;
    InExposure = false;
}

bool INovaCCD::Connect()
{
    const char *Sn;
    if(iNovaSDK_MaxCamera() > 0)
    {
        Sn = iNovaSDK_OpenCamera(1);
        DEBUGF(INDI::Logger::DBG_DEBUG, "Serial Number: %s", Sn);
        if(Sn[0] >= '0' && Sn[0] < '3')
        {
            iNovaSDK_InitST4();
            DEBUGF(INDI::Logger::DBG_SESSION, "Camera model is %s", iNovaSDK_GetName());
            iNovaSDK_InitCamera(RESOLUTION_FULL);
            //maxW = iNovaSDK_GetImageWidth();
            //maxH = iNovaSDK_GetImageHeight();

            iNovaSDK_SetFrameSpeed(FRAME_SPEED_LOW);
            iNovaSDK_CancelLongExpTime();
            iNovaSDK_OpenVideo();

            threadsRunning = true;

            RawData = (unsigned char *)malloc(iNovaSDK_GetArraySize() * (iNovaSDK_GetDataWide() > 0 ? 2 : 1));
            pthread_create(&captureThread, NULL, capture_Thread, (void*)this);

            CameraPropertiesNP.s = IPS_IDLE;

            // Set camera capabilities
            uint32_t cap = CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | (iNovaSDK_HasColorSensor() ? CCD_HAS_BAYER : 0) | (iNovaSDK_HasST4() ? CCD_HAS_ST4_PORT : 0);
            SetCCDCapability(cap);

            return true;
        }
        iNovaSDK_CloseCamera();
    }
    DEBUG(INDI::Logger::DBG_ERROR, "No cameras opened.");
    return false;
}

bool INovaCCD::Disconnect()
{
    threadsRunning = false;
    pthread_join(captureThread, NULL);
    iNovaSDK_SensorPowerDown();
    iNovaSDK_CloseVideo();
    iNovaSDK_CloseCamera();
    return true;
}

const char * INovaCCD::getDefaultName()
{
    return "iNova PLX";
}

const char * INovaCCD::getDeviceName()
{
    return getDefaultName();
}

/**************************************************************************************
** INDI is asking us to init our properties.
***************************************************************************************/
bool INovaCCD::initProperties()
{
    // Must init parent properties first!
    INDI::CCD::initProperties();

    // We init the property details. This is a stanard property of the INDI Library.
    IUFillText(&iNovaInformationT[0], "INOVA_NAME", "Camera Name", "");
    IUFillText(&iNovaInformationT[1], "INOVA_SENSOR_NAME", "Sensor Name", "");
    IUFillText(&iNovaInformationT[2], "INOVA_SN", "Serial Number", "");
    IUFillText(&iNovaInformationT[3], "INOVA_ST4", "Can Guide", "");
    IUFillText(&iNovaInformationT[4], "INOVA_COLOR", "Color Sensor", "");
    IUFillTextVector(&iNovaInformationTP, iNovaInformationT, 5, getDeviceName(), "INOVA_INFO", "iNova Info", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    IUFillNumber(&CameraPropertiesN[CCD_GAIN_N], "CCD_GAIN_VALUE", "Gain", "%4.0f", 0, 1023, 1, 255);
    IUFillNumber(&CameraPropertiesN[CCD_BLACKLEVEL_N], "CCD_BLACKLEVEL_VALUE", "Black Level", "%3.0f", 0, 255, 1, 0);
    IUFillNumberVector(&CameraPropertiesNP, CameraPropertiesN, 2, getDeviceName(), "CCD_PROPERTIES", "Control", MAIN_CONTROL_TAB, IP_RW, 60, IPS_IDLE);

    // Set minimum exposure speed to 0.001 seconds
    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.0001, 1000, 1, false);

    return true;

}

/**************************************************************************************
** INDI is asking us to submit list of properties for the device
***************************************************************************************/
void INovaCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    if (isConnected())
    {
        // Define our properties
        defineText(&iNovaInformationTP);
        defineNumber(&CameraPropertiesNP);
    }
}

/********************************************************************************************
** INDI is asking us to update the properties because there is a change in CONNECTION status
** This fucntion is called whenever the device is connected or disconnected.
*********************************************************************************************/
bool INovaCCD::updateProperties()
{
    // Call parent update properties
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Define our properties
        IUSaveText(&iNovaInformationT[0], iNovaSDK_GetName());
        IUSaveText(&iNovaInformationT[1], iNovaSDK_SensorName());
        IUSaveText(&iNovaInformationT[2], iNovaSDK_SerialNumber());
        IUSaveText(&iNovaInformationT[3], (iNovaSDK_HasST4() ? "Yes" : "No"));
        IUSaveText(&iNovaInformationT[4], (iNovaSDK_HasColorSensor() ? "Yes" : "No"));
        defineText(&iNovaInformationTP);
        defineNumber(&CameraPropertiesNP);

        // Let's get parameters now from CCD
        setupParams();

        // Start the timer
        SetTimer(POLLMS);
    }
    else
        // We're disconnected
    {
        deleteProperty(iNovaInformationTP.name);
        deleteProperty(CameraPropertiesNP.name);
    }

    return true;
}

/**************************************************************************************
** Setting up CCD parameters
***************************************************************************************/
void INovaCCD::setupParams()
{
    int bpp = iNovaSDK_GetDataWide() > 0 ? 16 : 8;
    SetCCDParams(iNovaSDK_GetImageWidth(), iNovaSDK_GetImageHeight(), bpp, iNovaSDK_GetPixelSizeX(), iNovaSDK_GetPixelSizeY());

    // Let's calculate how much memory we need for the primary CCD buffer
    int nbuf;
    nbuf=PrimaryCCD.getXRes()*PrimaryCCD.getYRes() * PrimaryCCD.getBPP()/8;
    nbuf+=512;	//  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);
}

/**************************************************************************************
** Client is asking us to start an exposure
***************************************************************************************/
bool INovaCCD::StartExposure(float duration)
{
    double expTime = 1000.0 * duration;
    iNovaSDK_SetExpTime(expTime);

    ExposureRequest = duration;
    PrimaryCCD.setExposureDuration(ExposureRequest);
    gettimeofday(&ExpStart,NULL);

    InExposure=true;

    // We're done
    return true;
}

/**************************************************************************************
** Client is asking us to abort an exposure
***************************************************************************************/
bool INovaCCD::AbortExposure()
{
    iNovaSDK_CancelLongExpTime();
    InExposure = false;
    return true;
}

/**************************************************************************************
** How much longer until exposure is done?
***************************************************************************************/
float INovaCCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now,NULL);

    timesince=(double)(now.tv_sec * 1000.0 + now.tv_usec/1000) - (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec/1000);
    timesince=timesince/1000;

    timeleft=ExposureRequest-timesince;
    return timeleft;
}

/**************************************************************************************
** Client is asking us to set a new number
***************************************************************************************/
bool INovaCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (strcmp (dev, getDeviceName()))
        return false;

    if (!strcmp(name, CameraPropertiesNP.name))
    {
        IUUpdateNumber(&CameraPropertiesNP, values, names, n);

        iNovaSDK_SetAnalogGain(static_cast<int16_t>(CameraPropertiesN[CCD_GAIN_N].value));
        iNovaSDK_SetBlackLevel(static_cast<int16_t>(CameraPropertiesN[CCD_BLACKLEVEL_N].value));

        CameraPropertiesNP.s = IPS_OK;
        IDSetNumber(&CameraPropertiesNP, NULL);
        return true;
    }

    return INDI::CCD::ISNewNumber(dev,name,values,names,n);

    /*
    {
        binX = PrimaryCCD.getBinX();
        binY = PrimaryCCD.getBinY();
        startX = PrimaryCCD.getSubX();
        startY = PrimaryCCD.getSubY();
        endX = startX + PrimaryCCD.getSubW();
        endY = startY + PrimaryCCD.getSubH();
        endX = (endX > maxW ? maxW : endX);
        endY = (endY > maxH ? maxH : endY);

        PrimaryCCD.setFrame (startX, startY, endX-startX, endY-startY);

        return true;
    }

    return false;
    */
}

/**************************************************************************************
** INDI is asking us to add any FITS keywords to the FITS header
***************************************************************************************/
void INovaCCD::addFITSKeywords(fitsfile *fptr, INDI::CCDChip *targetChip)
{
    // Let's first add parent keywords
    INDI::CCD::addFITSKeywords(fptr, targetChip);

    // Add temperature to FITS header
    int status=0;
    double gain =  CameraPropertiesN[CCD_GAIN_N].value;
    double blkLvl= CameraPropertiesN[CCD_BLACKLEVEL_N].value;
    fits_update_key_s(fptr, TDOUBLE, "GAIN", &gain, "CCD Gain", &status);
    fits_update_key_s(fptr, TDOUBLE, "BLACKLEVEL", &blkLvl, "CCD Black Level", &status);
    fits_write_date(fptr, &status);

}

/**************************************************************************************
** Main device loop. We check for exposure and temperature progress here
***************************************************************************************/
void INovaCCD::TimerHit()
{
    long timeleft;

    if(isConnected() == false)
        return;  //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        timeleft=CalcTimeLeft();

        // Less than a 0.1 second away from exposure completion
        // This is an over simplified timing method, check CCDSimulator and inova for better timing checks
        if(timeleft < 0.1)
        {
            /* We're done exposing */
            DEBUG(INDI::Logger::DBG_SESSION, "Exposure done, downloading image...");
        }
        else
            // Just update time left in client
            PrimaryCCD.setExposureLeft(timeleft);

    }

    SetTimer(POLLMS);
    return;
}

IPState INovaCCD::GuideEast(float ms)
{
    DIR |= 0x09;
    DIR &= 0x0E;
    iNovaSDK_SendST4(DIR);
    timerWE = IEAddTimer(ms, timerWestEast, (void *)this);
    return IPS_IDLE;
}

IPState INovaCCD::GuideWest(float ms)
{
    DIR |= 0x09;
    DIR &= 0x07;
    iNovaSDK_SendST4(DIR);
    timerWE = IEAddTimer(ms, timerWestEast, (void *)this);
    return IPS_IDLE;
}

IPState INovaCCD::GuideNorth(float ms)
{
    DIR |= 0x06;
    DIR &= 0x0D;
    iNovaSDK_SendST4(DIR);
    timerNS = IEAddTimer(ms, timerNorthSouth, (void *)this);
    return IPS_IDLE;
}

IPState INovaCCD::GuideSouth(float ms)
{
    DIR |= 0x06;
    DIR &= 0x0B;
    iNovaSDK_SendST4(DIR);
    timerNS = IEAddTimer(ms, timerNorthSouth, (void *)this);
    return IPS_IDLE;
}

void INovaCCD::CaptureThread()
{
    while(threadsRunning)
    {
        RawData = (unsigned char*)iNovaSDK_GrabFrame();
        if(RawData != NULL && InExposure)
        {
            // We're no longer exposing...
            InExposure = false;

            grabImage();
        }
    }
}

void INovaCCD::grabImage()
{
    // Let's get a pointer to the frame buffer
    unsigned char * image = PrimaryCCD.getFrameBuffer();
    if(image != NULL)
    {
        int Bpp = iNovaSDK_GetDataWide() > 0 ? 2 : 1;
        int p = 0;

        int binX = PrimaryCCD.getBinX();
        int binY = PrimaryCCD.getBinY();
        int startX = PrimaryCCD.getSubX();
        int startY = PrimaryCCD.getSubY();
        int endX = startX + PrimaryCCD.getSubW();
        int endY = startY + PrimaryCCD.getSubH();
        int maxW = PrimaryCCD.getXRes();
        int maxH = PrimaryCCD.getYRes();
        endX = (endX > maxW ? maxW : endX);
        endY = (endY > maxH ? maxH : endY);

        for(int y=startY; y<endY; y+=binY)
        {
            if(endY-y<binY)
                break;
            for(int x=startX*Bpp; x<endX*Bpp; x+=Bpp*binX)
            {
                if(endX*Bpp-x<binX*Bpp)
                    break;
                int t = 0;
                for(int yy = y; yy < y+binY; yy++)
                {
                    for(int xx = x; xx < x+Bpp*binX; xx+=Bpp)
                    {
                        if(Bpp>1)
                        {
                            t += RawData[1+xx+yy*maxW*Bpp] + (RawData[xx+yy*maxW*Bpp] << 8);
                            t = (t < 0xffff ? t : 0xffff);
                        }
                        else
                        {
                            t += RawData[xx+yy*maxW*Bpp];
                            t = (t < 0xff ? t : 0xff);
                        }
                    }
                }
                image[p++] = (unsigned char)(t & 0xff);
                if(Bpp>1)
                {
                    image[p++] = (unsigned char)((t >> 8) & 0xff);
                }
            }
        }
        // Let INDI::CCD know we're done filling the image buffer
        DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");
        ExposureComplete(&PrimaryCCD);
    }
}
