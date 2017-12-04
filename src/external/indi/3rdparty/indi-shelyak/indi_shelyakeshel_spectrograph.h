#ifndef SHELYAKESHEL_SPECTROGRAPH_H
#define SHELYAKESHEL_SPECTROGRAPH_H

#include <map>

#include "defaultdevice.h"

std::map<ISState, char> COMMANDS       = { { ISS_ON, 0x53 }, { ISS_OFF, 0x43 } };
std::map<std::string, char> PARAMETERS = { { "MIRROR", 0x31 },
                                           { "LED", 0x32 },
                                           { "THAR", 0x33 },
                                           { "TUNGSTEN", 0x34 } };

class ShelyakEshel : public INDI::DefaultDevice
{
  public:
    ShelyakEshel();
    ~ShelyakEshel();

    void ISGetProperties(const char *dev);
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

  protected:
    const char *getDefaultName();

    bool initProperties();
    bool updateProperties();

    bool Connect();
    bool Disconnect();

  private:
    int PortFD; // file descriptor for serial port

    // Main Control
    ISwitchVectorProperty LampSP;
    ISwitch LampS[3];
    ISwitchVectorProperty MirrorSP;
    ISwitch MirrorS[2];

    // Options
    ITextVectorProperty PortTP;
    IText PortT[1];

    // Spectrograph Settings
    INumberVectorProperty SettingsNP;
    INumber SettingsN[5];

    bool calibrationUnitCommand(char command, char parameter);
};

#endif // SHELYAKESHEL_SPECTROGRAPH_H
