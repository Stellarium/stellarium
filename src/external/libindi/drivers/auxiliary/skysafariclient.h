/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI SkySafari Client for INDI Mounts.

  The clients communicates with INDI server to control the mount from SkySafari

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

class SkySafariClient : public INDI::BaseClient
{
  public:
    typedef enum { SLEW, TRACK, SYNC } GotoMode;

    SkySafariClient();
    ~SkySafariClient();

    bool isConnected() { return isReady; }

    void setMount(const std::string &value);

    INumberVectorProperty *getEquatorialCoords() { return eqCoordsNP; }
    bool sendEquatorialCoords();

    INumberVectorProperty *getGeographiCoords() { return geoCoordsNP; }
    bool sendGeographicCoords();

    ISwitchVectorProperty *getGotoMode() { return gotoModeSP; }
    bool sendGotoMode();

    ISwitchVectorProperty *getMotionNS() { return motionNSSP; }
    bool setMotionNS();

    ISwitchVectorProperty *getMotionWE() { return motionWESP; }
    bool setMotionWE();

    bool parkMount();
    IPState getMountParkState();

    bool setSlewRate(int slewRate);

    bool abort();

    ITextVectorProperty *getTimeUTC() { return timeUTC; }
    bool setTimeUTC();

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
    std::string mount;
    bool isReady, mountOnline;

    ISwitchVectorProperty *mountParkSP = nullptr;
    ISwitchVectorProperty *gotoModeSP  = nullptr;
    INumberVectorProperty *eqCoordsNP  = nullptr;
    INumberVectorProperty *geoCoordsNP = nullptr;
    ISwitchVectorProperty *abortSP     = nullptr;
    ISwitchVectorProperty *slewRateSP  = nullptr;
    ISwitchVectorProperty *motionNSSP  = nullptr;
    ISwitchVectorProperty *motionWESP  = nullptr;
    ITextVectorProperty *timeUTC       = nullptr;
};
