/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  Tru Technology Filter Wheel

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "trutech_wheel.h"

#include "indicom.h"

#include <cstring>
#include <memory>

#define CMD_SIZE 5

//const uint8_t COMM_PRE  = 0x01;
const uint8_t COMM_INIT = 0xA5;
const uint8_t COMM_FILL = 0x20;

// We declare an auto pointer to TruTech.
std::unique_ptr<TruTech> tru_wheel(new TruTech());

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    tru_wheel->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    tru_wheel->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    tru_wheel->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    tru_wheel->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}
void ISSnoopDevice(XMLEle *root)
{
    tru_wheel->ISSnoopDevice(root);
}

TruTech::TruTech()
{
    setFilterConnection(CONNECTION_SERIAL | CONNECTION_TCP);
}

const char *TruTech::getDefaultName()
{
    return (const char *)"TruTech Wheel";
}

bool TruTech::initProperties()
{
    INDI::FilterWheel::initProperties();

    IUFillSwitch(&HomeS[0], "Find", "Find", ISS_OFF);
    IUFillSwitchVector(&HomeSP, HomeS, 1, getDeviceName(), "HOME", "Home", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 60,
                       IPS_IDLE);

    CurrentFilter      = 1;
    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 5;

    addAuxControls();
    return true;
}

bool TruTech::updateProperties()
{
    INDI::FilterWheel::updateProperties();

    if (isConnected())
        defineSwitch(&HomeSP);
    else
        deleteProperty(HomeSP.name);

    return true;
}

bool TruTech::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(HomeSP.name, name) == 0)
        {
            int rc = 0, nbytes_written = 0;
            uint8_t type   = 0x03;
            uint8_t chksum = COMM_INIT + type + COMM_FILL;
            char filter_command[CMD_SIZE];
            snprintf(filter_command, CMD_SIZE, "%c%c%c%c", COMM_INIT, type, COMM_FILL, chksum);

            DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %#02X %#02X %#02X %#02X", COMM_INIT, type, COMM_FILL, chksum);

            if (!isSimulation() &&
                (rc = tty_write(PortFD, filter_command, CMD_SIZE, &nbytes_written)) != TTY_OK)
            {
                char error_message[ERRMSG_SIZE];
                tty_error_msg(rc, error_message, ERRMSG_SIZE);

                HomeSP.s = IPS_ALERT;
                DEBUGF(INDI::Logger::DBG_ERROR, "Sending command Home to filter failed: %s", error_message);
            }
            else
            {
                CurrentFilter        = 1;
                FilterSlotN[0].value = 1;
                FilterSlotNP.s       = IPS_OK;
                HomeSP.s             = IPS_OK;
                DEBUG(INDI::Logger::DBG_SESSION, "Filter set to Home.");
                IDSetNumber(&FilterSlotNP, nullptr);
            }

            IDSetSwitch(&HomeSP, nullptr);
            return true;
        }
    }

    return INDI::FilterWheel::ISNewSwitch(dev, name, states, names, n);
}

bool TruTech::Handshake()
{
    // How do we do handshake with TruTech? We return true for now
    return true;
}

bool TruTech::SelectFilter(int f)
{
    TargetFilter = f;

    int rc = 0, nbytes_written = 0;
    char filter_command[CMD_SIZE];
    uint8_t type   = 0x01;
    uint8_t chksum = COMM_INIT + type + static_cast<uint8_t>(f);
    snprintf(filter_command, CMD_SIZE, "%c%c%c%c", COMM_INIT, type, f, chksum);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %#02X %#02X %#02X %#02X", COMM_INIT, type, f, chksum);

    if (!isSimulation() && (rc = tty_write(PortFD, filter_command, CMD_SIZE, &nbytes_written)) != TTY_OK)
    {
        char error_message[ERRMSG_SIZE];
        tty_error_msg(rc, error_message, ERRMSG_SIZE);

        DEBUGF(INDI::Logger::DBG_ERROR, "Sending command select filter failed: %s", error_message);
        return false;
    }

    // How do we check on TruTech if filter arrived? Check later
    CurrentFilter = f;
    SelectFilterDone(CurrentFilter);
    return true;
}

void TruTech::TimerHit()
{
    // Maybe needed later?
}
