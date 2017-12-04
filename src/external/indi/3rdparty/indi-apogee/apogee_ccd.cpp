#if 0
    Apogee CCD
    INDI Driver for Apogee CCDs and Filter Wheels
    Copyright (C) 2014 Jasem Mutlaq <mutlaqja AT ikarustech DOT com>

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
#include <stdexcept>
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

#include <libapogee/Alta.h>
#include <libapogee/AltaF.h>
#include <libapogee/Ascent.h>
#include <libapogee/Aspen.h>
#include <libapogee/Quad.h>
#include <libapogee/ApgLogger.h>

#include <fitsio.h>

#include "indidevapi.h"
#include "eventloop.h"
#include "indicom.h"
#include "apogee_ccd.h"
#include "config.h"

void ISInit(void);
void ISPoll(void *);

double min(void);
double max(void);

#define MAX_CCD_TEMP            45   /* Max CCD temperature */
#define MIN_CCD_TEMP            -55  /* Min CCD temperature */
#define MAX_X_BIN               16   /* Max Horizontal binning */
#define MAX_Y_BIN               16   /* Max Vertical binning */
#define MAX_PIXELS              4096 /* Max number of pixels in one dimension */
#define POLLMS                  1000 /* Polling time (ms) */
#define TEMP_THRESHOLD          .25  /* Differential temperature threshold (C)*/
#define NFLUSHES                1    /* Number of times a CCD array is flushed before an exposure */
#define TEMP_UPDATE_THRESHOLD   0.05
#define COOLER_UPDATE_THRESHOLD 0.05

std::unique_ptr<ApogeeCCD> apogeeCCD(new ApogeeCCD());

void ISGetProperties(const char *dev)
{
    apogeeCCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    apogeeCCD->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    apogeeCCD->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    apogeeCCD->ISNewNumber(dev, name, values, names, num);
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
    apogeeCCD->ISSnoopDevice(root);
}

ApogeeCCD::ApogeeCCD()
{
    ApgCam = NULL;

    setVersion(APOGEE_VERSION_MAJOR, APOGEE_VERSION_MINOR);
}

ApogeeCCD::~ApogeeCCD()
{
    delete (ApgCam);
}

const char *ApogeeCCD::getDefaultName()
{
    return (char *)"Apogee CCD";
}

bool ApogeeCCD::initProperties()
{
    // Init parent properties first
    INDI::CCD::initProperties();

    IUFillSwitch(&CoolerS[0], "COOLER_ON", "ON", ISS_OFF);
    IUFillSwitch(&CoolerS[1], "COOLER_OFF", "OFF", ISS_ON);
    IUFillSwitchVector(&CoolerSP, CoolerS, 2, getDeviceName(), "CCD_COOLER", "Cooler", MAIN_CONTROL_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "Cooling Power (%)", "%+06.2f", 0., 1., .2, 0.0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooling Power", MAIN_CONTROL_TAB,
                       IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&ReadOutS[0], "QUALITY_HIGH", "High Quality", ISS_OFF);
    IUFillSwitch(&ReadOutS[1], "QUALITY_LOW", "Fast", ISS_OFF);
    IUFillSwitchVector(&ReadOutSP, ReadOutS, 2, getDeviceName(), "READOUT_QUALITY", "Readout Speed", OPTIONS_TAB, IP_WO,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&PortTypeS[0], "USB_PORT", "USB", ISS_ON);
    IUFillSwitch(&PortTypeS[1], "NETWORK_PORT", "Network", ISS_OFF);
    IUFillSwitchVector(&PortTypeSP, PortTypeS, 2, getDeviceName(), "PORT_TYPE", "Port", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillText(&NetworkInfoT[NETWORK_SUBNET], "SUBNET_ADDRESS", "Subnet", "192.168.0.255");
    IUFillText(&NetworkInfoT[NETWORK_ADDRESS], "IP_PORT_ADDRESS", "IP:Port", "");
    IUFillTextVector(&NetworkInfoTP, NetworkInfoT, 2, getDeviceName(), "NETWORK_INFO", "Network", MAIN_CONTROL_TAB,
                     IP_RW, 0, IPS_IDLE);

    IUFillText(&CamInfoT[0], "CAM_NAME", "Name", "");
    IUFillText(&CamInfoT[1], "CAM_FIRMWARE", "Firmware", "");
    IUFillTextVector(&CamInfoTP, CamInfoT, 2, getDeviceName(), "CAM_INFO", "Info", MAIN_CONTROL_TAB, IP_RO, 0,
                     IPS_IDLE);

    IUFillSwitch(&FanStatusS[0], "FAN_OFF", "Off", ISS_ON);
    IUFillSwitch(&FanStatusS[1], "FAN_SLOW", "Slow", ISS_OFF);
    IUFillSwitch(&FanStatusS[2], "FAN_MED", "Medium", ISS_OFF);
    IUFillSwitch(&FanStatusS[3], "FAN_FAST", "Fast", ISS_OFF);
    IUFillSwitchVector(&FanStatusSP, FanStatusS, 4, getDeviceName(), "Fan Status", "", OPTIONS_TAB, IP_RW, ISR_1OFMANY,
                       0, IPS_IDLE);

    addDebugControl();
    addSimulationControl();

    return true;
}

void ApogeeCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);

    defineSwitch(&PortTypeSP);
    defineText(&NetworkInfoTP);

    loadConfig(true, "PORT_TYPE");
    loadConfig(true, "NETWORK_INFO");
}

