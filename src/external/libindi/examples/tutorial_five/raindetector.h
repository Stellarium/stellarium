/*
   INDI Developers Manual
   Tutorial #5 - Snooping

   Rain Detector

   Refer to README, which contains instruction on how to build this driver, and use it
   with an INDI-compatible client.

*/

/** \file raindetector.h
*   \brief Construct a rain detector device that the user may operate to raise a rain alert. This rain light property defined by this driver is \e snooped by the Dome driver
*         then takes whatever appropiate action to protect the dome.
*   \author Jasem Mutlaq
*
*   \example raindetector.h
*            The rain detector emits a signal each time it detects raid. This signal is \e snooped by the dome driver.
*/

#pragma once

#include "defaultdevice.h"

class RainDetector : public INDI::DefaultDevice
{
  public:
    RainDetector() = default;
    bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    // General device functions
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();
    bool initProperties();
    bool updateProperties();

  private:
    ILight RainL[1];
    ILightVectorProperty RainLP;

    ISwitch RainS[2];
    ISwitchVectorProperty RainSP;
};
