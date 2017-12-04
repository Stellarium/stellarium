/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Weather Underground (TM) Weather Driver

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

#include "indiweather.h"

class WunderGround : public INDI::Weather
{
  public:
    WunderGround();
    virtual ~WunderGround();

    //  Generic indi device entries
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();

    virtual bool initProperties();
    virtual void ISGetProperties(const char *dev);
    virtual bool ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);

  protected:
    virtual IPState updateWeather();

    virtual bool saveConfigItems(FILE *fp);
    virtual bool updateLocation(double latitude, double longitude, double elevation);

  private:
    IText wunderAPIKeyT[1];
    ITextVectorProperty wunderAPIKeyTP;

    double wunderLat, wunderLong;
};