bool ApogeeCCD::updateProperties()
{
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        defineText(&CamInfoTP);
        defineSwitch(&CoolerSP);
        defineNumber(&CoolerNP);
        defineSwitch(&ReadOutSP);

        defineSwitch(&FanStatusSP);

        getCameraParams();

        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(CoolerSP.name);
        deleteProperty(CoolerNP.name);
        deleteProperty(ReadOutSP.name);
        deleteProperty(CamInfoTP.name);
        deleteProperty(FanStatusSP.name);

        rmTimer(timerID);
    }

    return true;
}

bool ApogeeCCD::getCameraParams()
{
    double temperature;
    double pixel_size_x, pixel_size_y;
    long sub_frame_x, sub_frame_y;

    if (isSimulation())
    {
        TemperatureN[0].value = 10;
        IDSetNumber(&TemperatureNP, NULL);

        IUResetSwitch(&FanStatusSP);
        FanStatusS[2].s = ISS_ON;
        IDSetSwitch(&FanStatusSP, NULL);

        SetCCDParams(3326, 2504, 16, 5.4, 5.4);

        IUSaveText(&CamInfoT[0], modelStr.c_str());
        IUSaveText(&CamInfoT[1], firmwareRev.c_str());
        IDSetText(&CamInfoTP, NULL);

        IUResetSwitch(&CoolerSP);
        CoolerS[1].s = ISS_ON;
        IDSetSwitch(&CoolerSP, NULL);

        imageWidth  = PrimaryCCD.getSubW();
        imageHeight = PrimaryCCD.getSubH();

        int nbuf;
        nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel count
        nbuf += 512; //  leave a little extra at the end
        PrimaryCCD.setFrameBufferSize(nbuf);

        return true;
    }

    try
    {
        PrimaryCCD.setMinMaxStep("CCD_BINNING", "HOR_BIN", 1, ApgCam->GetMaxBinRows(), 1);
        PrimaryCCD.setMinMaxStep("CCD_BINNING", "VER_BIN", 1, ApgCam->GetMaxBinCols(), 1);
        pixel_size_x = ApgCam->GetPixelWidth();
        pixel_size_y = ApgCam->GetPixelHeight();

        IUSaveText(&CamInfoT[0], ApgCam->GetModel().c_str());
        IUSaveText(&CamInfoT[1], firmwareRev.c_str());
        IDSetText(&CamInfoTP, NULL);

        sub_frame_x = ApgCam->GetMaxImgCols();
        sub_frame_y = ApgCam->GetMaxImgRows();

        temperature = ApgCam->GetTempCcd();

        IUResetSwitch(&CoolerSP);
        Apg::CoolerStatus cStatus = ApgCam->GetCoolerStatus();
        if (cStatus == Apg::CoolerStatus_AtSetPoint || cStatus == Apg::CoolerStatus_RampingToSetPoint)
            CoolerS[0].s = ISS_ON;
        else
            CoolerS[1].s = ISS_ON;

        IDSetSwitch(&CoolerSP, NULL);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "getCameraParams failed. %s.", err.what());
        return false;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "The CCD Temperature is %f.", temperature);
    TemperatureN[0].value = temperature; /* CCD chip temperatre (degrees C) */
    IDSetNumber(&TemperatureNP, NULL);

    Apg::FanMode fStatus = Apg::FanMode_Unknown;

    try
    {
        fStatus = ApgCam->GetFanMode();
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "GetFanMode failed. %s.", err.what());
        return false;
    }

    if (fStatus != Apg::FanMode_Unknown)
    {
        IUResetSwitch(&FanStatusSP);
        FanStatusS[(int)fStatus].s = ISS_ON;
        IDSetSwitch(&FanStatusSP, NULL);
    }

    SetCCDParams(sub_frame_x, sub_frame_y, 16, pixel_size_x, pixel_size_y);

    imageWidth  = PrimaryCCD.getSubW();
    imageHeight = PrimaryCCD.getSubH();

    /*int filter_count;
    try
    {
            QSICam.get_FilterCount(filter_count);
    } catch (std::runtime_error err)
    {
            DEBUGF(INDI::Logger::DBG_SESSION, "get_FilterCount() failed. %s.", err.what());
            return false;
    }

    DEBUGF(INDI::Logger::DBG_SESSION,"The filter count is %d", filter_count);
    */

    try
    {
        minDuration = ApgCam->GetMinExposureTime();
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "get_MinExposureTime() failed. %s.", err.what());
        return false;
    }

    int nbuf;
    nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8; //  this is pixel count
    nbuf += 512;                                                                  //  leave a little extra at the end
    PrimaryCCD.setFrameBufferSize(nbuf);

    return true;
}

