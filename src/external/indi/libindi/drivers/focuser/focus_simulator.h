/*******************************************************************************
  Copyright(c) 2012 Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#pragma once

#include "indifocuser.h"

class FocusSim : public INDI::Focuser
{
  public:
    FocusSim();
    virtual ~FocusSim() = default;

    const char *getDefaultName();

    bool initProperties();
    void ISGetProperties(const char *dev);
    bool updateProperties();

    bool Connect();
    bool Disconnect();

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

    virtual IPState MoveFocuser(FocusDirection dir, int speed, uint16_t duration);
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool SetFocuserSpeed(int speed);

  private:
    double internalTicks { 0 };
    double initTicks { 0 };

    // Seeing in arcseconds
    INumberVectorProperty SeeingNP;
    INumber SeeingN[1];

    // FWHM to be used by CCD driver to draw 'fuzzy' stars
    INumberVectorProperty FWHMNP;
    INumber FWHMN[1];

    // Current mode of Focus simulator for testing purposes
    enum
    {
        MODE_ALL,
        MODE_ABSOLUTE,
        MODE_RELATIVE,
        MODE_TIMER,
        MODE_COUNT
    };
    ISwitchVectorProperty ModeSP;
    ISwitch ModeS[MODE_COUNT];
};
