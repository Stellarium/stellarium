/*
    Driver type: SBIG CCD Camera INDI Driver

    Copyright (C) 2017 Peter Polakovic (peter DOT polakovic AT cloudmakers DOT eu)
    Copyright (C) 2013-2016 Jasem Mutlaq (mutlaqja AT ikarustech DOT com)
    Copyright (C) 2005-2006 Jan Soldan (jsoldan AT asu DOT cas DOT cz)

    Acknowledgement:
    Matt Longmire 	(matto AT sbig DOT com)

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

    2016-01-07: Added ETH connection (by Simon Holmbo)
    2016-01-07: Changed Device port from text to switch (JM)
    2017-06-22: Bugfixes and code cleanup (PP)

 */

#include "sbig_ccd.h"

#include <eventloop.h>

#include <math.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define TEMPERATURE_POLL_MS 5000 /* Temperature Polling time (ms) */
#define MAX_RESOLUTION      4096 /* Maximum resolutoin for secondary chip */
#define POLLMS              1000 /* Polling time (ms) */
#define MAX_DEVICES         20   /* Max device cameraCount */
#define MAX_THREAD_RETRIES  3
#define MAX_THREAD_WAIT     300000

static int cameraCount;
static SBIGCCD *cameras[MAX_DEVICES];
#ifdef ASYNC_READOUT
pthread_cond_t cv         = PTHREAD_COND_INITIALIZER;
pthread_mutex_t condMutex = PTHREAD_MUTEX_INITIALIZER;
#endif
pthread_mutex_t sbigMutex = PTHREAD_MUTEX_INITIALIZER;

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
        cameraCount = 1;
        cameras[0]  = new SBIGCCD();
        atexit(cleanup);
        isInit = true;
    }
}

void ISGetProperties(const char *dev)
{
    ISInit();
    for (int i = 0; i < cameraCount; i++)
    {
        SBIGCCD *camera = cameras[i];
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
        SBIGCCD *camera = cameras[i];
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
        SBIGCCD *camera = cameras[i];
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
        SBIGCCD *camera = cameras[i];
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
        SBIGCCD *camera = cameras[i];
        camera->ISSnoopDevice(root);
    }
}

//==========================================================================