int ApogeeCCD::SetTemperature(double temperature)
{
    // If less than 0.1 of a degree, let's just return OK
    if (fabs(temperature - TemperatureN[0].value) < 0.1)
        return 1;

    activateCooler(true);

    try
    {
        if (isSimulation() == false)
            ApgCam->SetCoolerSetPoint(temperature);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "SetCoolerSetPoint failed. %s.", err.what());
        return -1;
    }

    DEBUGF(INDI::Logger::DBG_SESSION, "Setting CCD temperature to %+06.2f C", temperature);
    return 0;
}

bool ApogeeCCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        /* Port Type */
        if (!strcmp(name, PortTypeSP.name))
        {
            IUUpdateSwitch(&PortTypeSP, states, names, n);

            PortTypeSP.s = IPS_OK;

            IDSetSwitch(&PortTypeSP, NULL);

            return true;
        }

        /* Readout Speed */
        if (!strcmp(name, ReadOutSP.name))
        {
            if (IUUpdateSwitch(&ReadOutSP, states, names, n) < 0)
                return false;

            if (ReadOutS[0].s == ISS_ON)
            {
                try
                {
                    if (isSimulation() == false)
                        ApgCam->SetCcdAdcSpeed(Apg::AdcSpeed_Normal);
                }
                catch (std::runtime_error err)
                {
                    IUResetSwitch(&ReadOutSP);
                    ReadOutSP.s = IPS_ALERT;
                    DEBUGF(INDI::Logger::DBG_ERROR, "SetCcdAdcSpeed failed. %s.", err.what());
                    IDSetSwitch(&ReadOutSP, NULL);
                    return false;
                }
            }
            else
            {
                try
                {
                    if (isSimulation() == false)
                        ApgCam->SetCcdAdcSpeed(Apg::AdcSpeed_Fast);
                }
                catch (std::runtime_error err)
                {
                    IUResetSwitch(&ReadOutSP);
                    ReadOutSP.s = IPS_ALERT;
                    DEBUGF(INDI::Logger::DBG_ERROR, "SetCcdAdcSpeed failed. %s.", err.what());
                    IDSetSwitch(&ReadOutSP, NULL);
                    return false;
                }

                ReadOutSP.s = IPS_OK;
                IDSetSwitch(&ReadOutSP, NULL);
            }

            ReadOutSP.s = IPS_OK;
            IDSetSwitch(&ReadOutSP, NULL);
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
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool ApogeeCCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(NetworkInfoTP.name, name))
        {
            IUUpdateText(&NetworkInfoTP, texts, names, n);

            subnet = std::string(NetworkInfoT[NETWORK_SUBNET].text);

            if (strlen(NetworkInfoT[NETWORK_ADDRESS].text) > 0)
            {
                char ip[16];
                int port;
                int rc = sscanf(NetworkInfoT[NETWORK_ADDRESS].text, "%[^:]:%d", ip, &port);

                if (rc == 2)
                    NetworkInfoTP.s = IPS_OK;
                else
                {
                    DEBUG(INDI::Logger::DBG_ERROR, "Invalid format. Format must be IP:Port (e.g. 192.168.1.1:80)");
                    NetworkInfoTP.s = IPS_ALERT;
                }
            }
            else
                NetworkInfoTP.s = IPS_OK;

            IDSetText(&NetworkInfoTP, NULL);

            return true;
        }
    }

    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool ApogeeCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    /*if(strcmp(dev,getDeviceName())==0)
    {

    }*/

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool ApogeeCCD::StartExposure(float duration)
{
    if (duration < minDuration)
    {
        ExposureRequest = minDuration;
        DEBUGF(INDI::Logger::DBG_WARNING,
               "Exposure shorter than minimum ExposureRequest %g s requested. \n Setting exposure time to %g s.",
               minDuration, minDuration);
    }
    else
        ExposureRequest = duration;

    imageFrameType = PrimaryCCD.getFrameType();

    if (imageFrameType == INDI::CCDChip::BIAS_FRAME)
    {
        ExposureRequest = minDuration;
        DEBUGF(INDI::Logger::DBG_SESSION, "Bias Frame (s) : %g", ExposureRequest);
    }

    if (isSimulation() == false)
        ApgCam->SetImageCount(1);

    /* BIAS frame is the same as DARK but with minimum period. i.e. readout from camera electronics.*/
    if (imageFrameType == INDI::CCDChip::BIAS_FRAME || imageFrameType == INDI::CCDChip::DARK_FRAME)
    {
        try
        {
            if (isSimulation() == false)
            {
                ApgCam->StartExposure(ExposureRequest, false);
                PrimaryCCD.setExposureDuration(ExposureRequest);
            }
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
            if (isSimulation() == false)
            {
                ApgCam->StartExposure(ExposureRequest, true);
                PrimaryCCD.setExposureDuration(ExposureRequest);
            }
        }
        catch (std::runtime_error &err)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "StartExposure() failed. %s.", err.what());
            return false;
        }
    }

    gettimeofday(&ExpStart, NULL);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Taking a %g seconds frame...", ExposureRequest);

    InExposure = true;
    return true;
}

