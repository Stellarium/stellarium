/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Watchdog Client.

  The clients communicates with INDI server to put devices in a safe state for shutdown

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

#include "baseclient.h"
#include "basedevice.h"

#include <cstring>

class WatchDogClient : public INDI::BaseClient
{
  public:
    WatchDogClient();
    ~WatchDogClient();

    bool isBusy() { return isRunning; }
    bool isConnected() { return isReady; }

    void setDome(const std::string &value);
    void setMount(const std::string &value);

    bool parkDome();
    bool parkMount();
    IPState getDomeParkState();
    IPState getMountParkState();

  protected:
    virtual void newDevice(INDI::BaseDevice *dp);
    virtual void removeDevice(INDI::BaseDevice */*dp*/) {}
    virtual void newProperty(INDI::Property *property);
    virtual void removeProperty(INDI::Property */*property*/) {}
    virtual void newBLOB(IBLOB */*bp*/) {}
    virtual void newSwitch(ISwitchVectorProperty */*svp*/) {}
    virtual void newNumber(INumberVectorProperty */*nvp*/) {}
    virtual void newMessage(INDI::BaseDevice */*dp*/, int /*messageID*/) {}
    virtual void newText(ITextVectorProperty */*tvp*/) {}
    virtual void newLight(ILightVectorProperty */*lvp*/) {}
    virtual void serverConnected() {}
    virtual void serverDisconnected(int /*exit_code*/) {}

  private:
    std::string dome, mount;
    bool isReady, isRunning, domeOnline, mountOnline;

    ISwitchVectorProperty *mountParkSP, *domeParkSP;
};