int SBIGCCD::OpenDriver()
{
    GetDriverHandleResults gdhr;
    SetDriverHandleParams sdhp;
    int res = ::SBIGUnivDrvCommand(CC_OPEN_DRIVER, 0, 0);
    if (res == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s: CC_OPEN_DRIVER successfull", __FUNCTION__);
        res = ::SBIGUnivDrvCommand(CC_GET_DRIVER_HANDLE, 0, &gdhr);
    }
    else if (res == CE_DRIVER_NOT_CLOSED)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "%s: CC_OPEN_DRIVER -> (%s)", __FUNCTION__, GetErrorString(res));
        // The driver is already open which we interpret as having been
        // opened by another instance of the class so get the driver to
        // allocate a new handle and then record it.
        sdhp.handle = INVALID_HANDLE_VALUE;
        res         = ::SBIGUnivDrvCommand(CC_SET_DRIVER_HANDLE, &sdhp, 0);
        if (res == CE_NO_ERROR)
        {
            res = ::SBIGUnivDrvCommand(CC_OPEN_DRIVER, 0, 0);
            if (res == CE_NO_ERROR)
            {
                res = ::SBIGUnivDrvCommand(CC_GET_DRIVER_HANDLE, 0, &gdhr);
            }
        }
    }
    if (res == CE_NO_ERROR)
    {
        SetDriverHandle(gdhr.handle);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_OPEN_DRIVER -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

//==========================================================================

int SBIGCCD::CloseDriver()
{
    int res = ::SBIGUnivDrvCommand(CC_CLOSE_DRIVER, 0, 0);
    if (res == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s: CC_CLOSE_DRIVER successfull", __FUNCTION__);
        SetDriverHandle();
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CLOSE_DRIVER -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

//==========================================================================

int SBIGCCD::OpenDevice(uint32_t devType)
{
    int res = CE_NO_ERROR;
    if (isSimulation())
    {
        return res;
    }
    OpenDeviceParams odp;
    if (IsDeviceOpen()) // Check if device already opened
        return (CE_NO_ERROR);
    odp.deviceType = devType; // Try to open new device
    if (devType == DEV_ETH)
    {
        unsigned long ip = htonl(inet_addr(IpTP.tp->text));
        if (ip == INADDR_NONE)
            return (CE_BAD_PARAMETER);
        odp.ipAddress = ip;
    }
    res = SBIGUnivDrvCommand(CC_OPEN_DEVICE, &odp, 0);
    if (res == CE_NO_ERROR)
    {
        //SetDeviceName(devType);
        SetFileDescriptor(true);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_OPEN_DEVICE %d -> (%s)", __FUNCTION__, devType, GetErrorString(res));
    }
    return res;
}

//==========================================================================

int SBIGCCD::CloseDevice()
{
    int res = CE_NO_ERROR;
    if (isSimulation())
    {
        return res;
    }
    if (IsDeviceOpen())
    {
        if ((res = SBIGUnivDrvCommand(CC_CLOSE_DEVICE, 0, 0)) == CE_NO_ERROR)
        {
            SetFileDescriptor(); // set value to -1
            SetCameraType();     // set value to NO_CAMERA
        }
    }
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CLOSE_DEVICE -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

//==========================================================================

SBIGCCD::SBIGCCD() : FilterInterface(this)
{
    InitVars();
    int res = OpenDriver();
    if (res != CE_NO_ERROR)
        DEBUGF(INDI::Logger::DBG_DEBUG, "%s: Error (%s)", __FUNCTION__, GetErrorString(res));
    // TBD: For now let's set name to default name. In the future, we need to to support multiple devices per one driver
    if (*getDeviceName() == '\0')
        strncpy(name, getDefaultName(), MAXINDINAME);
    else
        strncpy(name, getDeviceName(), MAXINDINAME);
    isColor                = false;
    useExternalTrackingCCD = false;
#ifdef ASYNC_READOUT
    grabPredicate   = GRAB_NO_CCD;
    terminateThread = false;
#endif
    hasGuideHead   = false;
    hasFilterWheel = false;
    setVersion(1, 8);
}

//==========================================================================

/*SBIGCCD::SBIGCCD(const char* devName) {
 int res = CE_NO_ERROR;
 InitVars();
 if ((res = OpenDriver()) == CE_NO_ERROR)
    OpenDevice(devName);
 if (res != CE_NO_ERROR)
   DEBUGF(INDI::Logger::DBG_DEBUG, "%s: Error (%s)", __FUNCTION__, GetErrorString(res));
 }*/

//==========================================================================

SBIGCCD::~SBIGCCD()
{
    CloseDevice();
    CloseDriver();
}

//==========================================================================

const char *SBIGCCD::getDefaultName()
{
    return (const char *)"SBIG CCD";
}

bool SBIGCCD::initProperties()
{
    INDI::CCD::initProperties();

    addSimulationControl();

    // CCD PRODUCT
    IUFillText(&ProductInfoT[0], "NAME", "Name", "");
    IUFillText(&ProductInfoT[1], "ID", "ID", "");
    IUFillTextVector(&ProductInfoTP, ProductInfoT, 2, getDeviceName(), "CCD_PRODUCT", "Product", MAIN_CONTROL_TAB,
                     IP_RO, 0, IPS_IDLE);

    // IP Address
    IUFillText(&IpT[0], "IP", "IP Address", "192.168.0.100");
    IUFillTextVector(&IpTP, IpT, 1, getDeviceName(), "IP_ADDRESS", "IP Address", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    // CCD DEVICE PORT
    IUFillSwitch(&PortS[0], "Ethernet", "Ethernet", ISS_OFF);
    SBIGPortMap[0] = DEV_ETH;
    PortS[0].aux   = &SBIGPortMap[0];
    IUFillSwitch(&PortS[1], "USB 1", "USB 1", ISS_ON);
    SBIGPortMap[1] = DEV_USB1;
    PortS[1].aux   = &SBIGPortMap[1];
    IUFillSwitch(&PortS[2], "USB 2", "USB 2", ISS_OFF);
    SBIGPortMap[2] = DEV_USB2;
    PortS[2].aux   = &SBIGPortMap[2];
    IUFillSwitch(&PortS[3], "USB 3", "USB 3", ISS_OFF);
    SBIGPortMap[3] = DEV_USB3;
    PortS[3].aux   = &SBIGPortMap[3];
    IUFillSwitch(&PortS[4], "USB 4", "USB 4", ISS_OFF);
    SBIGPortMap[4] = DEV_USB4;
    PortS[4].aux   = &SBIGPortMap[4];
    IUFillSwitch(&PortS[5], "LPT 1", "LPT 1", ISS_OFF);
    SBIGPortMap[5] = DEV_LPT1;
    PortS[5].aux   = &SBIGPortMap[5];
    IUFillSwitch(&PortS[6], "LPT 2", "LPT 2", ISS_OFF);
    SBIGPortMap[6] = DEV_LPT2;
    PortS[6].aux   = &SBIGPortMap[6];
    IUFillSwitch(&PortS[7], "LPT 3", "LPT 3", ISS_OFF);
    SBIGPortMap[7] = DEV_LPT3;
    PortS[7].aux   = &SBIGPortMap[7];
    IUFillSwitchVector(&PortSP, PortS, 8, getDeviceName(), "DEVICE_PORT_TYPE", "Port", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // CCD FAN STATE
    IUFillSwitch(&FanStateS[0], "FAN_ON", "On", ISS_ON);
    IUFillSwitch(&FanStateS[1], "FAN_OFF", "Off", ISS_OFF);
    IUFillSwitchVector(&FanStateSP, FanStateS, 2, getDeviceName(), "CCD_FAN", "Fan", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_OK);

    // CCD Cooler Switch
    IUFillSwitch(&CoolerS[0], "COOLER_ON", "On", ISS_OFF);
    IUFillSwitch(&CoolerS[1], "COOLER_OFF", "Off", ISS_ON);
    IUFillSwitchVector(&CoolerSP, CoolerS, 2, getDeviceName(), "CCD_COOLER", "Cooler", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_OK);

    // CCD COOLER:
    IUFillNumber(&CoolerN[0], "CCD_COOLER_VALUE", "[%]", "%.1f", 0, 0, 0, 0);
    IUFillNumberVector(&CoolerNP, CoolerN, 1, getDeviceName(), "CCD_COOLER_POWER", "Cooler %", MAIN_CONTROL_TAB, IP_RO,
                       0, IPS_IDLE);

    // CFW PRODUCT
    IUFillText(&FilterProdcutT[0], "NAME", "Name", "");
    IUFillText(&FilterProdcutT[1], "ID", "ID", "");
    IUFillTextVector(&FilterProdcutTP, FilterProdcutT, 2, getDeviceName(), "CFW_PRODUCT", "Product", FILTER_TAB, IP_RO,
                     0, IPS_IDLE);

    // CFW_MODEL
    IUFillSwitch(&FilterTypeS[0], "CFW1", "CFW-2", ISS_OFF);
    FilterTypeS[0].aux = &SBIGFilterMap[0];
    SBIGFilterMap[0]   = CFWSEL_CFW2;
    IUFillSwitch(&FilterTypeS[1], "CFW2", "CFW-5", ISS_OFF);
    FilterTypeS[1].aux = &SBIGFilterMap[1];
    SBIGFilterMap[1]   = CFWSEL_CFW5;
    IUFillSwitch(&FilterTypeS[2], "CFW3", "CFW-6A", ISS_OFF);
    FilterTypeS[2].aux = &SBIGFilterMap[2];
    SBIGFilterMap[2]   = CFWSEL_CFW6A;
    IUFillSwitch(&FilterTypeS[3], "CFW4", "CFW-8", ISS_OFF);
    FilterTypeS[3].aux = &SBIGFilterMap[3];
    SBIGFilterMap[3]   = CFWSEL_CFW8;
    IUFillSwitch(&FilterTypeS[4], "CFW5", "CFW-402", ISS_OFF);
    FilterTypeS[4].aux = &SBIGFilterMap[4];
    SBIGFilterMap[4]   = CFWSEL_CFW402;
    IUFillSwitch(&FilterTypeS[5], "CFW6", "CFW-10", ISS_OFF);
    FilterTypeS[5].aux = &SBIGFilterMap[5];
    SBIGFilterMap[5]   = CFWSEL_CFW10;
    IUFillSwitch(&FilterTypeS[6], "CFW7", "CFW-10 SA", ISS_OFF);
    FilterTypeS[6].aux = &SBIGFilterMap[6];
    SBIGFilterMap[6]   = CFWSEL_CFW10_SERIAL;
    IUFillSwitch(&FilterTypeS[7], "CFW8", "CFW-L", ISS_OFF);
    FilterTypeS[7].aux = &SBIGFilterMap[7];
    SBIGFilterMap[7]   = CFWSEL_CFWL;
    IUFillSwitch(&FilterTypeS[8], "CFW9", "CFW-9", ISS_OFF);
    FilterTypeS[8].aux = &SBIGFilterMap[8];
    SBIGFilterMap[8]   = CFWSEL_CFW9;
    IUFillSwitch(&FilterTypeS[9], "CFW10", "CFW-8LG", ISS_OFF);
    FilterTypeS[9].aux = &SBIGFilterMap[9];
    SBIGFilterMap[9]   = CFWSEL_CFWL8G;
    IUFillSwitch(&FilterTypeS[10], "CFW11", "CFW-1603", ISS_OFF);
    FilterTypeS[10].aux = &SBIGFilterMap[10];
    SBIGFilterMap[10]   = CFWSEL_CFW1603;
    IUFillSwitch(&FilterTypeS[11], "CFW12", "CFW-FW5-STX", ISS_OFF);
    FilterTypeS[11].aux = &SBIGFilterMap[11];
    SBIGFilterMap[11]   = CFWSEL_FW5_STX;
    IUFillSwitch(&FilterTypeS[12], "CFW13", "CFW-FW5-8300", ISS_OFF);
    FilterTypeS[12].aux = &SBIGFilterMap[12];
    SBIGFilterMap[12]   = CFWSEL_FW5_8300;
    IUFillSwitch(&FilterTypeS[13], "CFW14", "CFW-FW8-8300", ISS_OFF);
    FilterTypeS[13].aux = &SBIGFilterMap[13];
    SBIGFilterMap[13]   = CFWSEL_FW8_8300;
    IUFillSwitch(&FilterTypeS[14], "CFW15", "CFW-FW7-STX", ISS_OFF);
    FilterTypeS[14].aux = &SBIGFilterMap[14];
    SBIGFilterMap[14]   = CFWSEL_FW7_STX;
    IUFillSwitch(&FilterTypeS[15], "CFW16", "CFW-FW8-STT", ISS_OFF);
    FilterTypeS[15].aux = &SBIGFilterMap[15];
    SBIGFilterMap[15]   = CFWSEL_FW8_STT;
#ifdef USE_CFW_AUTO
    IUFillSwitch(&FilterTypeS[16], "CFW17", "CFW-Auto", ISS_OFF);
    FilterTypeS[16].aux = &SBIGFilterMap[16];
    SBIGFilterMap[16]   = CFWSEL_AUTO;
#endif
    IUFillSwitchVector(&FilterTypeSP, FilterTypeS, MAX_CFW_TYPES, getDeviceName(), "CFW_TYPE", "Type", FILTER_TAB,
                       IP_RW, ISR_ATMOST1, 0, IPS_IDLE);

    // CFW CONNECTION
    IUFillSwitch(&FilterConnectionS[0], "CONNECT", "Connect", ISS_OFF);
    IUFillSwitch(&FilterConnectionS[1], "DISCONNECT", "Disconnect", ISS_ON);
    IUFillSwitchVector(&FilterConnectionSP, FilterConnectionS, 2, getDeviceName(), "CFW_CONNECTION", "Connect",
                       FILTER_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUSaveText(&BayerT[2], "BGGR");

    INDI::FilterInterface::initProperties(FILTER_TAB);

    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = MAX_CFW_TYPES;

    // Set minimum exposure speed to 0.001 seconds
    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.001, 3600, 1, false);

    setDriverInterface(getDriverInterface() | FILTER_INTERFACE);

    return true;
}

void SBIGCCD::ISGetProperties(const char *dev)
{
    INDI::CCD::ISGetProperties(dev);
    defineSwitch(&PortSP);
    loadConfig(true, "DEVICE_PORT_TYPE");
    loadConfig(true, "IP_ADDRESS");
    addAuxControls();
}

bool SBIGCCD::updateProperties()
{
    INDI::CCD::updateProperties();
    if (isConnected())
    {
        defineText(&ProductInfoTP);
        if (IsFanControlAvailable())
        {
            defineSwitch(&FanStateSP);
        }
        if (HasCooler())
        {
            defineSwitch(&CoolerSP);
            defineNumber(&CoolerNP);
        }
        if (hasFilterWheel)
        {
            defineSwitch(&FilterConnectionSP);
            defineSwitch(&FilterTypeSP);
        }
        setupParams();
        if (hasFilterWheel) // If filter type already selected (from config file), then try to connect to CFW
        {
            loadConfig(true, "CFW_TYPE");
            ISwitch *p = IUFindOnSwitch(&FilterTypeSP);
            if (p != NULL && FilterConnectionS[0].s == ISS_OFF)
            {
                DEBUG(INDI::Logger::DBG_DEBUG, "Filter type is already selected and filter is not connected. Will "
                                               "attempt to connect to filter now...");
                CFWConnect();
            }
        }
        timerID = SetTimer(POLLMS);
    }
    else
    {
        deleteProperty(ProductInfoTP.name);
        if (IsFanControlAvailable())
        {
            deleteProperty(FanStateSP.name);
        }
        if (HasCooler())
        {
            deleteProperty(CoolerSP.name);
            deleteProperty(CoolerNP.name);
        }
        if (hasFilterWheel)
        {
            deleteProperty(FilterConnectionSP.name);
            deleteProperty(FilterTypeSP.name);
            deleteProperty(FilterProdcutTP.name);
            if (FilterNameT != NULL)
            {
                deleteProperty(FilterNameTP->name);
            }
        }
        rmTimer(timerID);
    }
    return true;
}

bool SBIGCCD::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, IpTP.name) == 0)
        {
            unsigned long ip = htonl(inet_addr(texts[0]));
            if (ip == INADDR_NONE)
            {
                DEBUGF(INDI::Logger::DBG_ERROR, "Invalid ip address %s.", texts[0]);
                IpTP.s = IPS_ALERT;
                IDSetText(&IpTP, NULL);
                return false;
            }
            IpTP.s = IPS_OK;
            IUUpdateText(&IpTP, texts, names, n);
            IDSetText(&IpTP, NULL);
            return true;
        }
        if (strcmp(name, FilterNameTP->name) == 0)
        {
            INDI::FilterInterface::processText(dev, name, texts, names, n);
            return true;
        }
    }
    return INDI::CCD::ISNewText(dev, name, texts, names, n);
}

bool SBIGCCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, PortSP.name) == 0)
        {
            IUUpdateSwitch(&PortSP, states, names, n);
            if (*((uint32_t *)IUFindOnSwitch(&PortSP)->aux) == DEV_ETH)
            {
                defineText(&IpTP);
            }
            else
            {
                deleteProperty(IpTP.name);
            }
            PortSP.s = IPS_OK;
            IDSetSwitch(&PortSP, NULL);
            return true;
        }
        if (strcmp(name, FanStateSP.name) == 0)
        {
            IUUpdateSwitch(&FanStateSP, states, names, n);
            MiscellaneousControlParams mcp;
            mcp.fanEnable      = FanStateS[0].s == ISS_ON;
            mcp.shutterCommand = SC_LEAVE_SHUTTER;
            mcp.ledState       = LED_OFF;
            if (MiscellaneousControl(&mcp) == CE_NO_ERROR)
            {
                FanStateSP.s = IPS_OK;
                IDSetSwitch(&FanStateSP, mcp.fanEnable == 1 ? "Fan turned On" : "Fan turned Off");
                return true;
            }
            FanStateSP.s = IPS_ALERT;
            IDSetSwitch(&FanStateSP, "Failed to control fan");
            return false;
        }
        if (strcmp(name, FilterTypeSP.name) == 0) // CFW TYPE
        {
            IUResetSwitch(&FilterTypeSP);
            IUUpdateSwitch(&FilterTypeSP, states, names, n);
            FilterTypeSP.s = IPS_OK;
            IDSetSwitch(&FilterTypeSP, NULL);
            return true;
        }
        if (strcmp(name, CoolerSP.name) == 0)
        {
            IUUpdateSwitch(&CoolerSP, states, names, n);
            int state = CoolerS[0].s == ISS_ON;
            if (SetTemperatureRegulation(TemperatureN[0].value, state) == CE_NO_ERROR)
            {
                CoolerSP.s = state ? IPS_OK : IPS_IDLE;
                IDSetSwitch(&CoolerSP, state ? "Cooler turned On" : "Cooler turned Off");
                return true;
            }
            CoolerSP.s = IPS_ALERT;
            IDSetSwitch(&CoolerSP, "Failed to control cooler");
            return false;
        }
        if (strcmp(name, FilterConnectionSP.name) == 0) // CFW CONNECTION
        {
            IUUpdateSwitch(&FilterConnectionSP, states, names, n);
            FilterConnectionSP.s = IPS_BUSY;
            if (FilterConnectionS[0].s == ISS_ON)
            {
                ISwitch *p = IUFindOnSwitch(&FilterTypeSP);
                if (p == NULL)
                {
                    FilterConnectionSP.s = IPS_ALERT;
                    IUResetSwitch(&FilterConnectionSP);
                    FilterConnectionSP.sp[1].s = ISS_ON;
                    IDSetSwitch(&FilterConnectionSP, "Please select CFW type before connecting");
                    return false;
                }
                CFWConnect();
            }
            else
            {
                CFWDisconnect();
            }
            return true;
        }
    }
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