bool ApogeeCCD::AbortExposure()
{
    try
    {
        if (isSimulation() == false)
            ApgCam->StopExposure(false);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "AbortExposure() failed. %s.", err.what());
        return false;
    }

    InExposure = false;
    return true;
}

float ApogeeCCD::CalcTimeLeft(timeval start, float req)
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, NULL);

    timesince =
        (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) - (double)(start.tv_sec * 1000.0 + start.tv_usec / 1000);
    timesince = timesince / 1000;
    timeleft  = req - timesince;
    return timeleft;
}

bool ApogeeCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    if (InExposure == true)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot change CCD frame while exposure is in progress.");
        return false;
    }

    /* Add the X and Y offsets */
    long x_1 = x;
    long y_1 = y;

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
        if (isSimulation() == false)
        {
            ApgCam->SetRoiStartCol(x_1);
            ApgCam->SetRoiStartRow(y_1);
            ApgCam->SetRoiNumCols(imageWidth);
            ApgCam->SetRoiNumRows(imageHeight);
        }
    }
    catch (std::runtime_error err)
    {
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

bool ApogeeCCD::UpdateCCDBin(int binx, int biny)
{
    if (InExposure == true)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot change CCD binning while exposure is in progress.");
        return false;
    }

    try
    {
        if (isSimulation() == false)
            ApgCam->SetRoiBinCol(binx);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "SetRoiBinCol failed. %s.", err.what());
        return false;
    }

    try
    {
        if (isSimulation() == false)
            ApgCam->SetRoiBinRow(biny);
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "SetRoiBinRow failed. %s.", err.what());
        return false;
    }

    PrimaryCCD.setBin(binx, biny);

    return UpdateCCDFrame(PrimaryCCD.getSubX(), PrimaryCCD.getSubY(), PrimaryCCD.getSubW(), PrimaryCCD.getSubH());
}

