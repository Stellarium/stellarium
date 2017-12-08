/*
    INDI Driver for Optec TCF-S Focuser

    Copyright (C) 2010 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indifocuser.h"

#include <string>

#define TCFS_MAX_CMD      16
#define TCFS_ERROR_BUFFER 1024

class TCFS : public INDI::Focuser
{
  public:
    enum TCFSCommand
    {
        FMMODE, // Focuser Manual Mode
        FFMODE, // Focuser Free Mode
        FAMODE, // Focuser Auto-A Mode
        FBMODE, // Focuser Auto-B Mode
        FCENTR, // Focus Center
        FIN,    // Focuser In “nnnn”
        FOUT,   // Focuser Out “nnnn”
        FPOSRO, // Focuser Position Read Out
        FTMPRO, // Focuser Temperature Read Out
        FSLEEP, // Focuser Sleep
        FWAKUP, // Focuser Wake Up
        FHOME,  // Focuser Home Command
    };

    enum TCFSError
    {
        NO_ERROR,
        ER_1,
        ER_2,
        ER_3
    };

    TCFS();
    virtual ~TCFS() = default;

    // Standard INDI interface fucntions
    virtual bool Handshake();
    virtual bool Disconnect();
    const char *getDefaultName();
    virtual bool initProperties();
    virtual bool updateProperties();
    virtual bool ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);

  protected:
    virtual IPState MoveAbsFocuser(uint32_t targetTicks);
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks);
    virtual void TimerHit();

  private:
    bool read_tcfs(char *response, bool silent = false);
    bool dispatch_command(TCFSCommand command_type);

    ISwitchVectorProperty *FocusPowerSP { nullptr };
    ISwitchVectorProperty *FocusModeSP { nullptr };
    ISwitchVectorProperty *FocusGotoSP { nullptr };
    INumberVectorProperty *FocusTemperatureNP { nullptr };

    unsigned int simulated_position { 3000 };
    float simulated_temperature { 25.4 };

    unsigned int targetTicks { 0 };
    unsigned int targetPosition { 0 };
    bool isTCFS3 { false };
};