bool SBIGCCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, FilterSlotNP.name) == 0)
        {
            INDI::FilterInterface::processNumber(dev, name, values, names, n);
            return true;
        }
    }
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

bool SBIGCCD::Connect()
{
    if (isConnected())
        return true;
    sim              = isSimulation();
    hasGuideHead     = false;
    hasFilterWheel   = false;
    uint32_t devType = *((uint32_t *)IUFindOnSwitch(&PortSP)->aux);
    char *port       = IUFindOnSwitch(&PortSP)->label;
    if (OpenDevice(devType) == CE_NO_ERROR)
    {
        if (EstablishLink() == CE_NO_ERROR)
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "CCD is connected at port %s", port);
            GetExtendedCCDInfo();
            uint32_t cap = CCD_CAN_ABORT | CCD_CAN_BIN | CCD_CAN_SUBFRAME | CCD_HAS_SHUTTER | CCD_HAS_ST4_PORT;
            if (hasGuideHead)
                cap |= CCD_HAS_GUIDE_HEAD;
            if (isColor)
                cap |= CCD_HAS_BAYER;
            if (GetCameraType() == STI_CAMERA)
            {
                PrimaryCCD.setMinMaxStep("CCD_BINNING", "HOR_BIN", 1, 2, 1, false);
                PrimaryCCD.setMinMaxStep("CCD_BINNING", "VER_BIN", 1, 2, 1, false);
            }
            else
            {
                cap |= CCD_HAS_COOLER;
                IEAddTimer(TEMPERATURE_POLL_MS, SBIGCCD::updateTemperatureHelper, this);
                PrimaryCCD.setMinMaxStep("CCD_BINNING", "HOR_BIN", 1, 9, 1, false);
                PrimaryCCD.setMinMaxStep("CCD_BINNING", "VER_BIN", 1, 9, 1, false);
            }
            SetCCDCapability(cap);
#ifdef ASYNC_READOUT
            pthread_create(&primary_thread, NULL, &grabCCDHelper, this);
#endif
            return true;
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Failed to connect CCD at port %s", port);
        }
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Failed to connect CCD at port %s", port);
    }
    return false;
}

bool SBIGCCD::Disconnect()
{
    if (!isConnected())
        return true;
    useExternalTrackingCCD = false;
    hasGuideHead           = false;
#ifdef ASYNC_READOUT
    pthread_mutex_lock(&condMutex);
    grabPredicate   = GRAB_PRIMARY_CCD;
    terminateThread = true;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&condMutex);
#endif
    if (FilterConnectionS[0].s == ISS_ON)
        CFWDisconnect();
    if (CloseDevice() == CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_SESSION, "CCD is disconnected");
        return true;
    }
    else
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to disconnect CCD");
    }
    return false;
}

bool SBIGCCD::setupParams()
{
    DEBUG(INDI::Logger::DBG_DEBUG, "Retrieving CCD Parameters...");
    float x_pixel_size, y_pixel_size;
    int bit_depth = 16;
    int x_1, y_1, x_2, y_2;
    int wCcd = 0, hCcd = 0, binning = 0;
    double wPixel = 0, hPixel = 0;

    if (getBinningMode(&PrimaryCCD, binning) != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to get primary camera binning mode");
        return false;
    }
    if (getCCDSizeInfo(CCD_IMAGING, binning, wCcd, hCcd, wPixel, hPixel) != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to get primary camera size info");
        return false;
    }

    x_pixel_size = wPixel;
    y_pixel_size = hPixel;
    x_1 = y_1 = 0;
    x_2       = wCcd;
    y_2       = hCcd;
    SetCCDParams(x_2 - x_1, y_2 - y_1, bit_depth, x_pixel_size, y_pixel_size);

    if (HasGuideHead())
    {
        if (getBinningMode(&GuideCCD, binning) != CE_NO_ERROR)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to get guide head binning mode");
            return false;
        }
        if (getCCDSizeInfo(useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING, binning, wCcd, hCcd, wPixel,
                           hPixel) != CE_NO_ERROR)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to get guide head size info");
            return false;
        }
        if (useExternalTrackingCCD && (wCcd <= 0 || hCcd <= 0 || wCcd > MAX_RESOLUTION || hCcd > MAX_RESOLUTION))
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Invalid external tracking camera dimensions, trying regular tracking");
            if (getCCDSizeInfo(CCD_TRACKING, binning, wCcd, hCcd, wPixel, hPixel) != CE_NO_ERROR)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to get external tracking camera size info");
                return false;
            }
            useExternalTrackingCCD = false;
        }
        x_pixel_size = wPixel;
        y_pixel_size = hPixel;
        x_1 = y_1 = 0;
        x_2       = wCcd;
        y_2       = hCcd;
        SetGuiderParams(x_2 - x_1, y_2 - y_1, bit_depth, x_pixel_size, y_pixel_size);
    }

    int nbuf = PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8 + 512;
    PrimaryCCD.setFrameBufferSize(nbuf);
    if (PrimaryCCD.getFrameBuffer() == NULL)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to allocate memory for primary camera buffer");
        return false;
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "Created primary camera buffer %d bytes.", nbuf);

    if (HasGuideHead())
    {
        nbuf = GuideCCD.getXRes() * GuideCCD.getYRes() * GuideCCD.getBPP() / 8 + 512;
        GuideCCD.setFrameBufferSize(nbuf);
        if (GuideCCD.getFrameBuffer() == NULL)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to allocate memory for guide head buffer");
            return false;
        }
        DEBUGF(INDI::Logger::DBG_DEBUG, "Created guide head buffer %d bytes.", nbuf);
    }

    if (HasCooler())
    {
        bool regulationEnabled = false;
        double temp, setPoint, power;
        QueryTemperatureStatus(regulationEnabled, temp, setPoint, power);
        CoolerS[0].s = regulationEnabled ? ISS_ON : ISS_OFF;
        CoolerS[1].s = regulationEnabled ? ISS_OFF : ISS_ON;
        IDSetSwitch(&CoolerSP, NULL);
        CoolerN[0].value = power * 100;
        IDSetNumber(&CoolerNP, NULL);
        TemperatureN[0].min = MIN_CCD_TEMP;
        TemperatureN[0].max = MAX_CCD_TEMP;
        IUUpdateMinMax(&TemperatureNP);
    }

    IUSaveText(&ProductInfoT[0], GetCameraName());
    IUSaveText(&ProductInfoT[1], GetCameraID());
    ProductInfoTP.s = IPS_OK;
    IDSetText(&ProductInfoTP, NULL);
    return true;
}

int SBIGCCD::SetTemperature(double temperature)
{
    if (fabs(temperature - TemperatureN[0].value) < 0.1)
        return 1;
    if (SetTemperatureRegulation(temperature) == CE_NO_ERROR)
    {
        // Set property to busy and poll in ISPoll for CCD temp
        TemperatureRequest = temperature;
        DEBUGF(INDI::Logger::DBG_SESSION, "Temperature set to %+.1fC", temperature);
        if (CoolerS[0].s != ISS_ON)
        {
            CoolerS[0].s = ISS_ON;
            CoolerS[1].s = ISS_OFF;
            CoolerSP.s   = IPS_BUSY;
            IDSetSwitch(&CoolerSP, NULL);
        }
        return 0;
    }
    DEBUG(INDI::Logger::DBG_ERROR, "Failed to set temperature");
    return -1;
}

