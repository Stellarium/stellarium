/*
    IEQ45 Basic Driver
    Copyright (C) 2011 Nacho Mas (mas.ignacio@gmail.com). Only litle changes
    from lx200basic made it by Jasem Mutlaq (mutlaqja@ikarustech.com)

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

class IEQ45Basic
{
  public:
    IEQ45Basic();
    ~IEQ45Basic();

    void ISGetProperties(const char *dev);
    void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n);
    void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n);
    void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n);
    void ISPoll();

    void connection_lost();
    void connection_resumed();

  private:
    enum IEQ45_STATUS
    {
        IEQ45_SLEW,
        IEQ45_SYNC,
        IEQ45_PARK
    };

    /* Switches */
    ISwitch ConnectS[2];
    ISwitch OnCoordSetS[2];
    ISwitch TrackModeS[4];
    ISwitch AbortSlewS[1];

    /* Texts */
    IText PortT[1];

    /* Numbers */
    INumber EquatorialCoordsRN[2];

    /* Switch Vectors */
    ISwitchVectorProperty ConnectSP;
    ISwitchVectorProperty OnCoordSetSP;
    ISwitchVectorProperty TrackModeSP;
    ISwitchVectorProperty AbortSlewSP;

    /* Number Vectors */
    INumberVectorProperty EquatorialCoordsRNP;

    /* Text Vectors */
    ITextVectorProperty PortTP;

    /*******************************************************/
    /* Connection Routines
        ********************************************************/
    void init_properties();
    void get_initial_data();
    void connect_telescope();
    bool is_connected();

    /*******************************************************/
    /* Misc routines
        ********************************************************/
    bool process_coords();
    int get_switch_index(ISwitchVectorProperty *sp);

    /*******************************************************/
    /* Simulation Routines
        ********************************************************/
    void enable_simulation(bool enable);

    /*******************************************************/
    /* Error handling routines
        ********************************************************/
    void slew_error(int slewCode);
    void reset_all_properties();
    void handle_error(INumberVectorProperty *nvp, int err, const char *msg);
    void correct_fault();

  protected:
    double JD; /* Julian Date */
    double lastRA;
    double lastDEC;
    bool simulation;
    bool fault;
    int fd; /* Telescope tty file descriptor */

    int currentSet;
    int lastSet;
    double targetRA, targetDEC;
};
