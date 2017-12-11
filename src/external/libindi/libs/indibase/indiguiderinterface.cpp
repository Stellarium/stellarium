/*
    Guider Interface
    Copyright (C) 2011 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "indiguiderinterface.h"

#include <cstring>

namespace INDI
{

GuiderInterface::GuiderInterface()
{
}

GuiderInterface::~GuiderInterface()
{
}

void GuiderInterface::initGuiderProperties(const char *deviceName, const char *groupName)
{
    IUFillNumber(&GuideNSN[DIRECTION_NORTH], "TIMED_GUIDE_N", "North (ms)", "%.f", 0, 60000, 100, 0);
    IUFillNumber(&GuideNSN[DIRECTION_SOUTH], "TIMED_GUIDE_S", "South (ms)", "%.f", 0, 60000, 100, 0);
    IUFillNumberVector(&GuideNSNP, GuideNSN, 2, deviceName, "TELESCOPE_TIMED_GUIDE_NS", "Guide N/S", groupName, IP_RW,
                       60, IPS_IDLE);

    IUFillNumber(&GuideWEN[DIRECTION_WEST], "TIMED_GUIDE_W", "West (ms)", "%.f", 0, 60000, 100, 0);
    IUFillNumber(&GuideWEN[DIRECTION_EAST], "TIMED_GUIDE_E", "East (ms)", "%.f", 0, 60000, 100, 0);
    IUFillNumberVector(&GuideWENP, GuideWEN, 2, deviceName, "TELESCOPE_TIMED_GUIDE_WE", "Guide E/W", groupName, IP_RW,
                       60, IPS_IDLE);
}

void GuiderInterface::processGuiderProperties(const char *name, double values[], char *names[], int n)
{
    if (strcmp(name, GuideNSNP.name) == 0)
    {
        //  We are being asked to send a guide pulse north/south on the st4 port
        IUUpdateNumber(&GuideNSNP, values, names, n);

        if (GuideNSN[DIRECTION_NORTH].value != 0)
        {
            GuideNSN[DIRECTION_SOUTH].value = 0;
            GuideNSNP.s                     = GuideNorth(GuideNSN[DIRECTION_NORTH].value);
        }
        else if (GuideNSN[DIRECTION_SOUTH].value != 0)
            GuideNSNP.s = GuideSouth(GuideNSN[DIRECTION_SOUTH].value);

        IDSetNumber(&GuideNSNP, nullptr);
        return;
    }

    if (strcmp(name, GuideWENP.name) == 0)
    {
        //  We are being asked to send a guide pulse north/south on the st4 port
        IUUpdateNumber(&GuideWENP, values, names, n);

        if (GuideWEN[DIRECTION_WEST].value != 0)
        {
            GuideWEN[DIRECTION_EAST].value = 0;
            GuideWENP.s                    = GuideWest(GuideWEN[DIRECTION_WEST].value);
        }
        else if (GuideWEN[DIRECTION_EAST].value != 0)
            GuideWENP.s = GuideEast(GuideWEN[DIRECTION_EAST].value);

        IDSetNumber(&GuideWENP, nullptr);
        return;
    }
}

void GuiderInterface::GuideComplete(INDI_EQ_AXIS axis)
{
    switch (axis)
    {
        case AXIS_DE:
            GuideNSNP.s = IPS_IDLE;
            IDSetNumber(&GuideNSNP, nullptr);
            break;

        case AXIS_RA:
            GuideWENP.s = IPS_IDLE;
            IDSetNumber(&GuideWENP, nullptr);
            break;
    }
}
}
