/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI GPS NMEA Driver

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

#include <indigps.h>

class GPSNMEA : public INDI::GPS
{
  public:
    GPSNMEA();
    virtual ~GPSNMEA() = default;

    IText GPSstatusT[1];
    ITextVectorProperty GPSstatusTP;

    static void* parseNMEAHelper(void *);

  protected:    
    //  Generic indi device entries
    virtual const char *getDefaultName() override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual IPState updateGPS() override;

private:
    Connection::TCP *tcpConnection { nullptr };
    bool isNMEA();
    void parseNEMA();

    int PortFD { -1 };
    uint8_t timeoutCounter=0;
    bool locationPending = true, timePending=true;

    pthread_mutex_t lock;
    pthread_t nmeaThread;
};