/* Downloads the image from the CCD.
 N.B. No processing is done on the image */
int ApogeeCCD::grabImage()
{
    std::vector<uint16_t> pImageData;
    unsigned short *image = (unsigned short *)PrimaryCCD.getFrameBuffer();

    try
    {
        if (isSimulation())
        {
            for (int i = 0; i < imageHeight; i++)
                for (int j = 0; j < imageWidth; j++)
                    image[i * imageWidth + j] = rand() % 65535;
        }
        else
        {
            ApgCam->GetImage(pImageData);
            imageWidth  = ApgCam->GetRoiNumCols();
            imageHeight = ApgCam->GetRoiNumRows();
            copy(pImageData.begin(), pImageData.end(), image);
        }
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "GetImage failed. %s.", err.what());
        return -1;
    }

    ExposureComplete(&PrimaryCCD);

    DEBUG(INDI::Logger::DBG_SESSION, "Download complete.");

    return 0;
}

///////////////////////////
// MAKE	  TOKENS
std::vector<std::string> ApogeeCCD::MakeTokens(const std::string &str, const std::string &separator)
{
    std::vector<std::string> returnVector;
    std::string::size_type start = 0;
    std::string::size_type end   = 0;

    while ((end = str.find(separator, start)) != std::string::npos)
    {
        returnVector.push_back(str.substr(start, end - start));
        start = end + separator.size();
    }

    returnVector.push_back(str.substr(start));

    return returnVector;
}

///////////////////////////
//	GET    ITEM    FROM     FIND       STR
std::string ApogeeCCD::GetItemFromFindStr(const std::string &msg, const std::string &item)
{
    //search the single device input string for the requested item
    std::vector<std::string> params = MakeTokens(msg, ",");
    std::vector<std::string>::iterator iter;

    for (iter = params.begin(); iter != params.end(); ++iter)
    {
        if (std::string::npos != (*iter).find(item))
        {
            std::string result = MakeTokens((*iter), "=").at(1);

            return result;
        }
    } //for

    std::string noOp;
    return noOp;
}

////////////////////////////
//	GET		USB  ADDRESS
std::string ApogeeCCD::GetUsbAddress(const std::string &msg)
{
    return GetItemFromFindStr(msg, "address=");
}

////////////////////////////
//	GET		IP Address
std::string ApogeeCCD::GetIPAddress(const std::string &msg)
{
    std::string addr = GetItemFromFindStr(msg, "address=");
    return addr;
}

////////////////////////////
//	GET		ETHERNET  ADDRESS
std::string ApogeeCCD::GetEthernetAddress(const std::string &msg)
{
    std::string addr = GetItemFromFindStr(msg, "address=");
    addr.append(":");
    addr.append(GetItemFromFindStr(msg, "port="));
    return addr;
}
////////////////////////////
//	GET		ID
uint16_t ApogeeCCD::GetID(const std::string &msg)
{
    std::string str = GetItemFromFindStr(msg, "id=");
    uint16_t id     = 0;
    std::stringstream ss;
    ss << std::hex << std::showbase << str.c_str();
    ss >> id;

    return id;
}

////////////////////////////
//	GET		FRMWR       REV
uint16_t ApogeeCCD::GetFrmwrRev(const std::string &msg)
{
    std::string str = GetItemFromFindStr(msg, "firmwareRev=");

    uint16_t rev = 0;
    std::stringstream ss;
    ss << std::hex << std::showbase << str.c_str();
    ss >> rev;

    return rev;
}

////////////////////////////
//	        IS      DEVICE      FILTER      WHEEL
bool ApogeeCCD::IsDeviceFilterWheel(const std::string &msg)
{
    std::string str = GetItemFromFindStr(msg, "deviceType=");

    return (0 == str.compare("filterWheel") ? true : false);
}

