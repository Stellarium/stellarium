
#pragma once

#include "defaultdevice.h"

//For SPC900 Led control
#include "webcam/pwc-ioctl.h"

//For serial control
#include <termios.h>

#define LX_TAB "Long Exposure"
// LX Modes
#define LXSERIAL   0
#define LXLED      1
#define LXPARALLEL 2
#define LXGPIO     3

#define LXMODENUM 2

class Lx
{
  public:
    ISwitch LxEnableS[2];
    ISwitchVectorProperty LxEnableSP;
    ISwitch LxModeS[LXMODENUM];
    ISwitchVectorProperty LxModeSP;
    IText LxPortT[1];
    ITextVectorProperty LxPortTP;
    ISwitch LxSerialOptionS[3];
    ISwitchVectorProperty LxSerialOptionSP;
    ISwitch LxParallelOptionS[9];
    ISwitchVectorProperty LxParallelOptionSP;
    IText LxStartStopCmdT[2];
    ITextVectorProperty LxStartStopCmdTP;
    ISwitch LxLogicalLevelS[2];
    ISwitchVectorProperty LxLogicalLevelSP;
    ISwitch LxSerialSpeedS[9];
    ISwitchVectorProperty LxSerialSpeedSP;
    ISwitch LxSerialSizeS[4];
    ISwitchVectorProperty LxSerialSizeSP;
    ISwitch LxSerialParityS[3];
    ISwitchVectorProperty LxSerialParitySP;
    ISwitch LxSerialStopS[2];
    ISwitchVectorProperty LxSerialStopSP;
    ISwitch LxSerialAddeolS[4];
    ISwitchVectorProperty LxSerialAddeolSP;

    bool isEnabled();
    void setCamerafd(int fd);
    bool initProperties(INDI::DefaultDevice *device);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    bool updateProperties();
    bool startLx();
    int stopLx();
    unsigned int getLxmode();

  private:
    INDI::DefaultDevice *dev;
    const char *device_name;
    int camerafd;

    // Serial
    int serialfd;
    struct termios oldterminfo;
    void closeserial(int fd);
    int openserial(char *devicename);
    int setRTS(int fd, int level);
    int setDTR(int fd, int level);
    bool startLxSerial();
    int stopLxSerial();
    void getSerialOptions(unsigned int *speed, unsigned int *wordsize, unsigned int *parity, unsigned int *stops);
    const char *getSerialEOL();
    INDI::Property *findbyLabel(INDI::DefaultDevice *dev, char *label);
    // PWC Cameras
    ISwitchVectorProperty *FlashStrobeSP;
    ISwitchVectorProperty *FlashStrobeStopSP;
    enum pwcledmethod
    {
        PWCIOCTL,
        FLASHLED
    };
    char ledmethod;
    struct pwc_probe probe;
    bool checkPWC();
    void pwcsetLed(int on, int off);
    void pwcsetflashon();
    void pwcsetflashoff();
    bool startLxPWC();
    int stopLxPWC();
};