int SBIGCCD::StartExposure(INDI::CCDChip *targetChip, double duration)
{
    int res, binning, shutter;

    if ((res = getShutterMode(targetChip, shutter)) != CE_NO_ERROR)
    {
        return res;
    }
    if ((res = getBinningMode(targetChip, binning)) != CE_NO_ERROR)
    {
        return res;
    }

    INDI::CCDChip::CCD_FRAME frameType;
    getFrameType(targetChip, &frameType);
    ulong expTime = (ulong)floor(duration * 100.0 + 0.5);
    if (frameType == INDI::CCDChip::BIAS_FRAME) // Flat frame = zero seconds
    {
        expTime = 0;
    }

    unsigned short left   = (unsigned short)targetChip->getSubX();
    unsigned short top    = (unsigned short)targetChip->getSubY();
    unsigned short width  = (unsigned short)targetChip->getSubW() / targetChip->getBinX();
    unsigned short height = (unsigned short)targetChip->getSubH() / targetChip->getBinY();

    int ccd;
    if (targetChip == &PrimaryCCD)
    {
        ccd = CCD_IMAGING;
    }
    else
    {
        ccd = useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING;
    }

    StartExposureParams2 sep;
    sep.ccd          = (unsigned short)ccd;
    sep.abgState     = (unsigned short)ABG_LOW7;
    sep.openShutter  = (unsigned short)shutter;
    sep.exposureTime = expTime;
    sep.readoutMode  = binning;
    sep.left         = left;
    sep.top          = top;
    sep.width        = width;
    sep.height       = height;

    DEBUGF(INDI::Logger::DBG_DEBUG,
           "Exposure params for CCD (%d) openShutter(%d), exposureTime (%ld), binnig (%d), left (%d), top (%d), w(%d), "
           "h(%d)",
           sep.ccd, sep.openShutter, sep.exposureTime, sep.readoutMode, sep.left, sep.top, sep.width, sep.height);

    for (int i = 0; i < MAX_THREAD_RETRIES; i++)
    {
        pthread_mutex_lock(&sbigMutex);
        res = StartExposure(&sep);
        pthread_mutex_unlock(&sbigMutex);
        if (res == CE_NO_ERROR)
        {
            targetChip->setExposureDuration(duration);
            break;
        }
        usleep(MAX_THREAD_WAIT);
    }

    if (res != CE_NO_ERROR)
    {
        return res;
    }

    if (frameType == INDI::CCDChip::LIGHT_FRAME)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Light Frame exposure in progress...");
    }
    else if (frameType == INDI::CCDChip::DARK_FRAME)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Dark Frame exposure in progress...");
    }
    else if (frameType == INDI::CCDChip::FLAT_FRAME)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Flat Frame exposure in progress...");
    }
    else if (frameType == INDI::CCDChip::BIAS_FRAME)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Bias Frame exposure in progress...");
    }
    return res;
}

bool SBIGCCD::StartExposure(float duration)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "Taking %gs exposure on main camera...", ExposureRequest);
    int res = StartExposure(&PrimaryCCD, duration);
    if (res != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Failed to start exposure on main camera");
        return false;
    }
    ExposureRequest = duration;
    gettimeofday(&ExpStart, NULL);
    InExposure = true;
    return true;
}

bool SBIGCCD::StartGuideExposure(float duration)
{
    DEBUGF(INDI::Logger::DBG_SESSION, "Taking %gs exposure on guide head...", ExposureRequest);
    int res = StartExposure(&GuideCCD, duration);
    if (res != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_DEBUG, "Failed to start exposure on guide head");
        return false;
    }
    GuideExposureRequest = duration;
    gettimeofday(&GuideExpStart, NULL);
    InGuideExposure = true;
    return true;
}

int SBIGCCD::AbortExposure(INDI::CCDChip *targetChip)
{
    int ccd;
    if (targetChip == &PrimaryCCD)
    {
        ccd = CCD_IMAGING;
    }
    else
    {
        ccd = useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING;
    }
    EndExposureParams eep;
    eep.ccd = (unsigned short)ccd;
    pthread_mutex_lock(&sbigMutex);
    int res = EndExposure(&eep);
    pthread_mutex_unlock(&sbigMutex);
    return res;
}

bool SBIGCCD::AbortExposure()
{
    int res = CE_NO_ERROR;
    DEBUG(INDI::Logger::DBG_DEBUG, "Aborting primary camera exposure...");
    for (int i = 0; i < MAX_THREAD_RETRIES; i++)
    {
        res = AbortExposure(&PrimaryCCD);
        if (res == CE_NO_ERROR)
        {
            break;
        }
        usleep(MAX_THREAD_WAIT);
    }
    if (res != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to abort primary camera exposure");
        return false;
    }
    InExposure = false;
    DEBUG(INDI::Logger::DBG_DEBUG, "Primary camera exposure aborted");
    return true;
}

bool SBIGCCD::AbortGuideExposure()
{
    int res = CE_NO_ERROR;
    DEBUG(INDI::Logger::DBG_DEBUG, "Aborting guide head exposure...");
    for (int i = 0; i < MAX_THREAD_RETRIES; i++)
    {
        res = AbortExposure(&GuideCCD);
        if (res == CE_NO_ERROR)
        {
            break;
        }
        usleep(MAX_THREAD_WAIT);
    }
    if (res != CE_NO_ERROR)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to abort guide head exposure");
        return false;
    }
    InExposure = false;
    DEBUG(INDI::Logger::DBG_DEBUG, "Guide head exposure aborted");
    return true;
}

bool SBIGCCD::UpdateCCDFrameType(INDI::CCDChip::CCD_FRAME fType)
{
    INDI::CCDChip::CCD_FRAME imageFrameType = PrimaryCCD.getFrameType();
    if (fType != imageFrameType)
    {
        PrimaryCCD.setFrameType(fType);
    }
    return true;
}

bool SBIGCCD::updateFrameProperties(INDI::CCDChip *targetChip)
{
    int wCcd, hCcd, binning;
    double wPixel, hPixel;
    int res = getBinningMode(targetChip, binning);
    if (res != CE_NO_ERROR)
    {
        return false;
    }
    if (targetChip == &PrimaryCCD)
    {
        res = getCCDSizeInfo(CCD_IMAGING, binning, wCcd, hCcd, wPixel, hPixel);
    }
    else
    {
        res = getCCDSizeInfo(useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING, binning, wCcd, hCcd, wPixel,
                             hPixel);
    }
    if (res == CE_NO_ERROR)
    {
        // SBIG returns binned width and height, which is OK, but we used unbinned width and height across all drivers to be consistent.
        wCcd *= targetChip->getBinX();
        hCcd *= targetChip->getBinY();
        targetChip->setResolution(wCcd, hCcd);
        if (targetChip == &PrimaryCCD)
        {
            UpdateCCDFrame(0, 0, wCcd, hCcd);
        }
        else
        {
            UpdateGuiderFrame(0, 0, wCcd, hCcd);
        }
        return true;
    }
    DEBUG(INDI::Logger::DBG_DEBUG, "Failed to updat frame properties");
    return false;
}

bool SBIGCCD::UpdateCCDFrame(int x, int y, int w, int h)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "The final main camera image area is (%ld, %ld), (%ld, %ld)", x, y, w, h);
    PrimaryCCD.setFrame(x, y, w, h);
    int nbuf = (w * h * PrimaryCCD.getBPP() / 8) + 512;
    PrimaryCCD.setFrameBufferSize(nbuf);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Created primary camera buffer %d bytes", nbuf);
    return true;
}

bool SBIGCCD::UpdateGuiderFrame(int x, int y, int w, int h)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "The final guide head image area is (%ld, %ld), (%ld, %ld)", x, y, w, h);
    GuideCCD.setFrame(x, y, w, h);
    int nbuf = (w * h * GuideCCD.getBPP() / 8) + 512;
    GuideCCD.setFrameBufferSize(nbuf);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Created guide head buffer %d bytes", nbuf);
    return true;
}

bool SBIGCCD::UpdateCCDBin(int binx, int biny)
{
    if (binx != biny)
    {
        biny = binx;
    }
    if (GetCameraType() == STI_CAMERA)
    {
        if (binx < 1 || binx > 2)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Failed to update main camera binning mode, use 1x1 or 2x2");
            return false;
        }
    }
    else if ((binx < 1 || binx > 3) && binx != 9)
    {
        DEBUG(
            INDI::Logger::DBG_ERROR,
            "Failed to update main camera binning mode, use 1x1, 2x2, 3x3 or 9x9"); // expand conditions to supply 9x9 binning
        return false;
    }
    PrimaryCCD.setBin(binx, biny);
    return updateFrameProperties(&PrimaryCCD);
}

bool SBIGCCD::UpdateGuiderBin(int binx, int biny)
{
    if (binx != biny)
        biny = binx;
    if (binx < 1 || binx > 3)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to update guide head binning mode, use 1x1, 2x2 or 3x3");
        return false;
    }
    GuideCCD.setBin(binx, biny);
    return updateFrameProperties(&GuideCCD);
}

IPState SBIGCCD::GuideNorth(float duration)
{
    ActivateRelayParams rp;
    rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
    unsigned short dur                              = duration / 10.0;
    rp.tYMinus                                      = dur;
    ActivateRelay(&rp);
    return IPS_OK;
}

IPState SBIGCCD::GuideSouth(float duration)
{
    ActivateRelayParams rp;
    rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
    unsigned short dur                              = duration / 10.0;
    rp.tYPlus                                       = dur;
    ActivateRelay(&rp);
    return IPS_OK;
}

IPState SBIGCCD::GuideEast(float duration)
{
    ActivateRelayParams rp;
    rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
    unsigned short dur                              = duration / 10.0;
    rp.tXPlus                                       = dur;
    ActivateRelay(&rp);
    return IPS_OK;
}

IPState SBIGCCD::GuideWest(float duration)
{
    ActivateRelayParams rp;
    rp.tXMinus = rp.tXPlus = rp.tYMinus = rp.tYPlus = 0;
    unsigned short dur                              = duration / 10.0;
    rp.tXMinus                                      = dur;
    ActivateRelay(&rp);
    return IPS_OK;
}

float SBIGCCD::CalcTimeLeft(timeval start, float req)
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

#ifdef ASYNC_READOUT
void *SBIGCCD::grabCCDHelper(void *context)
{
    return ((SBIGCCD *)context)->grabCCD();
}

void *SBIGCCD::grabCCD()
{
    DEBUG(INDI::Logger::DBG_DEBUG, "grabCCD thread started...");
    INDI::CCDChip *targetChip = NULL;
    pthread_mutex_lock(&condMutex);
    while (true)
    {
        while (grabPredicate == GRAB_NO_CCD)
        {
            pthread_cond_wait(&cv, &condMutex);
        }
        targetChip    = (grabPredicate == GRAB_PRIMARY_CCD) ? &PrimaryCCD : &GuideCCD;
        grabPredicate = GRAB_NO_CCD;
        if (terminateThread)
            break;
        pthread_mutex_unlock(&condMutex);
        if (grabImage(targetChip) == false)
        {
            targetChip->setExposureFailed();
        }
        pthread_mutex_lock(&condMutex);
    }
    pthread_mutex_unlock(&condMutex);
    DEBUG(INDI::Logger::DBG_DEBUG, "grabCCD thread finished");
    return 0;
}
#endif