////////////////////////////
//	        IS  	ASCENT
bool ApogeeCCD::IsAscent(const std::string &msg)
{
    std::string model = GetItemFromFindStr(msg, "model=");
    std::string ascent("Ascent");
    return (0 == model.compare(0, ascent.size(), ascent) ? true : false);
}

////////////////////////////
//      PRINT       INFO
void ApogeeCCD::printInfo(const std::string &model, const uint16_t maxImgRows, const uint16_t maxImgCols)
{
    std::cout << "Cam Info: " << std::endl;
    std::cout << "model = " << model.c_str() << std::endl;
    std::cout << "max # imaging rows = " << maxImgRows;
    std::cout << "\tmax # imaging cols = " << maxImgCols << std::endl;
}

////////////////////////////
//		CHECK	STATUS
void ApogeeCCD::checkStatus(const Apg::Status status)
{
    switch (status)
    {
        case Apg::Status_ConnectionError:
        {
            std::string errMsg("Status_ConnectionError");
            std::runtime_error except(errMsg);
            throw except;
        }
        break;

        case Apg::Status_DataError:
        {
            std::string errMsg("Status_DataError");
            std::runtime_error except(errMsg);
            throw except;
        }
        break;

        case Apg::Status_PatternError:
        {
            std::string errMsg("Status_PatternError");
            std::runtime_error except(errMsg);
            throw except;
        }
        break;

        case Apg::Status_Idle:
        {
            std::string errMsg("Status_Idle");
            std::runtime_error except(errMsg);
            throw except;
        }
        break;

        default:
            //no op on purpose
            break;
    }
}

CamModel::PlatformType ApogeeCCD::GetModel(const std::string &msg)
{
    modelStr = GetItemFromFindStr(msg, "model=");

    return CamModel::GetPlatformType(modelStr);
}

