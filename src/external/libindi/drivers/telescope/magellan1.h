/*
    MAGELLAN Generic
    Copyright (C) 2011 Onno Hommes  (ohommes@alumni.cmu.edu)

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

#include "indicom.h"
#include "indidevapi.h"

#define POLLMS 1000 /* poll period, ms */

/*
   The device name below eventhough we have a Magellan I
   should remain set to a KStars registered telescope so
   It allows the service to be stopped
*/
#define mydev "Magellan I" /* The device name */

class Magellan1
{
  public:
    Magellan1();
    virtual ~Magellan1();

    virtual void ISGetProperties(const char *dev);
    virtual void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    virtual void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual void ISSnoopDevice(XMLEle *root);
    virtual void ISPoll();
    virtual void getBasicData();

    void handleError(ISwitchVectorProperty *svp, int err, const char *msg);
    void handleError(INumberVectorProperty *nvp, int err, const char *msg);
    void handleError(ITextVectorProperty *tvp, int err, const char *msg);
    bool isTelescopeOn();
    void connectTelescope();
    void setCurrentDeviceName(const char *devName);
    void correctFault();

    int fd;

  protected:
    int timeFormat;
    int currentSiteNum;
    int trackingMode;

    double JD;
    double lastRA;
    double lastDEC;
    bool fault;
    bool simulation;
    char thisDevice[64];
    int currentSet;
    int lastSet;
    double targetRA, targetDEC;
};
