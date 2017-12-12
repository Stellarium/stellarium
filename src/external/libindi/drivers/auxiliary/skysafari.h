/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI SkySafar Middleware Driver.

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

class SkySafariClient;

class SkySafari : public INDI::DefaultDevice
{
  public:
    SkySafari();
    virtual ~SkySafari();

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
    void processCommand(std::string cmd);

    bool startServer();
    bool stopServer();

    bool sendSkySafari(const char *message);

    void sendGeographicCoords();
    void sendUTCtimedate();

    template <typename Out>
    void split(const std::string &s, char delim, Out result);
    std::vector<std::string> split(const std::string &s, char delim);

    // Settings
    ITextVectorProperty SettingsTP;
    IText SettingsT[3];
    enum
    {
        INDISERVER_HOST,
        INDISERVER_PORT,
        SKYSAFARI_PORT
    };

    // Active Devices
    ITextVectorProperty ActiveDeviceTP;
    IText ActiveDeviceT[1];
    enum
    {
        ACTIVE_TELESCOPE
    };

    // Server Control
    ISwitchVectorProperty ServerControlSP;
    ISwitch ServerControlS[2];
    enum
    {
        SERVER_ENABLE,
        SERVER_DISABLE
    };

    // Our client
    SkySafariClient *skySafariClient = nullptr;

    int lsocket = -1, clientFD = -1;

    bool isSkySafariConnected = false, haveLatitude = false, haveLongitude = false;
    bool haveUTCoffset = false, haveUTCtime = false, haveUTCdate = false;

    double siteLatitude = 0, siteLongitude = 0;
    double RA = 0, DE = 0;
    double timeUTCOffset = 0;
    int timeYear = 0, timeMonth = 0, timeDay = 0, timeHour = 0, timeMin = 0, timeSec = 0;
};