bool ApogeeCCD::Connect()
{
    std::string msg;
    std::string addr;

    DEBUG(INDI::Logger::DBG_SESSION, "Attempting to find Apogee CCD...");

    // USB
    if (PortTypeS[0].s == ISS_ON)
    {
        // Simulation
        if (isSimulation())
        {
            msg  = std::string("<d>address=0,interface=usb,deviceType=camera,id=0x49,firmwareRev=0x21,model=AltaU-"
                              "4020ML,interfaceStatus=NA</d><d>address=1,interface=usb,model=Filter "
                              "Wheel,deviceType=filterWheel,id=0xFFFF,firmwareRev=0xFFEE</d>");
            addr = GetUsbAddress(msg);
        }
        else
        {
            ioInterface = std::string("usb");
            FindDeviceUsb lookUsb;
            try
            {
                msg  = lookUsb.Find();
                addr = GetUsbAddress(msg);
            }
            catch (std::runtime_error &err)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Error getting USB address: %s", err.what());
                return false;
            }
        }
    }
    // Ethernet
    else
    {
        ioInterface = std::string("ethernet");
        FindDeviceEthernet look4cam;
        char ip[32];
        int port;

        // Simulation
        if (isSimulation())
        {
            msg = std::string("<d>address=192.168.1.20,interface=ethernet,port=80,mac=0009510000FF,deviceType=camera,"
                              "id=0xfeff,firmwareRev=0x0,model=AltaU-4020ML</d>"
                              "<d>address=192.168.1.21,interface=ethernet,port=80,mac=0009510000FF,deviceType=camera,"
                              "id=0xfeff,firmwareRev=0x0,model=AltaU-4020ML</d>"
                              "<d>address=192.168.2.22,interface=ethernet,port=80,mac=0009510000FF,deviceType=camera,"
                              "id=0xfeff,firmwareRev=0x0,model=AltaU-4020ML</d>");
        }
        else
        {
            try
            {
                msg = look4cam.Find(subnet);
                DEBUGF(INDI::Logger::DBG_DEBUG, "Network search result: %s", msg.c_str());
            }
            catch (std::runtime_error &err)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Error getting network address: %s", err.what());
                return false;
            }
        }

        int rc = 0;

        // Check if we have IP:Port format
        if (NetworkInfoT[NETWORK_ADDRESS].text != NULL && strlen(NetworkInfoT[NETWORK_ADDRESS].text) > 0)
            rc = sscanf(NetworkInfoT[NETWORK_ADDRESS].text, "%[^:]:%d", ip, &port);

        // If we have IP:Port, then let's skip all entries that does not have our desired IP address.
        if (rc == 2)
        {
            addr                  = NetworkInfoT[NETWORK_ADDRESS].text;
            std::string delimiter = "</d>";

            size_t pos = 0;
            std::string token, token_ip;
            while ((pos = msg.find(delimiter)) != std::string::npos)
            {
                token    = msg.substr(0, pos);
                token_ip = GetIPAddress(token);
                DEBUGF(INDI::Logger::DBG_DEBUG, "Checking %s (%s) for IP %s", token.c_str(), token_ip.c_str(), ip);
                if (token_ip == ip)
                {
                    msg = token;
                    DEBUGF(INDI::Logger::DBG_DEBUG, "IP matched (%s).", msg.c_str());
                    break;
                }
                msg.erase(0, pos + delimiter.length());
            }
        }
        else
        {
            addr = GetEthernetAddress(msg);
            IUSaveText(&NetworkInfoT[NETWORK_ADDRESS], addr.c_str());
            DEBUGF(INDI::Logger::DBG_SESSION, "Detected camera at %s", addr.c_str());
            IDSetText(&NetworkInfoTP, NULL);
        }
    }

    if (msg.compare("<d></d>") == 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR,
              "Unable to find Apogee CCDs or Filter Wheels attached. Please check connection and power and try again.");
        return false;
    }

    uint16_t id       = GetID(msg);
    uint16_t frmwrRev = GetFrmwrRev(msg);

    char firmwareStr[16];
    snprintf(firmwareStr, 16, "0x%X", frmwrRev);
    firmwareRev = std::string(firmwareStr);
    model       = GetModel(msg);

    switch (model)
    {
        case CamModel::ALTAU:
        case CamModel::ALTAE:
            ApgCam = new Alta();
            break;

        case CamModel::ASPEN:
            ApgCam = new Aspen();
            break;

        case CamModel::ALTAF:
            ApgCam = new AltaF();
            break;

        case CamModel::ASCENT:
            ApgCam = new Ascent();
            break;

        case CamModel::QUAD:
            ApgCam = new Quad();
            break;

        default:
            DEBUGF(INDI::Logger::DBG_ERROR, "Model %s is not supported by the INDI Apogee driver.",
                   GetItemFromFindStr(msg, "model=").c_str());
            return false;
            break;
    }

    try
    {
        if (isSimulation() == false)
        {
            ApgCam->OpenConnection(ioInterface, addr, frmwrRev, id);
            ApgCam->Init();
        }
    }
    catch (std::runtime_error &err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error opening camera: %s", err.what());
        return false;
    }

    uint32_t cap = CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_COOLER | CCD_HAS_SHUTTER;
    SetCCDCapability(cap);

    /* Success! */
    DEBUG(INDI::Logger::DBG_SESSION, "CCD is online. Retrieving basic data.");
    return true;
}

bool ApogeeCCD::Disconnect()
{
    try
    {
        if (isSimulation() == false)
            ApgCam->CloseConnection();
    }
    catch (std::runtime_error err)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: CloseConnection failed. %s.", err.what());
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "CCD is offline.");
    return true;
}

void ApogeeCCD::activateCooler(bool enable)
{
    bool coolerOn;
    bool coolerSet = false;

    if (isSimulation())
        return;

    try
    {
        coolerOn = ApgCam->IsCoolerOn();
        if ((enable && coolerOn == false) || (!enable && coolerOn == true))
        {
            ApgCam->SetCooler(enable);
            coolerSet = true;
        }
    }
    catch (std::runtime_error err)
    {
        CoolerSP.s   = IPS_ALERT;
        CoolerS[0].s = ISS_OFF;
        CoolerS[1].s = ISS_ON;
        DEBUGF(INDI::Logger::DBG_ERROR, "Error: SetCooler failed. %s.", err.what());
        IDSetSwitch(&CoolerSP, NULL);
        return;
    }

    /* Success! */
    CoolerS[0].s = enable ? ISS_ON : ISS_OFF;
    CoolerS[1].s = enable ? ISS_OFF : ISS_ON;
    CoolerSP.s   = IPS_OK;
    if (coolerSet)
        DEBUG(INDI::Logger::DBG_SESSION, enable ? "Cooler ON" : "Cooler Off");
    IDSetSwitch(&CoolerSP, NULL);
}

