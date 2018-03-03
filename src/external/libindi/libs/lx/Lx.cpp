
#include "Lx.h"

#include "indicom.h"

#include <fcntl.h>
#include <cstring>
#include <unistd.h>
#include <sys/ioctl.h>

// from indicom.cpp for tty_connect
#define PARITY_NONE 0
#define PARITY_EVEN 1
#define PARITY_ODD  2

void Lx::setCamerafd(int fd)
{
    camerafd = fd;
}

bool Lx::isEnabled()
{
    return (LxEnableS[1].s == ISS_ON);
}

bool Lx::initProperties(INDI::DefaultDevice *device)
{
    //IDLog("Initializing Long Exposure Properties\n");
    dev         = device;
    device_name = dev->getDeviceName();
    IUFillSwitch(&LxEnableS[0], "Disable", "", ISS_ON);
    IUFillSwitch(&LxEnableS[1], "Enable", "", ISS_OFF);
    IUFillSwitchVector(&LxEnableSP, LxEnableS, NARRAY(LxEnableS), device_name, "Activate", "", LX_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxModeS[LXSERIAL], "Serial", "", ISS_ON);
    //IUFillSwitch(&LxModeS[LXPARALLEL], "Parallel", "", ISS_OFF);
    IUFillSwitch(&LxModeS[LXLED], "SPC900 LED", "", ISS_OFF);
    //IUFillSwitch(&LxModeS[LXGPIO], "GPIO (Arm/RPI)", "", ISS_OFF);
    //  IUFillSwitch(&LxModeS[4], "IndiDuino Switcher", "", ISS_OFF); // Snooping is not enough
    IUFillSwitchVector(&LxModeSP, LxModeS, NARRAY(LxModeS), device_name, "LX Mode", "", LX_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);
    IUFillText(&LxPortT[0], "Port", "", "/dev/ttyUSB0");
    IUFillTextVector(&LxPortTP, LxPortT, NARRAY(LxPortT), device_name, "Lx port", "", LX_TAB, IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialOptionS[0], "Use DTR (pin 4)", "", ISS_OFF);
    IUFillSwitch(&LxSerialOptionS[1], "Use RTS (pin 7)", "", ISS_ON);
    IUFillSwitch(&LxSerialOptionS[2], "Use Serial command", "", ISS_OFF);
    IUFillSwitchVector(&LxSerialOptionSP, LxSerialOptionS, NARRAY(LxSerialOptionS), device_name, "Serial Options", "",
                       LX_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxParallelOptionS[0], "Use Data 0 (pin 2)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[1], "Use Data 1 (pin 3)", "", ISS_ON); // Steve's Chambers Schematics
    IUFillSwitch(&LxParallelOptionS[2], "Use Data 2 (pin 4)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[3], "Use Data 3 (pin 5)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[4], "Use Data 4 (pin 6)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[5], "Use Data 5 (pin 7)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[6], "Use Data 6 (pin 8)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[7], "Use Data 7 (pin 9)", "", ISS_OFF);
    IUFillSwitch(&LxParallelOptionS[8], "Use Parallel command", "", ISS_OFF);
    IUFillSwitchVector(&LxParallelOptionSP, LxParallelOptionS, NARRAY(LxParallelOptionS), device_name,
                       "Parallel Options", "", LX_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillText(&LxStartStopCmdT[0], "Start command", "", ":O1");
    IUFillText(&LxStartStopCmdT[1], "Stop command", "", ":O0");
    IUFillTextVector(&LxStartStopCmdTP, LxStartStopCmdT, NARRAY(LxStartStopCmdT), device_name, "Start/Stop commands",
                     "", LX_TAB, IP_RW, 0, IPS_IDLE);
    IUFillSwitch(&LxLogicalLevelS[0], "Low to High", "", ISS_ON);
    IUFillSwitch(&LxLogicalLevelS[1], "High to Low", "", ISS_OFF);
    IUFillSwitchVector(&LxLogicalLevelSP, LxLogicalLevelS, NARRAY(LxLogicalLevelS), device_name, "Start Transition", "",
                       LX_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialSpeedS[0], "1200", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[1], "2400", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[2], "4800", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[3], "9600", "", ISS_ON);
    IUFillSwitch(&LxSerialSpeedS[4], "19200", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[5], "38400", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[6], "57600", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[7], "115200", "", ISS_OFF);
    IUFillSwitch(&LxSerialSpeedS[8], "230400", "", ISS_OFF);
    IUFillSwitchVector(&LxSerialSpeedSP, LxSerialSpeedS, NARRAY(LxSerialSpeedS), device_name, "Serial speed", "",
                       LX_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialSizeS[0], "5", "", ISS_OFF);
    IUFillSwitch(&LxSerialSizeS[1], "6", "", ISS_OFF);
    IUFillSwitch(&LxSerialSizeS[2], "7", "", ISS_OFF);
    IUFillSwitch(&LxSerialSizeS[3], "8", "", ISS_ON);
    IUFillSwitchVector(&LxSerialSizeSP, LxSerialSizeS, NARRAY(LxSerialSizeS), device_name, "Serial size", "", LX_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialParityS[0], "None", "", ISS_ON);
    IUFillSwitch(&LxSerialParityS[1], "Even", "", ISS_OFF);
    IUFillSwitch(&LxSerialParityS[2], "Odd", "", ISS_OFF);
    IUFillSwitchVector(&LxSerialParitySP, LxSerialParityS, NARRAY(LxSerialParityS), device_name, "Serial parity", "",
                       LX_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialStopS[0], "1", "", ISS_ON);
    IUFillSwitch(&LxSerialStopS[1], "2", "", ISS_OFF);
    IUFillSwitchVector(&LxSerialStopSP, LxSerialStopS, NARRAY(LxSerialStopS), device_name, "Serial stop", "", LX_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    IUFillSwitch(&LxSerialAddeolS[0], "None", "", ISS_OFF);
    IUFillSwitch(&LxSerialAddeolS[1], "CR (OxD, \\r)", "", ISS_ON);
    IUFillSwitch(&LxSerialAddeolS[2], "LF (0xA, \\n)", "", ISS_OFF);
    IUFillSwitch(&LxSerialAddeolS[3], "CR+LF", "", ISS_OFF);
    IUFillSwitchVector(&LxSerialAddeolSP, LxSerialAddeolS, NARRAY(LxSerialAddeolS), device_name, "Add EOL", "", LX_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    FlashStrobeSP     = nullptr;
    FlashStrobeStopSP = nullptr;
    ledmethod         = PWCIOCTL;
    return true;
}

bool Lx::updateProperties()
{
    if (dev->isConnected())
    {
        INDI::Property *pfound;
        dev->defineSwitch(&LxEnableSP);
        dev->defineSwitch(&LxModeSP);
        dev->defineText(&LxPortTP);
        dev->defineSwitch(&LxSerialOptionSP);
        //dev->defineSwitch(&LxParallelOptionSP);
        dev->defineText(&LxStartStopCmdTP);
        dev->defineSwitch(&LxLogicalLevelSP);
        dev->defineSwitch(&LxSerialSpeedSP);
        dev->defineSwitch(&LxSerialSizeSP);
        dev->defineSwitch(&LxSerialParitySP);
        dev->defineSwitch(&LxSerialStopSP);
        dev->defineSwitch(&LxSerialAddeolSP);
        pfound = findbyLabel(dev, (char *)"Strobe");
        if (pfound)
        {
            FlashStrobeSP     = dev->getSwitch(pfound->getName());
            pfound            = findbyLabel(dev, (char *)"Stop Strobe");
            FlashStrobeStopSP = dev->getSwitch(pfound->getName());
        }
    }
    else
    {
        dev->deleteProperty(LxEnableSP.name);
        dev->deleteProperty(LxModeSP.name);
        dev->deleteProperty(LxPortTP.name);
        dev->deleteProperty(LxSerialOptionSP.name);
        //dev->deleteProperty(LxParallelOptionSP.name);
        dev->deleteProperty(LxStartStopCmdTP.name);
        dev->deleteProperty(LxLogicalLevelSP.name);
        dev->deleteProperty(LxSerialSpeedSP.name);
        dev->deleteProperty(LxSerialSizeSP.name);
        dev->deleteProperty(LxSerialParitySP.name);
        dev->deleteProperty(LxSerialStopSP.name);
        dev->deleteProperty(LxSerialAddeolSP.name);
        FlashStrobeSP     = nullptr;
        FlashStrobeStopSP = nullptr;
    }
    return true;
}

bool Lx::ISNewSwitch(const char *devname, const char *name, ISState *states, char *names[], int n)
{
    /* ignore if not ours */
    if (devname && strcmp(device_name, devname))
        return true;

    if (!strcmp(name, LxEnableSP.name))
    {
        IUResetSwitch(&LxEnableSP);
        IUUpdateSwitch(&LxEnableSP, states, names, n);
        LxEnableSP.s = IPS_OK;

        IDSetSwitch(&LxEnableSP, "%s long exposure on device %s", (LxEnableS[0].s == ISS_ON ? "Disabling" : "Enabling"),
                    device_name);
        return true;
    }

    if (!strcmp(name, LxModeSP.name))
    {
        unsigned int index, oldindex;
        oldindex = IUFindOnSwitchIndex(&LxModeSP);
        IUResetSwitch(&LxModeSP);
        IUUpdateSwitch(&LxModeSP, states, names, n);
        LxModeSP.s = IPS_OK;
        index      = IUFindOnSwitchIndex(&LxModeSP);
        if (index == LXLED)
            if (!checkPWC())
            {
                IUResetSwitch(&LxModeSP);
                LxModeSP.s          = IPS_ALERT;
                LxModeS[oldindex].s = ISS_ON;
                IDSetSwitch(&LxModeSP, "Can not set Lx Mode to %s", LxModeS[index].name);
                return false;
            }
        IDSetSwitch(&LxModeSP, "Setting Lx Mode to %s", LxModeS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialOptionSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialOptionSP);
        IUUpdateSwitch(&LxSerialOptionSP, states, names, n);
        LxSerialOptionSP.s = IPS_OK;
        index              = IUFindOnSwitchIndex(&LxSerialOptionSP);
        IDSetSwitch(&LxSerialOptionSP, "Setting Lx Serial option: %s", LxSerialOptionS[index].name);
        return true;
    }

    if (!strcmp(name, LxParallelOptionSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxParallelOptionSP);
        IUUpdateSwitch(&LxParallelOptionSP, states, names, n);
        LxParallelOptionSP.s = IPS_OK;
        index                = IUFindOnSwitchIndex(&LxParallelOptionSP);
        IDSetSwitch(&LxParallelOptionSP, "Setting Lx Parallel option: %s", LxParallelOptionS[index].name);
        return true;
    }

    if (!strcmp(name, LxLogicalLevelSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxLogicalLevelSP);
        IUUpdateSwitch(&LxLogicalLevelSP, states, names, n);
        LxLogicalLevelSP.s = IPS_OK;
        index              = IUFindOnSwitchIndex(&LxLogicalLevelSP);
        IDSetSwitch(&LxLogicalLevelSP, "Setting Lx logical levels for start transition: %s",
                    LxLogicalLevelS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialSpeedSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialSpeedSP);
        IUUpdateSwitch(&LxSerialSpeedSP, states, names, n);
        LxSerialSpeedSP.s = IPS_OK;
        index             = IUFindOnSwitchIndex(&LxSerialSpeedSP);
        IDSetSwitch(&LxSerialSpeedSP, "Setting Lx serial speed: %s", LxSerialSpeedS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialSizeSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialSizeSP);
        IUUpdateSwitch(&LxSerialSizeSP, states, names, n);
        LxSerialSizeSP.s = IPS_OK;
        index            = IUFindOnSwitchIndex(&LxSerialSizeSP);
        IDSetSwitch(&LxSerialSizeSP, "Setting Lx serial word size: %s", LxSerialSizeS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialParitySP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialParitySP);
        IUUpdateSwitch(&LxSerialParitySP, states, names, n);
        LxSerialParitySP.s = IPS_OK;
        index              = IUFindOnSwitchIndex(&LxSerialParitySP);
        IDSetSwitch(&LxSerialParitySP, "Setting Lx serial parity: %s", LxSerialParityS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialStopSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialStopSP);
        IUUpdateSwitch(&LxSerialStopSP, states, names, n);
        LxSerialStopSP.s = IPS_OK;
        index            = IUFindOnSwitchIndex(&LxSerialStopSP);
        IDSetSwitch(&LxSerialStopSP, "Setting Lx serial stop bits: %s", LxSerialStopS[index].name);
        return true;
    }

    if (!strcmp(name, LxSerialAddeolSP.name))
    {
        unsigned int index;
        IUResetSwitch(&LxSerialAddeolSP);
        IUUpdateSwitch(&LxSerialAddeolSP, states, names, n);
        LxSerialAddeolSP.s = IPS_OK;
        index              = IUFindOnSwitchIndex(&LxSerialAddeolSP);
        IDSetSwitch(&LxSerialAddeolSP, "Setting Lx End of Line: %s", LxSerialAddeolS[index].name);
        return true;
    }

    return true; // not ours, don't care
}

bool Lx::ISNewText(const char *devname, const char *name, char *texts[], char *names[], int n)
{
    IText *tp;
    /* ignore if not ours */
    if (devname && strcmp(device_name, devname))
        return true;

    if (!strcmp(name, LxPortTP.name))
    {
        LxPortTP.s = IPS_OK;
        tp         = IUFindText(&LxPortTP, names[0]);
        if (!tp)
            return false;

        IUSaveText(tp, texts[0]);
        IDSetText(&LxPortTP, "Setting Lx port to %s", tp->text);
        return true;
    }

    if (!strcmp(name, LxStartStopCmdTP.name))
    {
        LxStartStopCmdTP.s = IPS_OK;
        for (int i = 0; i < n; i++)
        {
            tp = IUFindText(&LxStartStopCmdTP, names[i]);
            if (!tp)
                return false;
            IUSaveText(tp, texts[i]);
        }
        IDSetText(&LxStartStopCmdTP, "Setting Lx Start/stop commands");
        return true;
    }

    return true; // not ours, don't care
}

unsigned int Lx::getLxmode()
{
    return IUFindOnSwitchIndex(&LxModeSP);
}

bool Lx::startLx()
{
    unsigned int index;
    IDMessage(device_name, "Starting Long Exposure");
    index = IUFindOnSwitchIndex(&LxModeSP);
    switch (index)
    {
        case LXSERIAL:
            return startLxSerial();
        case LXLED:
            return startLxPWC();
        default:
            return false;
    }
    return false;
}

int Lx::stopLx()
{
    unsigned int index = 0;

    IDMessage(device_name, "Stopping Long Exposure");
    index = IUFindOnSwitchIndex(&LxModeSP);
    switch (index)
    {
        case LXSERIAL:
            return stopLxSerial();
        case LXLED:
            return stopLxPWC();
        default:
            return -1;
    }

    return 0;
}

// Serial Stuff

void Lx::closeserial(int fd)
{
    tcsetattr(fd, TCSANOW, &oldterminfo);
    if (close(fd) < 0)
        perror("closeserial()");
}

int Lx::openserial(char *devicename)
{
    int fd;
    struct termios attr;

    if ((fd = open(devicename, O_RDWR)) == -1)
    {
        IDLog("openserial(): open()");
        return -1;
    }
    if (tcgetattr(fd, &oldterminfo) == -1)
    {
        IDLog("openserial(): tcgetattr()");
        return -1;
    }
    attr = oldterminfo;
    //attr.c_cflag |= CRTSCTS | CLOCAL;
    attr.c_cflag |= CLOCAL;
    attr.c_oflag = 0;
    if (tcflush(fd, TCIOFLUSH) == -1)
    {
        IDLog("openserial(): tcflush()");
        return -1;
    }
    if (tcsetattr(fd, TCSANOW, &attr) == -1)
    {
        IDLog("initserial(): tcsetattr()");
        return -1;
    }
    return fd;
}

int Lx::setRTS(int fd, int level)
{
//    int status;
    int mcr = 0;
    // does not work for RTS
    //if (ioctl(fd, TIOCMGET, &status) == -1) {
    //    IDLog("setRTS(): TIOCMGET");
    //    return 0;
    //}
    //if (level)
    //    status |= TIOCM_RTS;
    //else
    //    status &= ~TIOCM_RTS;
    //if (ioctl(fd, TIOCMSET, &status) == -1) {
    //    IDLog("setRTS(): TIOCMSET");
    //    return 0;
    //}
    mcr = TIOCM_RTS;
    if (level)
    {
        if (ioctl(fd, TIOCMBIS, &mcr) == -1)
        {
            IDLog("setRTS(): TIOCMBIS");
            return 0;
        }
    }
    else
    {
        if (ioctl(fd, TIOCMBIC, &mcr) == -1)
        {
            IDLog("setRTS(): TIOCMBIC");
            return 0;
        }
    }
    return 1;
}

int Lx::setDTR(int fd, int level)
{
    int status;

    if (ioctl(fd, TIOCMGET, &status) == -1)
    {
        IDLog("setDTR(): TIOCMGET");
        return 0;
    }
    if (level)
        status |= TIOCM_DTR;
    else
        status &= ~TIOCM_DTR;
    if (ioctl(fd, TIOCMSET, &status) == -1)
    {
        IDLog("setDTR(): TIOCMSET");
        return 0;
    }
    return 1;
}

void Lx::getSerialOptions(unsigned int *speed, unsigned int *wordsize, unsigned int *parity, unsigned int *stops)
{
    unsigned int index;
    unsigned int speedopts[]  = { 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400 };
    unsigned int sizeopts[]   = { 5, 6, 7, 8 };
    unsigned int parityopts[] = { PARITY_NONE, PARITY_EVEN, PARITY_ODD };
    unsigned int stopopts[]   = { 1, 2 };
    index                     = IUFindOnSwitchIndex(&LxSerialSpeedSP);
    *speed                    = speedopts[index];
    index                     = IUFindOnSwitchIndex(&LxSerialSizeSP);
    *wordsize                 = sizeopts[index];
    index                     = IUFindOnSwitchIndex(&LxSerialParitySP);
    *parity                   = parityopts[index];
    index                     = IUFindOnSwitchIndex(&LxSerialStopSP);
    *stops                    = stopopts[index];
}

const char *Lx::getSerialEOL()
{
    unsigned int index;
    index = IUFindOnSwitchIndex(&LxSerialAddeolSP);
    switch (index)
    {
        case 0:
            return "";
        case 1:
            return "\r";
        case 2:
            return "\n";
        case 3:
            return "\r\n";
    }
    return nullptr;
}

bool Lx::startLxSerial()
{
    unsigned int speed = 0, wordsize = 0, parity = 0, stops = 0;
    const char *eol = nullptr;
    unsigned int index = IUFindOnSwitchIndex(&LxSerialOptionSP);
    int ret = 0;

    switch (index)
    {
        case 0:
            serialfd = openserial(LxPortT[0].text);
            if (serialfd < 0)
                return false;
            if (LxLogicalLevelS[0].s == ISS_ON)
                setDTR(serialfd, 1);
            else
                setDTR(serialfd, 0);
            break;
        case 1:
            serialfd = openserial(LxPortT[0].text);
            if (serialfd < 0)
                return false;
            if (LxLogicalLevelS[0].s == ISS_ON)
                setRTS(serialfd, 1);
            else
                setRTS(serialfd, 0);
            break;
        case 2:
            getSerialOptions(&speed, &wordsize, &parity, &stops);
            eol = getSerialEOL();
            tty_connect(LxPortT[0].text, speed, wordsize, parity, stops, &serialfd);
            if (serialfd < 0)
                return false;

            ret = write(serialfd, LxStartStopCmdT[0].text, strlen(LxStartStopCmdT[0].text));
            ret = write(serialfd, eol, strlen(eol));
            break;
    }
    return true;
}

int Lx::stopLxSerial()
{
    int ret = 0;
    const char *eol = nullptr;
    unsigned int index = IUFindOnSwitchIndex(&LxSerialOptionSP);

    switch (index)
    {
        case 0:
            if (LxLogicalLevelS[0].s == ISS_ON)
                setDTR(serialfd, 0);
            else
                setDTR(serialfd, 1);
            break;
        case 1:
            if (LxLogicalLevelS[0].s == ISS_ON)
                setRTS(serialfd, 0);
            else
                setRTS(serialfd, 1);
            break;
        case 2:
            ret = write(serialfd, LxStartStopCmdT[1].text, strlen(LxStartStopCmdT[1].text));
            eol = getSerialEOL();
            ret = write(serialfd, eol, strlen(eol));
            break;
    }
    close(serialfd);
    return 0;
}

INDI::Property *Lx::findbyLabel(INDI::DefaultDevice *dev, char *label)
{
    std::vector<INDI::Property *> *allprops = dev->getProperties();

    for (std::vector<INDI::Property *>::iterator it = allprops->begin(); it != allprops->end(); ++it)
    {
        if (!(strcmp((*it)->getLabel(), label)))
            return *it;
    }
    return nullptr;
}

// PWC Stuff
bool Lx::checkPWC()
{
    if (FlashStrobeSP && FlashStrobeStopSP)
    {
        IDMessage(device_name, "Using Flash control for led Lx Mode");
        ledmethod = FLASHLED;
        return true;
    }
    if (ioctl(camerafd, VIDIOCPWCPROBE, &probe) != 0)
    {
        IDMessage(device_name, "ERROR: device does not support PWC ioctl");
        return false;
    }
    if (probe.type < 730)
    {
        IDMessage(device_name, "ERROR: camera type %d does not support led control", probe.type);
        return false;
    }
    IDMessage(device_name, "Using PWC ioctl for led Lx Mode");
    return true;
}

void Lx::pwcsetLed(int on, int off)
{
    struct pwc_leds leds;
    leds.led_on  = on;
    leds.led_off = off;
    if (ioctl(camerafd, VIDIOCPWCSLED, &leds))
    {
        IDLog("ioctl: can't set Led.\n");
    }
}

void Lx::pwcsetflashon()
{
    ISState states[2]    = { ISS_ON, ISS_OFF };
    const char *names[2] = { FlashStrobeSP->sp[0].name, FlashStrobeStopSP->sp[0].name };
    dev->ISNewSwitch(device_name, FlashStrobeSP->name, &(states[0]), (char **)names, 1);
    //dev->ISNewSwitch(device_name, FlashStrobeStopSP->name, &(states[1]), (char **)(names + 1), 1);
    FlashStrobeSP->s = IPS_OK;
    IDSetSwitch(FlashStrobeSP, nullptr);
    FlashStrobeStopSP->s = IPS_IDLE;
    IDSetSwitch(FlashStrobeStopSP, nullptr);
}

void Lx::pwcsetflashoff()
{
    ISState states[2]    = { ISS_OFF, ISS_ON };
    const char *names[2] = { FlashStrobeSP->sp[0].name, FlashStrobeStopSP->sp[0].name };
    //dev->ISNewSwitch(device_name, FlashStrobeSP->name, &(states[0]), (char **)names, 1);
    dev->ISNewSwitch(device_name, FlashStrobeStopSP->name, &(states[1]), (char **)(names + 1), 1);
    FlashStrobeStopSP->s = IPS_OK;
    IDSetSwitch(FlashStrobeStopSP, nullptr);
    FlashStrobeSP->s = IPS_IDLE;
    IDSetSwitch(FlashStrobeSP, nullptr);
}

bool Lx::startLxPWC()
{
    switch (ledmethod)
    {
        case PWCIOCTL:
            if (LxLogicalLevelS[0].s == ISS_ON)
                pwcsetLed(25500, 0);
            else
                pwcsetLed(0, 25500);
            return true;
        case FLASHLED:
            if (LxLogicalLevelS[0].s == ISS_ON)
                pwcsetflashon();
            else
                pwcsetflashoff();
            return true;
    }

    return false;
}

int Lx::stopLxPWC()
{
    switch (ledmethod)
    {
        case PWCIOCTL:
            if (LxLogicalLevelS[0].s == ISS_ON)
                pwcsetLed(0, 25500);
            else
                pwcsetLed(25500, 0);
            return 0;
        case FLASHLED:
            if (LxLogicalLevelS[0].s == ISS_ON)
                pwcsetflashoff();
            else
                pwcsetflashon();
            return 0;
    }

    return -1;
}