bool SBIGCCD::grabImage(INDI::CCDChip *targetChip)
{
    unsigned short left   = (unsigned short)targetChip->getSubX() / targetChip->getBinX();
    unsigned short top    = (unsigned short)targetChip->getSubY() / targetChip->getBinX();
    unsigned short width  = (unsigned short)targetChip->getSubW() / targetChip->getBinX();
    unsigned short height = (unsigned short)targetChip->getSubH() / targetChip->getBinY();
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s readout in progress...",
           targetChip == &PrimaryCCD ? "Primary camera" : "Guide head");
    if (isSimulation())
    {
        uint8_t *image = targetChip->getFrameBuffer();
        for (int i = 0; i < height * 2; i++)
        {
            for (int j = 0; j < width; j++)
            {
                image[i * width + j] = rand() % 255;
            }
        }
    }
    else
    {
        unsigned short *buffer = (unsigned short *)targetChip->getFrameBuffer();
        int res                = 0;
        for (int i = 0; i < MAX_THREAD_RETRIES; i++)
        {
            res = readoutCCD(left, top, width, height, buffer, targetChip);
            if (res == CE_NO_ERROR)
                break;
            DEBUGF(INDI::Logger::DBG_DEBUG, "Readout error, retrying...", res);
            usleep(MAX_THREAD_WAIT);
        }
        if (res != CE_NO_ERROR)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "%s readout error",
                   targetChip == &PrimaryCCD ? "Primary camera" : "Guide head");
            return false;
        }
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "%s readout complete", targetChip == &PrimaryCCD ? "Primary camera" : "Guide head");
    ExposureComplete(targetChip);
    return true;
}

bool SBIGCCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);
    IUSaveConfigSwitch(fp, &PortSP);
    IUSaveConfigText(fp, &IpTP);

    if (FilterNameT)
        INDI::FilterInterface::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &FilterTypeSP);
    return true;
}

void SBIGCCD::TimerHit()
{
    long timeleft       = 1e6;
    INDI::CCDChip *targetChip = NULL;
    if (isConnected() == false)
    {
        return;
    }
    if (InExposure)
    {
        targetChip = &PrimaryCCD;
        timeleft   = CalcTimeLeft(ExpStart, ExposureRequest);
        if (isExposureDone(targetChip))
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Primay camera exposure done, downloading image...");
            targetChip->setExposureLeft(0);
            InExposure = false;
#ifdef ASYNC_READOUT
            pthread_mutex_lock(&condMutex);
            grabPredicate = GRAB_PRIMARY_CCD;
            pthread_mutex_unlock(&condMutex);
            pthread_cond_signal(&cv);
#else
            if (grabImage(targetChip) == false)
                targetChip->setExposureFailed();
#endif
        }
        else
        {
            targetChip->setExposureLeft(timeleft);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Primary camera exposure in progress with %ld seconds left...", timeleft);
        }
    }
    if (InGuideExposure)
    {
        targetChip = &GuideCCD;
        timeleft   = CalcTimeLeft(GuideExpStart, GuideExposureRequest);
        if (isExposureDone(targetChip))
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Guide head exposure done, downloading image...");
            targetChip->setExposureLeft(0);
            InGuideExposure = false;
#ifdef ASYNC_READOUT
            pthread_mutex_lock(&condMutex);
            grabPredicate = GRAB_GUIDE_CCD;
            pthread_mutex_unlock(&condMutex);
            pthread_cond_signal(&cv);
#else
            if (grabImage(targetChip) == false)
                targetChip->setExposureFailed();
#endif
        }
        else
        {
            targetChip->setExposureLeft(timeleft);
            DEBUGF(INDI::Logger::DBG_DEBUG, "Guide head exposure in progress with %ld seconds left...", timeleft);
        }
    }
    SetTimer(POLLMS);
    return;
}

//=========================================================================

int SBIGCCD::GetDriverInfo(GetDriverInfoParams *gdip, void *res)
{
    return SBIGUnivDrvCommand(CC_GET_DRIVER_INFO, gdip, res);
}

