/*
    LX200 Basic Driver
    Copyright (C) 2005 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "inditelescope.h"

class LX200Basic : public INDI::Telescope
{
  public:
    LX200Basic();
    ~LX200Basic() = default;

    virtual const char *getDefaultName() override;
    virtual bool Handshake() override;
    virtual bool ReadScopeStatus() override;
    virtual void ISGetProperties(const char *dev) override;
    virtual bool initProperties() override;
    virtual bool updateProperties() override;
    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n) override;

  protected:
    virtual bool Abort() override;
    virtual bool Goto(double, double) override;
    virtual bool Sync(double ra, double dec) override;

    virtual void debugTriggered(bool enable) override;

    void getBasicData();

  private:
    bool isSlewComplete();
    void slewError(int slewCode);
    void mountSim();

    INumber SlewAccuracyN[2];
    INumberVectorProperty SlewAccuracyNP;

    double targetRA, targetDEC;
    double currentRA, currentDEC;
    unsigned int DBG_SCOPE;
};
