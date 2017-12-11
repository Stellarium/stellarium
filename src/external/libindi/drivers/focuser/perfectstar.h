/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

 PerfectStar Focuser

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
#include "hidapi.h"

class PerfectStar : public INDI::Focuser
{
  public:
    // Perfect Star (PS) status
    typedef enum { PS_NOOP, PS_IN, PS_OUT, PS_GOTO, PS_SETPOS, PS_LOCKED, PS_HALT = 0xFF } PS_STATUS;

    PerfectStar();
    virtual ~PerfectStar() = default;

    virtual bool ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);

    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool saveConfigItems(FILE *fp);

    bool Connect();
    bool Disconnect();

    void TimerHit();

    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual bool AbortFocuser();

  private:
    bool setPosition(uint32_t ticks);
    bool getPosition(uint32_t *ticks);

    bool setStatus(PS_STATUS targetStatus);
    bool getStatus(PS_STATUS *currentStatus);

    bool sync(uint32_t ticks);

    hid_device *handle { nullptr };
    PS_STATUS status { PS_NOOP };
    bool sim { false };
    uint32_t simPosition { 0 };
    uint32_t targetPosition { 0 };

    // Max position in ticks
    INumber MaxPositionN[1];
    INumberVectorProperty MaxPositionNP;

    // Sync to a particular position
    INumber SyncN[1];
    INumberVectorProperty SyncNP;
};