void ApogeeCCD::TimerHit()
{
    long timeleft;
    double ccdTemp;
    double coolerPower;

    if (isConnected() == false)
        return; //  No need to reset timer if we are not connected anymore

    if (InExposure)
    {
        timeleft = CalcTimeLeft(ExpStart, ExposureRequest);

        if (timeleft < 1)
        {
            if (isSimulation() == false)
            {
                Apg::Status status = ApgCam->GetImagingStatus();

                while (status != Apg::Status_ImageReady)
                {
                    usleep(250000);
                    status = ApgCam->GetImagingStatus();
                }
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
                if (isSimulation())
                    ccdTemp = TemperatureN[0].value;
                else
                    ccdTemp = ApgCam->GetTempCcd();
            }
            catch (std::runtime_error err)
            {
                TemperatureNP.s = IPS_IDLE;
                DEBUGF(INDI::Logger::DBG_ERROR, "GetTempCcd failed. %s.", err.what());
                IDSetNumber(&TemperatureNP, NULL);
                return;
            }

            if (fabs(TemperatureN[0].value - ccdTemp) >= TEMP_UPDATE_THRESHOLD)
            {
                TemperatureN[0].value = ccdTemp;
                IDSetNumber(&TemperatureNP, NULL);
            }
            break;

        case IPS_BUSY:

            try
            {
                if (isSimulation())
                    ccdTemp = TemperatureN[0].value;
                else
                    ccdTemp = ApgCam->GetTempCcd();
            }
            catch (std::runtime_error err)
            {
                TemperatureNP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "GetTempCcd failed. %s.", err.what());
                IDSetNumber(&TemperatureNP, NULL);
                return;
            }

            if (fabs(TemperatureN[0].value - ccdTemp) <= TEMP_THRESHOLD)
                TemperatureNP.s = IPS_OK;

            TemperatureN[0].value = ccdTemp;
            IDSetNumber(&TemperatureNP, NULL);
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
                if (isSimulation())
                    coolerPower = 50;
                else
                    coolerPower = ApgCam->GetCoolerDrive();
            }
            catch (std::runtime_error err)
            {
                CoolerNP.s = IPS_IDLE;
                DEBUGF(INDI::Logger::DBG_ERROR, "GetCoolerDrive failed. %s.", err.what());
                IDSetNumber(&CoolerNP, NULL);
                return;
            }

            if (fabs(CoolerN[0].value - coolerPower) >= COOLER_UPDATE_THRESHOLD)
            {
                if (coolerPower > 0)
                    CoolerNP.s = IPS_BUSY;

                CoolerN[0].value = coolerPower;
                IDSetNumber(&CoolerNP, NULL);
            }

            break;

        case IPS_BUSY:

            try
            {
                if (isSimulation())
                    coolerPower = 50;
                else
                    coolerPower = ApgCam->GetCoolerDrive();
            }
            catch (std::runtime_error err)
            {
                CoolerNP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "GetCoolerDrive failed. %s.", err.what());
                IDSetNumber(&CoolerNP, NULL);
                return;
            }

            if (fabs(CoolerN[0].value - coolerPower) >= COOLER_UPDATE_THRESHOLD)
            {
                if (coolerPower <= 0)
                    CoolerNP.s = IPS_IDLE;

                CoolerN[0].value = coolerPower;
                IDSetNumber(&CoolerNP, NULL);
            }

            break;

        case IPS_ALERT:
            break;
    }

    SetTimer(POLLMS);
    return;
}

void ApogeeCCD::debugTriggered(bool enabled)
{
    ApgLogger::Instance().SetLogLevel(enabled ? ApgLogger::LEVEL_DEBUG : ApgLogger::LEVEL_RELEASE);
}

bool ApogeeCCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &PortTypeSP);
    IUSaveConfigText(fp, &NetworkInfoTP);

    return true;
}
