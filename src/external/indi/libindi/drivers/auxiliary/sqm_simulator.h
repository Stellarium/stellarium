/*******************************************************************************
  Copyright(c) 2017 Ralph Rogge. All rights reserved.

  INDI Sky Quality Meter Simulator 

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

class SQMSimulator : public INDI::DefaultDevice
{
  public:
    SQMSimulator();
    virtual ~SQMSimulator() = default;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

    virtual bool initProperties();
    virtual bool updateProperties();

  protected:
    bool Connect();
    bool Disconnect();
    const char *getDefaultName();

  private:
    bool getReading();
    bool getUnit();

    // Reading
    static const int READING_NUMBER_OF_VALUES = 5;
    INumberVectorProperty readingProperties;
    INumber readingValues[READING_NUMBER_OF_VALUES];

    // Unit
    static const int UNIT_NUMBER_OF_VALUES = 4;
    INumberVectorProperty unitProperties;
    INumber unitValues[UNIT_NUMBER_OF_VALUES];
};