int SBIGCCD::SetDriverHandle(SetDriverHandleParams *sdhp)
{
    int res = SBIGUnivDrvCommand(CC_SET_DRIVER_HANDLE, sdhp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_SET_DRIVER_HANDLE -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::GetDriverHandle(GetDriverHandleResults *gdhr)
{
    int res = SBIGUnivDrvCommand(CC_GET_DRIVER_HANDLE, 0, gdhr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_DRIVER_HANDLE -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::StartExposure(StartExposureParams2 *sep)
{
    if (isSimulation())
    {
        return CE_NO_ERROR;
    }
    int res = SBIGUnivDrvCommand(CC_START_EXPOSURE2, sep, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_START_EXPOSURE2 -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::EndExposure(EndExposureParams *eep)
{
    if (isSimulation())
    {
        return CE_NO_ERROR;
    }
    int res = SBIGUnivDrvCommand(CC_END_EXPOSURE, eep, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_END_EXPOSURE -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::StartReadout(StartReadoutParams *srp)
{
    int res = SBIGUnivDrvCommand(CC_START_READOUT, srp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_START_READOUT -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::ReadoutLine(ReadoutLineParams *rlp, unsigned short *results, bool bSubtract)
{
    int res;
    if (bSubtract)
    {
        res = SBIGUnivDrvCommand(CC_READ_SUBTRACT_LINE, rlp, results);
    }
    else
    {
        res = SBIGUnivDrvCommand(CC_READOUT_LINE, rlp, results);
    }
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_READ_SUBTRACT_LINE/CC_READOUT_LINE -> (%s)", __FUNCTION__,
               GetErrorString(res));
    }
    return res;
}

int SBIGCCD::DumpLines(DumpLinesParams *dlp)
{
    int res = SBIGUnivDrvCommand(CC_DUMP_LINES, dlp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_DUMP_LINES -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::EndReadout(EndReadoutParams *erp)
{
    int res = SBIGUnivDrvCommand(CC_END_READOUT, erp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_END_READOUT -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::SetTemperatureRegulation(SetTemperatureRegulationParams *strp)
{
    int res = SBIGUnivDrvCommand(CC_SET_TEMPERATURE_REGULATION, strp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_SET_TEMPERATURE_REGULATION -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::SetTemperatureRegulation(double temperature, bool enable)
{
    int res;
    SetTemperatureRegulationParams strp;
    if (isSimulation())
    {
        TemperatureN[0].value = temperature;
        return CE_NO_ERROR;
    }
    if (CheckLink())
    {
        strp.regulation  = enable ? REGULATION_ON : REGULATION_OFF;
        strp.ccdSetpoint = CalcSetpoint(temperature);
        res              = SBIGUnivDrvCommand(CC_SET_TEMPERATURE_REGULATION, &strp, 0);
    }
    else
    {
        res = CE_DEVICE_NOT_OPEN;
    }
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_SET_TEMPERATURE_REGULATION -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::QueryTemperatureStatus(bool &enabled, double &ccdTemp, double &setpointTemp, double &power)
{
    int res;
    QueryTemperatureStatusResults qtsr;
    if (isSimulation())
    {
        enabled      = (CoolerS[0].s == ISS_ON);
        ccdTemp      = TemperatureN[0].value;
        setpointTemp = ccdTemp;
        power        = enabled ? 0.5 : 0;
        return CE_NO_ERROR;
    }
    if (CheckLink())
    {
        res = SBIGUnivDrvCommand(CC_QUERY_TEMPERATURE_STATUS, 0, &qtsr);
        if (res == CE_NO_ERROR)
        {
            enabled      = (qtsr.enabled != 0);
            ccdTemp      = CalcTemperature(CCD_THERMISTOR, qtsr.ccdThermistor);
            setpointTemp = CalcTemperature(CCD_THERMISTOR, qtsr.ccdSetpoint);
            power        = qtsr.power / 255.0;
            DEBUGF(INDI::Logger::DBG_DEBUG, "%s: Regulation Enabled (%s) ccdTemp (%g) setpointTemp (%g) power (%g)",
                   __FUNCTION__, enabled ? "True" : "False", ccdTemp, setpointTemp, power);
        }
    }
    else
    {
        res = CE_DEVICE_NOT_OPEN;
    }
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_SET_TEMPERATURE_REGULATION -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

unsigned short SBIGCCD::CalcSetpoint(double temperature)
{
    // Calculate 'setpoint' from the temperature T in degr. of Celsius.
    double expo = (log(R_RATIO_CCD) * (T0 - temperature)) / DT_CCD;
    double r    = R0 * exp(expo);
    return ((unsigned short)(((MAX_AD / (R_BRIDGE_CCD / r + 1.0)) + 0.5)));
}

double SBIGCCD::CalcTemperature(short thermistorType, short setpoint)
{
    double r, expo, rBridge, rRatio, dt;
    switch (thermistorType)
    {
        case AMBIENT_THERMISTOR:
            rBridge = R_BRIDGE_AMBIENT;
            rRatio  = R_RATIO_AMBIENT;
            dt      = DT_AMBIENT;
            break;
        case CCD_THERMISTOR:
        default:
            rBridge = R_BRIDGE_CCD;
            rRatio  = R_RATIO_CCD;
            dt      = DT_CCD;
            break;
    }
    // Calculate temperature T in degr. Celsius from the 'setpoint'
    r    = rBridge / ((MAX_AD / setpoint) - 1.0);
    expo = log(r / R0) / log(rRatio);
    return (T0 - dt * expo);
}

int SBIGCCD::ActivateRelay(ActivateRelayParams *arp)
{
    int res = SBIGUnivDrvCommand(CC_ACTIVATE_RELAY, arp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_ACTIVATE_RELAY -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::PulseOut(PulseOutParams *pop)
{
    int res = SBIGUnivDrvCommand(CC_PULSE_OUT, pop, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_PULSE_OUT -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::TxSerialBytes(TXSerialBytesParams *txsbp, TXSerialBytesResults *txsbr)
{
    int res = SBIGUnivDrvCommand(CC_TX_SERIAL_BYTES, txsbp, txsbr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_TX_SERIAL_BYTES -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::GetSerialStatus(GetSerialStatusResults *gssr)
{
    int res = SBIGUnivDrvCommand(CC_GET_SERIAL_STATUS, 0, gssr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_SERIAL_STATUS -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::AoTipTilt(AOTipTiltParams *aottp)
{
    int res = SBIGUnivDrvCommand(CC_AO_TIP_TILT, aottp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_AO_TIP_TILT -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::AoDelay(AODelayParams *aodp)
{
    int res = SBIGUnivDrvCommand(CC_AO_DELAY, aodp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_AO_DELAY -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::CFW(CFWParams *CFWp, CFWResults *CFWr)
{
    int res = SBIGUnivDrvCommand(CC_CFW, CFWp, CFWr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::EstablishLink()
{
    EstablishLinkParams elp;
    EstablishLinkResults elr;

    elp.sbigUseOnly = 0;
    int res         = SBIGUnivDrvCommand(CC_ESTABLISH_LINK, &elp, &elr);
    if (res == CE_NO_ERROR)
    {
        SetCameraType((CAMERA_TYPE)elr.cameraType);
        SetLinkStatus(true);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_ESTABLISH_LINK -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::GetCcdInfo(GetCCDInfoParams *gcp, void *gcr)
{
    int res = SBIGUnivDrvCommand(CC_GET_CCD_INFO, gcp, gcr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_CCD_INFO -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::getCCDSizeInfo(int ccd, int binning, int &frmW, int &frmH, double &pixW, double &pixH)
{
    GetCCDInfoParams gcp;
    GetCCDInfoResults0 gcr;
    if (isSimulation())
    {
        if (ccd == CCD_IMAGING)
        {
            frmW = 1024;
            frmH = 1024;
        }
        else
        {
            frmW = 512;
            frmH = 512;
        }
        pixW = 5.2;
        pixH = 5.2;
        return CE_NO_ERROR;
    }
    gcp.request = ccd;
    int res     = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gcp, &gcr);
    if (res == CE_NO_ERROR)
    {
        frmW = gcr.readoutInfo[binning].width;
        frmH = gcr.readoutInfo[binning].height;
        pixW = BcdPixel2double(gcr.readoutInfo[binning].pixelWidth);
        pixH = BcdPixel2double(gcr.readoutInfo[binning].pixelHeight);
        DEBUGF(INDI::Logger::DBG_DEBUG,
               "%s: CC_GET_CCD_INFO -> binning (%d) width (%d) height (%d) pixW (%g) pixH (%g)", __FUNCTION__, binning,
               frmW, frmH, pixW, pixH);
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_CCD_INFO -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::QueryCommandStatus(QueryCommandStatusParams *qcsp, QueryCommandStatusResults *qcsr)
{
    int res = SBIGUnivDrvCommand(CC_QUERY_COMMAND_STATUS, qcsp, qcsr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_QUERY_COMMAND_STATUS -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::MiscellaneousControl(MiscellaneousControlParams *mcp)
{
    int res = SBIGUnivDrvCommand(CC_MISCELLANEOUS_CONTROL, mcp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_QUERY_COMMAND_STATUS -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::ReadOffset(ReadOffsetParams *rop, ReadOffsetResults *ror)
{
    int res = SBIGUnivDrvCommand(CC_READ_OFFSET, rop, ror);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_READ_OFFSET -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::GetLinkStatus(GetLinkStatusResults *glsr)
{
    int res = SBIGUnivDrvCommand(CC_GET_LINK_STATUS, glsr, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_LINK_STATUS -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

char *SBIGCCD::GetErrorString(int err)
{
    GetErrorStringParams gesp;
    gesp.errorNo = err;
    static GetErrorStringResults gesr;
    int res = SBIGUnivDrvCommand(CC_GET_ERROR_STRING, &gesp, &gesr);
    if (res == CE_NO_ERROR)
    {
        return gesr.errorString;
    }
    static char str[128];
    sprintf(str, "No error string found! Error code: %d", err);
    return str;
}

int SBIGCCD::SetDriverControl(SetDriverControlParams *sdcp)
{
    int res = SBIGUnivDrvCommand(CC_SET_DRIVER_CONTROL, sdcp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_SET_DRIVER_CONTROL -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::GetDriverControl(GetDriverControlParams *gdcp, GetDriverControlResults *gdcr)
{
    int res = SBIGUnivDrvCommand(CC_GET_DRIVER_CONTROL, gdcp, gdcr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_DRIVER_CONTROL -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::UsbAdControl(USBADControlParams *usbadcp)
{
    int res = SBIGUnivDrvCommand(CC_USB_AD_CONTROL, usbadcp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_USB_AD_CONTROL -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::QueryUsb(QueryUSBResults *qusbr)
{
    int res = SBIGUnivDrvCommand(CC_QUERY_USB, 0, qusbr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_QUERY_USB -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::RwUsbI2c(RWUSBI2CParams *rwusbi2cp)
{
    int res = SBIGUnivDrvCommand(CC_RW_USB_I2C, rwusbi2cp, 0);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_RW_USB_I2C -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

int SBIGCCD::BitIo(BitIOParams *biop, BitIOResults *bior)
{
    int res = SBIGUnivDrvCommand(CC_BIT_IO, biop, bior);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_BIT_IO -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    return res;
}

const char *SBIGCCD::GetCameraName()
{
    if (isSimulation())
    {
        return "Simulated camera";
    }
    GetCCDInfoParams gccdip;
    static GetCCDInfoResults0 gccdir;
    gccdip.request = CCD_INFO_IMAGING;
    int res        = SBIGUnivDrvCommand(CC_GET_CCD_INFO, &gccdip, &gccdir);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_CCD_INFO -> (%s)", __FUNCTION__, GetErrorString(res));
        return "Unknown camera";
    }
    if (gccdir.cameraType == NO_CAMERA)
    {
        return "No camera";
    }
    return gccdir.name;
}

const char *SBIGCCD::GetCameraID()
{
    if (isSimulation())
    {
        return "Simulated ID";
    }
    GetCCDInfoParams gccdip;
    static GetCCDInfoResults2 gccdir2;
    gccdip.request = 2;
    int res        = GetCcdInfo(&gccdip, (void *)&gccdir2);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_GET_CCD_INFO -> (%s)", __FUNCTION__, GetErrorString(res));
        return "Unknown ID";
    }
    return gccdir2.serialNumber;
}

void SBIGCCD::GetExtendedCCDInfo()
{
    int res = 0;
    GetCCDInfoParams gccdip;
    GetCCDInfoResults4 imaging_ccd_results4;
    GetCCDInfoResults4 tracking_ccd_results4;
    GetCCDInfoResults6 results6;

    if (isSimulation())
    {
        hasGuideHead   = true;
        hasFilterWheel = true;
        return;
    }
    gccdip.request = 4;
    if (GetCcdInfo(&gccdip, (void *)&imaging_ccd_results4) == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "CCD_IMAGING Extended CCD Info 4. CapabilitiesBit: (%u) Dump Extra (%u)",
               imaging_ccd_results4.capabilitiesBits, imaging_ccd_results4.dumpExtra);
    }
    gccdip.request = 5;
    if (GetCcdInfo(&gccdip, (void *)&tracking_ccd_results4) == CE_NO_ERROR)
    {
        hasGuideHead = true;
        DEBUGF(INDI::Logger::DBG_DEBUG, "TRACKING_CCD Extended CCD Info 4. CapabilitiesBit: (%u) Dump Extra (%u)",
               tracking_ccd_results4.capabilitiesBits, tracking_ccd_results4.dumpExtra);
        if (tracking_ccd_results4.capabilitiesBits & CB_CCD_EXT_TRACKER_YES)
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "External tracking CCD detected.");
            useExternalTrackingCCD = true;
        }
        else
        {
            useExternalTrackingCCD = false;
        }
    }
    else
    {
        hasGuideHead = false;
        DEBUGF(INDI::Logger::DBG_DEBUG, "TRACKING_CCD Error getting extended CCD Info 4 (%s). No guide head detected.",
               GetErrorString(res));
    }
    gccdip.request = 6;
    if (GetCcdInfo(&gccdip, (void *)&results6) == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Extended CCD Info 6. Camerabit: (%ld) CCD bits (%ld) Extra bit (%ld)",
               results6.cameraBits, results6.ccdBits, results6.extraBits);
        if (results6.ccdBits & 0x0001)
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Color CCD detected.");
            isColor = true;
            DEBUGF(INDI::Logger::DBG_DEBUG, "Detected color matrix is %s.",
                   (results6.ccdBits & 0x0002) ? "Truesense" : "Bayer");
        }
        else
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "Mono CCD detected.");
            isColor = false;
        }
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Error getting extended CCD Info 6 (%s)", GetErrorString(res));
    }
    CFWParams CFWp;
    CFWResults CFWr;
    CFWp.cfwModel   = CFWSEL_AUTO;
    CFWp.cfwCommand = CFWC_GET_INFO;
    CFWp.cfwParam1  = CFWG_FIRMWARE_VERSION;
    if (SBIGUnivDrvCommand(CC_CFW, &CFWp, &CFWr) == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "Fitler wheel detected (firmware %ld).", CFWr.cfwResult1);
        hasFilterWheel = true;
    }
    else
    {
        hasFilterWheel = false;
    }
}

//==========================================================================
/*int SBIGCCD::SetDeviceName(const char *name) {
  int res = CE_NO_ERROR;
  if (strlen(name) < PATH_MAX) {
    strcpy(m_dev_name, name);
  } else {
    res = CE_BAD_PARAMETER;
  }
  return res;
}*/

//==========================================================================
// SBIGUnivDrvCommand:
// Bottleneck function for all calls to the driver that logs the command
// and error. First it activates our handle and then it calls the driver.
// Activating the handle first allows having multiple instances of this
// class dealing with multiple cameras on different communications port.
// Also allows direct access to the SBIG Universal Driver after the driver
// has been opened.

int SBIGCCD::SBIGUnivDrvCommand(PAR_COMMAND command, void *params, void *results)
{
    int res;
    SetDriverHandleParams sdhp;
    if (isSimulation())
    {
        return CE_NO_ERROR;
    }
    // Make sure we have a valid handle to the driver.
    if (GetDriverHandle() == INVALID_HANDLE_VALUE)
    {
        res = CE_DRIVER_NOT_OPEN;
    }
    else
    {
        // Handle is valid so install it in the driver.
        sdhp.handle = GetDriverHandle();
        res         = ::SBIGUnivDrvCommand(CC_SET_DRIVER_HANDLE, &sdhp, 0);
#if !defined(OSX_EMBEDED_MODE)
        if (res == CE_FAKE_DRIVER)
        {
            // The user is using the dummy driver. Tell him to download the real driver
            IDMessage(getDeviceName(), "Error: SBIG Dummy Driver is being used now. You can only control your camera "
                                       "by downloading SBIG driver from INDI website @ indi.sf.net");
        }
        else
#endif
            if (res == CE_NO_ERROR)
        {
            res = ::SBIGUnivDrvCommand(command, params, results);
        }
    }
    return res;
}

bool SBIGCCD::CheckLink()
{
    if (GetCameraType() != NO_CAMERA && GetLinkStatus())
    {
        return true;
    }
    return false;
}

/*int SBIGCCD::getNumberOfINDI::CCDChips()
{
    int res;

    switch(GetCameraType())
    {
        case ST237_CAMERA:
        case ST5C_CAMERA:
        case ST402_CAMERA:
        case STI_CAMERA:
        case STT_CAMERA:
        case STF_CAMERA:
                    res = 1;
                    break;
        case ST7_CAMERA:
        case ST8_CAMERA:
        case ST9_CAMERA:
        case ST10_CAMERA:
        case ST2K_CAMERA:
                    res = 2;
                    break;
        case STL_CAMERA:
                    res = 3;
                    break;
        case NO_CAMERA:
        default:
                    res = 0;
                    break;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s Camera Type (%d) Number of chips (%d)", __FUNCTION__, GetCameraType(), res);
    return res;
}*/

//==========================================================================

bool SBIGCCD::IsFanControlAvailable()
{
    CAMERA_TYPE camera = GetCameraType();
    if (camera == ST5C_CAMERA || camera == ST402_CAMERA || camera == STI_CAMERA)
    {
        return false;
    }
    return true;
}

double SBIGCCD::BcdPixel2double(ulong bcd)
{
    double value = 0.0;
    double digit = 0.01;
    for (int i = 0; i < 8; i++)
    {
        value += (bcd & 0x0F) * digit;
        digit *= 10.0;
        bcd >>= 4;
    }
    return value;
}

void SBIGCCD::InitVars()
{
    SetFileDescriptor();
    SetCameraType();
    SetLinkStatus();
}

//==========================================================================

int SBIGCCD::getBinningMode(INDI::CCDChip *targetChip, int &binning)
{
    int res = CE_NO_ERROR;
    if (targetChip->getBinX() == 1 && targetChip->getBinY() == 1)
    {
        binning = CCD_BIN_1x1_I;
    }
    else if (targetChip->getBinX() == 2 && targetChip->getBinY() == 2)
    {
        binning = CCD_BIN_2x2_I;
    }
    else if (targetChip->getBinX() == 3 && targetChip->getBinY() == 3)
    {
        binning = CCD_BIN_3x3_I;
    }
    else if (targetChip->getBinX() == 9 && targetChip->getBinY() == 9)
    {
        binning = CCD_BIN_9x9_I;
    }
    else
    {
        res = CE_BAD_PARAMETER;
        DEBUG(INDI::Logger::DBG_ERROR, "Bad CCD binning mode, use 1x1, 2x2, 3x3 or 9x9");
    }
    return res;
}

int SBIGCCD::getFrameType(INDI::CCDChip *targetChip, INDI::CCDChip::CCD_FRAME *frameType)
{
    *frameType = targetChip->getFrameType();
    return CE_NO_ERROR;
}

int SBIGCCD::getShutterMode(INDI::CCDChip *targetChip, int &shutter)
{
    int res = CE_NO_ERROR;
    INDI::CCDChip::CCD_FRAME frameType;
    getFrameType(targetChip, &frameType);
    int ccd = CCD_IMAGING;
    if (targetChip == &PrimaryCCD)
    {
        ccd = CCD_IMAGING;
    }
    else if (targetChip == &GuideCCD)
    {
        ccd = useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING;
    }
    if (frameType == INDI::CCDChip::LIGHT_FRAME || frameType == INDI::CCDChip::FLAT_FRAME)
    {
        if (ccd == CCD_EXT_TRACKING)
        {
            shutter = SC_OPEN_EXT_SHUTTER;
        }
        else
        {
            shutter = SC_OPEN_SHUTTER;
        }
    }
    else if (frameType == INDI::CCDChip::DARK_FRAME || frameType == INDI::CCDChip::BIAS_FRAME)
    {
        if (ccd == CCD_EXT_TRACKING)
        {
            shutter = SC_CLOSE_EXT_SHUTTER;
        }
        else
        {
            shutter = SC_CLOSE_SHUTTER;
        }
    }
    else
    {
        res = CE_OS_ERROR;
        DEBUGF(INDI::Logger::DBG_ERROR, "Unknown selected CCD frame type %s", targetChip->getFrameTypeName(frameType));
    }
    return res;
}

bool SBIGCCD::SelectFilter(int position)
{
    CFWResults CFWr;
    int res = CFWGoto(&CFWr, position);
    if (res == CE_NO_ERROR)
    {
        int type = GetCFWSelType();
        if (type == CFWSEL_CFW6A || type == CFWSEL_CFW8)
        {
            DEBUG(INDI::Logger::DBG_SESSION, "CFW position reached");
            CFWr.cfwPosition = position;
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_SESSION, "CFW position %d reached.", CFWr.cfwPosition);
        }
        SelectFilterDone(CurrentFilter = CFWr.cfwPosition);
        return true;
    }
    else
    {
        FilterSlotNP.s = IPS_ALERT;
        IDSetNumber(&FilterSlotNP, NULL);
        DEBUG(INDI::Logger::DBG_SESSION, "Failed to reach position");
        return false;
    }
    return false;
}

int SBIGCCD::QueryFilter()
{
    return CurrentFilter;
}

void SBIGCCD::updateTemperatureHelper(void *p)
{
    if (static_cast<SBIGCCD *>(p)->isConnected())
        static_cast<SBIGCCD *>(p)->updateTemperature();
}

void SBIGCCD::updateTemperature()
{
    bool enabled;
    double ccdTemp, setpointTemp, percentTE, power;
    pthread_mutex_lock(&sbigMutex);
    int res = QueryTemperatureStatus(enabled, ccdTemp, setpointTemp, percentTE);
    pthread_mutex_unlock(&sbigMutex);
    if (res == CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "ccdTemp: %g setpointTemp: %g TEMP_DIFF %g", ccdTemp, setpointTemp, TEMP_DIFF);
        power = 100.0 * percentTE;

        // Compare the current temperature against the setpoint value:
        if (fabs(setpointTemp - ccdTemp) <= TEMP_DIFF)
        {
            TemperatureNP.s = IPS_OK;
        }
        else if (power == 0)
        {
            TemperatureNP.s = IPS_IDLE;
        }
        else
        {
            TemperatureNP.s = IPS_BUSY;
            DEBUGF(INDI::Logger::DBG_DEBUG, "CCD temperature %+.1f [C], TE cooler: %.1f [%%].", ccdTemp, power);
        }
        TemperatureN[0].value = ccdTemp;
        // Check the TE cooler if inside the range:
        if (power <= CCD_COOLER_THRESHOLD)
        {
            CoolerNP.s = IPS_OK;
        }
        else
        {
            CoolerNP.s = IPS_BUSY;
        }
        CoolerN[0].value = power;
        IDSetNumber(&TemperatureNP, NULL);
        IDSetNumber(&CoolerNP, 0);
    }
    else
    {
        // ignore share errors
        if (res == CE_SHARE_ERROR)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Erro reading temperature. %s", GetErrorString(res));
            TemperatureNP.s = IPS_IDLE;
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Erro reading temperature. %s", GetErrorString(res));
            TemperatureNP.s = IPS_ALERT;
        }
        IDSetNumber(&TemperatureNP, NULL);
    }
    IEAddTimer(TEMPERATURE_POLL_MS, SBIGCCD::updateTemperatureHelper, this);
}

bool SBIGCCD::isExposureDone(INDI::CCDChip *targetChip)
{
    int ccd;
    if (isSimulation())
    {
        long timeleft = 1e6;
        if (targetChip == &PrimaryCCD)
        {
            timeleft = CalcTimeLeft(ExpStart, ExposureRequest);
        }
        else
        {
            timeleft = CalcTimeLeft(GuideExpStart, GuideExposureRequest);
        }
        if (timeleft <= 0)
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    if (targetChip == &PrimaryCCD)
    {
        ccd = CCD_IMAGING;
    }
    else
    {
        ccd = useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING;
    }
    EndExposureParams eep;
    QueryCommandStatusParams qcsp;
    QueryCommandStatusResults qcsr;
    // Query command status:
    qcsp.command = CC_START_EXPOSURE2;
    pthread_mutex_lock(&sbigMutex);
    int res = QueryCommandStatus(&qcsp, &qcsr);
    if (res != CE_NO_ERROR)
    {
        pthread_mutex_unlock(&sbigMutex);
        return false;
    }
    int mask = 12; // Tracking & external tracking CCD chip mask.
    if (ccd == CCD_IMAGING)
    {
        mask = 3; // Imaging chip mask.
    }
    // Check exposure progress:
    if ((qcsr.status & mask) != mask)
    {
        // The exposure is still in progress, decrement an
        // exposure time:
        pthread_mutex_unlock(&sbigMutex);
        return false;
    }
    // Exposure done - update client's property:
    eep.ccd = ccd;
    EndExposure(&eep);
    pthread_mutex_unlock(&sbigMutex);
    return true;
}

//==========================================================================

int SBIGCCD::readoutCCD(unsigned short left, unsigned short top, unsigned short width, unsigned short height,
                        unsigned short *buffer, INDI::CCDChip *targetChip)
{
    int h, ccd, binning, res;
    if (targetChip == &PrimaryCCD)
    {
        ccd = CCD_IMAGING;
    }
    else
    {
        ccd = useExternalTrackingCCD ? CCD_EXT_TRACKING : CCD_TRACKING;
    }
    if ((res = getBinningMode(targetChip, binning)) != CE_NO_ERROR)
    {
        return res;
    }
    StartReadoutParams srp;
    srp.ccd         = ccd;
    srp.readoutMode = binning;
    srp.left        = left;
    srp.top         = top;
    srp.width       = width;
    srp.height      = height;
    pthread_mutex_lock(&sbigMutex);
    res = StartReadout(&srp);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s readoutCCD - StartReadout error! (%s)",
               (targetChip == &PrimaryCCD) ? "Primary" : "Guide", GetErrorString(res));
        pthread_mutex_unlock(&sbigMutex);
        return res;
    }
    ReadoutLineParams rlp;
    rlp.ccd         = ccd;
    rlp.readoutMode = binning;
    rlp.pixelStart  = left;
    rlp.pixelLength = width;
    for (h = 0; h < height; h++)
    {
        ReadoutLine(&rlp, buffer + (h * width), false);
    }
    EndReadoutParams erp;
    erp.ccd = ccd;
    if ((res = EndReadout(&erp)) != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s readoutCCD - EndReadout error! (%s)",
               (targetChip == &PrimaryCCD) ? "Primary" : "Guide", GetErrorString(res));
        pthread_mutex_unlock(&sbigMutex);
        return res;
    }
    pthread_mutex_unlock(&sbigMutex);
    return res;
}

//==========================================================================

int SBIGCCD::CFWConnect()
{
    IUResetSwitch(&FilterConnectionSP);
    if (isConnected() == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "You must establish connection to CCD before connecting to filter wheel.");
        FilterConnectionSP.s   = IPS_IDLE;
        FilterConnectionS[1].s = ISS_ON;
        IDSetSwitch(&FilterConnectionSP, NULL);
        return CE_OS_ERROR;
    }

    CFWResults CFWr;
    CFWParams CFWp;
    int res       = CE_NO_ERROR;
    CFWp.cfwModel = GetCFWSelType();
    if (CFWp.cfwModel == CFWSEL_CFW10_SERIAL)
    {
        CFWp.cfwCommand = CFWC_OPEN_DEVICE;
        res             = SBIGUnivDrvCommand(CC_CFW, &CFWp, &CFWr);
        if (res != CE_NO_ERROR)
            DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_OPEN_DEVICE -> (%s)", __FUNCTION__, GetErrorString(res));
    }
    if (res == CE_NO_ERROR)
    {
        CFWp.cfwCommand = CFWC_INIT;
        for (int i = 0; i < 3; i++)
        {
            res = SBIGUnivDrvCommand(CC_CFW, &CFWp, &CFWr);
            if (res == CE_NO_ERROR)
            {
                res = CFWGotoMonitor(&CFWr);
                break;
            }
            DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_INIT -> (%s)", __FUNCTION__, GetErrorString(res));
            sleep(1);
        }
    }
    if (res == CE_NO_ERROR)
    {
        if (isSimulation())
        {
            CFWr.cfwModel    = CFWp.cfwModel;
            CFWr.cfwPosition = 1;
            CFWr.cfwResult1  = 0;
            int cfwsim[16]   = { 2, 5, 6, 8, 4, 10, 10, 8, 9, 8, 10, 5, 5, 8, 7, 8 };
            int filnum       = IUFindOnSwitchIndex(&FilterTypeSP);
            if (filnum < 0)
            {
                CFWr.cfwResult2 = 5;
            }
            else
            {
                CFWr.cfwResult2 = cfwsim[filnum];
            }
        }
        else
        {
            CFWp.cfwCommand = CFWC_GET_INFO;
            CFWp.cfwParam1  = CFWG_FIRMWARE_VERSION;
            res             = SBIGUnivDrvCommand(CC_CFW, &CFWp, &CFWr);
            if (res != CE_NO_ERROR)
                DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_GET_INFO -> (%s)", __FUNCTION__, GetErrorString(res));
        }
    }
    if (res == CE_NO_ERROR)
    {
        const char *name = "Unknown filterwheel";
        char fw[64]      = "Unknown ID";
        bool bClear      = true;
        int model        = CFWr.cfwModel;
        for (int i = 0; i < MAX_CFW_TYPES; i++)
        {
            if (model == SBIGFilterMap[i])
            {
                name   = FilterTypeS[i].label;
                bClear = false;
                break;
            }
        }
        IText *pIText = IUFindText(&FilterProdcutTP, "NAME");
        if (pIText)
        {
            IUSaveText(pIText, name);
        }
        DEBUGF(INDI::Logger::DBG_DEBUG, "CFW Product ID: %s", name);
        if (!bClear)
        {
            sprintf(fw, "%d", (int)CFWr.cfwResult1);
        }
        pIText = IUFindText(&FilterProdcutTP, "ID");
        if (pIText)
        {
            IUSaveText(pIText, fw);
        }
        DEBUGF(INDI::Logger::DBG_DEBUG, "CFW Firmware: %s", fw);
        FilterProdcutTP.s = IPS_OK;
        defineText(&FilterProdcutTP);
        FilterSlotN[0].min   = 1;
        FilterSlotN[0].max   = CFWr.cfwResult2;
        FilterSlotN[0].value = CFWr.cfwPosition;
        if (FilterSlotN[0].value < FilterSlotN[0].min)
        {
            FilterSlotN[0].value = FilterSlotN[0].min;
        }
        else if (FilterSlotN[0].value > FilterSlotN[0].max)
        {
            FilterSlotN[0].value = FilterSlotN[0].max;
        }
        DEBUGF(INDI::Logger::DBG_DEBUG, "CFW min: 1 Max: %g Current Slot: %g", FilterSlotN[0].max,
               FilterSlotN[0].value);

        defineNumber(&FilterSlotNP);
        if (FilterNameT == NULL)
            GetFilterNames();
        if (FilterNameT)
            defineText(FilterNameTP);

        DEBUG(INDI::Logger::DBG_DEBUG, "Loading FILTER_SLOT from config file...");
        loadConfig(true, "FILTER_SLOT");

        FilterConnectionSP.s = IPS_OK;
        DEBUG(INDI::Logger::DBG_SESSION, "CFW connected.");
        FilterConnectionS[0].s = ISS_ON;
        IDSetSwitch(&FilterConnectionSP, NULL);
    }
    else
    {
        FilterConnectionSP.s   = IPS_ALERT;
        FilterConnectionS[1].s = ISS_ON;
        IUResetSwitch(&FilterConnectionSP);
        FilterConnectionSP.sp[1].s = ISS_ON;
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to connect CFW");
        IDSetSwitch(&FilterConnectionSP, NULL);
    }
    return res;
}

//==========================================================================

int SBIGCCD::CFWDisconnect()
{
    CFWParams CFWp;
    CFWResults CFWr;
    CFWp.cfwModel   = GetCFWSelType();
    CFWp.cfwCommand = CFWC_CLOSE_DEVICE;
    IUResetSwitch(&FilterConnectionSP);
    int res = SBIGUnivDrvCommand(CC_CFW, &CFWp, &CFWr);
    if (res != CE_NO_ERROR)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_CLOSE_DEVICE -> (%s)", __FUNCTION__, GetErrorString(res));
        FilterConnectionS[0].s = ISS_ON;
        FilterConnectionSP.s   = IPS_ALERT;
        IDSetSwitch(&FilterConnectionSP, "Failed to disconnect CFW");
    }
    else
    {
        FilterConnectionS[1].s = ISS_ON;
        FilterConnectionSP.s   = IPS_IDLE;
        IDSetSwitch(&FilterConnectionSP, "CFW disconnected");
        deleteProperty(FilterSlotNP.name);
        deleteProperty(FilterProdcutTP.name);
        deleteProperty(FilterNameTP->name);
    }
    return res;
}

//==========================================================================

int SBIGCCD::CFWQuery(CFWResults *CFWr)
{
    CFWParams CFWp;
    CFWp.cfwModel   = GetCFWSelType();
    CFWp.cfwCommand = CFWC_QUERY;
    int res         = SBIGUnivDrvCommand(CC_CFW, &CFWp, CFWr);
    if (res != CE_NO_ERROR)
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_QUERY -> (%s)", __FUNCTION__, GetErrorString(res));
    return res;
}

//==========================================================================

int SBIGCCD::CFWGoto(CFWResults *CFWr, int position)
{
    if (CFWr == nullptr)
        return CE_NO_ERROR;

    if (isSimulation())
    {
        CFWr->cfwPosition = position;
        return CE_NO_ERROR;
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "CFW GOTO: %d", position);

    // 2014-06-16: Do we need to also checking if the position is reached here? A test will determine.
    CFWParams CFWp;
    CFWp.cfwModel   = GetCFWSelType();
    CFWp.cfwCommand = CFWC_GOTO;
    CFWp.cfwParam1  = (unsigned long)position;
    int res         = SBIGUnivDrvCommand(CC_CFW, &CFWp, CFWr);
    if (res == CE_NO_ERROR)
    {
        if (CFWp.cfwParam1 == CFWr->cfwPosition)
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "CFW Reached position %d", CFWr->cfwPosition);
            return res;
        }
        DEBUG(INDI::Logger::DBG_DEBUG, "CFW did not reach position yet, invoking CFWGotoMonitor");
        return CFWGotoMonitor(CFWr);
    }
    DEBUGF(INDI::Logger::DBG_ERROR, "%s: CC_CFW/CFWC_GOTO -> (%s)", __FUNCTION__, GetErrorString(res));
    return res;
}

//==========================================================================

int SBIGCCD::CFWGotoMonitor(CFWResults *CFWr)
{
    int res;
    if (isSimulation())
        return CE_NO_ERROR;
    do
    {
        if ((res = CFWQuery(CFWr)) != CE_NO_ERROR)
            return res;
        switch (CFWr->cfwStatus)
        {
            case CFWS_IDLE:
                DEBUG(INDI::Logger::DBG_DEBUG, "CFW Status Idle.");
                break;
            case CFWS_BUSY:
                DEBUG(INDI::Logger::DBG_DEBUG, "CFW Status Busy.");
                break;
            default:
                DEBUG(INDI::Logger::DBG_DEBUG, "CFW Status Unknown.");
                break;
        }
        sleep(1);
    } while (CFWr->cfwStatus != CFWS_IDLE);
    return res;
}

//==========================================================================

int SBIGCCD::GetCFWSelType()
{
    int filnum = IUFindOnSwitchIndex(&FilterTypeSP);
    if (filnum < 0)
    {
        return CFWSEL_UNKNOWN;
    }
    return *((uint32_t *)FilterTypeS[filnum].aux);
}
