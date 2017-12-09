/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Watchdog driver.

  The driver expects a heartbeat from the client every X minutes. If no heartbeat
  is received, the driver executes the shutdown procedures.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.
*******************************************************************************/

#pragma once

#include "defaultdevice.h"

class WatchDogClient;

class WatchDog : public INDI::DefaultDevice
{
  public:
    typedef enum {
        WATCHDOG_IDLE,
        WATCHDOG_CLIENT_STARTED,
        WATCHDOG_MOUNT_PARKED,
        WATCHDOG_DOME_PARKED,
        WATCHDOG_COMPLETE,
        WATCHDOG_ERROR
    } ShutdownStages;
    typedef enum { PARK_MOUNT, PARK_DOME, EXECUTE_SCRIPT } ShutdownProcedure;

    WatchDog();
    virtual ~WatchDog();

    virtual void ISGetProperties(const char *dev);
    //virtual bool ISSnoopDevice (XMLEle *root);

    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    virtual bool initProperties();
    //virtual bool updateProperties();

    virtual void TimerHit();

    virtual bool Connect();
    virtual bool Disconnect();
    virtual const char *getDefaultName();

    virtual bool saveConfigItems(FILE *fp);

  private:
    void parkDome();
    void parkMount();
    void executeScript();

    INumberVectorProperty HeartBeatNP;
    INumber HeartBeatN[1];

    ITextVectorProperty SettingsTP;
    IText SettingsT[3];

    ISwitchVectorProperty ShutdownProcedureSP;
    ISwitch ShutdownProcedureS[3];

    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[2];
    enum { ACTIVE_TELESCOPE, ACTIVE_DOME };

    WatchDogClient *watchdogClient;
    int watchDogTimer;

    ShutdownStages shutdownStage;
};
