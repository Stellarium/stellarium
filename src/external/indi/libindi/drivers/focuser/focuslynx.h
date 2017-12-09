/*
  FocusLynx/FocusBoss II INDI driver
  Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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
*/

#pragma once

#include "focuslynxbase.h"

#define POLLMS 1000
#define FOCUSNAMEF1 "FocusLynx F1"
#define FOCUSNAMEF2 "FocusLynx F2"

class FocusLynxF1 : public FocusLynxBase
{
  public:
    explicit FocusLynxF1(const char *target);
    ~FocusLynxF1();

    const char *getDefaultName() override;
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual bool updateProperties() override;
    virtual bool initProperties() override;
//    virtual void ISGetProperties(const char *dev) override;
    int getPortFD();

    virtual void setSimulation(bool enable);
    virtual void setDebug(bool enable);
  private:
    // Get functions
    bool getHubConfig();

    // HUB Main Parameter
    IText HubT[2];
    ITextVectorProperty HubTP;

    // Network Wired Info
    IText WiredT[2];
    ITextVectorProperty WiredTP;

    //Network WIFI Info
    IText WifiT[9];
    ITextVectorProperty WifiTP;
};

class FocusLynxF2 : public FocusLynxBase
{
  public:
    explicit FocusLynxF2(const char *target);
    ~FocusLynxF2();

    const char *getDefaultName() override;
    virtual bool Connect() override;
    virtual bool Disconnect() override;
    virtual bool RemoteDisconnect();
    virtual bool initProperties() override;
    virtual void simulationTriggered(bool enable) override;
    virtual void debugTriggered(bool enable) override;
};

