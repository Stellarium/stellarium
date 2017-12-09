/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

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

#include "xagyl_wheel.h"

#include "indicom.h"

#include <cstring>
#include <memory>

#include <termios.h>

#define XAGYL_MAXBUF 32
#define SETTINGS_TAB "Settings"

// We declare an auto pointer to XAGYLWheel.
std::unique_ptr<XAGYLWheel> xagylWheel(new XAGYLWheel());

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    xagylWheel->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    xagylWheel->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    xagylWheel->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    xagylWheel->ISNewNumber(dev, name, values, names, n);
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
    xagylWheel->ISSnoopDevice(root);
}

XAGYLWheel::XAGYLWheel()
{
    simData.position   = 1;
    simData.speed      = 0xA;
    simData.pulseWidth = 1500;
    simData.threshold  = 30;
    simData.jitter     = 1;
    simData.offset[0] = simData.offset[1] = simData.offset[2] = simData.offset[3] = simData.offset[4] = 0;
    strncpy(simData.product, "Xagyl FW5125VX", 16);
    strncpy(simData.version, "FW3.1.5", 16);
    strncpy(simData.serial, "S/N: 123456", 16);

    setVersion(0, 1);

    setFilterConnection(CONNECTION_SERIAL | CONNECTION_TCP);
}

const char *XAGYLWheel::getDefaultName()
{
    return (const char *)"XAGYL Wheel";
}

bool XAGYLWheel::initProperties()
{
    INDI::FilterWheel::initProperties();

    // Firmware info
    IUFillText(&FirmwareInfoT[0], "Product", "", nullptr);
    IUFillText(&FirmwareInfoT[1], "Firmware", "", nullptr);
    IUFillText(&FirmwareInfoT[2], "Serial #", "", nullptr);
    IUFillTextVector(&FirmwareInfoTP, FirmwareInfoT, 3, getDeviceName(), "Info", "", MAIN_CONTROL_TAB, IP_RO, 60,
                     IPS_IDLE);

    // Settings
    IUFillNumber(&SettingsN[0], "Speed", "", "%.f", 0, 100, 10., 0.);
    IUFillNumber(&SettingsN[1], "Jitter", "", "%.f", 0, 10, 1., 0.);
    IUFillNumber(&SettingsN[2], "Threshold", "", "%.f", 0, 100, 10., 0.);
    IUFillNumber(&SettingsN[3], "Pulse Width", "", "%.f", 100, 10000, 100., 0.);
    IUFillNumberVector(&SettingsNP, SettingsN, 4, getDeviceName(), "Settings", "", SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    // Reset
    IUFillSwitch(&ResetS[0], "Reboot", "", ISS_OFF);
    IUFillSwitch(&ResetS[1], "Initialize", "", ISS_OFF);
    IUFillSwitch(&ResetS[2], "Clear Calibration", "", ISS_OFF);
    IUFillSwitch(&ResetS[3], "Perform Calibration", "", ISS_OFF);
    IUFillSwitchVector(&ResetSP, ResetS, 4, getDeviceName(), "Commands", "", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1, 0,
                       IPS_IDLE);

    addAuxControls();

    return true;
}

bool XAGYLWheel::updateProperties()
{
    INDI::FilterWheel::updateProperties();

    if (isConnected())
    {
        getStartupData();

        defineSwitch(&ResetSP);
        defineNumber(&OffsetNP);
        defineText(&FirmwareInfoTP);
        defineNumber(&SettingsNP);
    }
    else
    {
        deleteProperty(ResetSP.name);
        deleteProperty(OffsetNP.name);
        deleteProperty(FirmwareInfoTP.name);
        deleteProperty(SettingsNP.name);
    }

    return true;
}

bool XAGYLWheel::Handshake()
{
    char resp[XAGYL_MAXBUF];
    bool rc = getCommand(INFO_FIRMWARE_VERSION, resp);

    if (rc)
    {
        int fwver = 0;
        int fw_rc = sscanf(resp, "%d", &fwver);

        if (fw_rc != 1)
            fw_rc = sscanf(resp, "FW %d", &fwver);

        if (fw_rc > 0)
        {
            firmwareVersion = fwver;

            // We don't have pulse width for version < 3
            if (firmwareVersion < 3)
                SettingsNP.nnp--;

            if (getMaxFilterSlots())
            {
                initOffset();

                DEBUG(INDI::Logger::DBG_SESSION, "XAGYL is online. Getting filter parameters...");
                return true;
            }
        }
        else
            DEBUGF(INDI::Logger::DBG_ERROR, "Unable to parse (%s)", resp);
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Error retreiving data from XAGYL Filter Wheel, please ensure filter wheel is "
                                     "powered and the port is correct.");
    return false;
}

bool XAGYLWheel::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(ResetSP.name, name) == 0)
        {
            IUUpdateSwitch(&ResetSP, states, names, n);
            int value = IUFindOnSwitchIndex(&ResetSP);
            IUResetSwitch(&ResetSP);

            if (value == 3)
                value = 6;

            bool rc = reset(value);

            if (rc)
            {
                switch (value)
                {
                    case 0:
                        DEBUG(INDI::Logger::DBG_SESSION, "Executing hard reboot...");
                        break;

                    case 1:
                        DEBUG(INDI::Logger::DBG_SESSION, "Restarting and moving to filter position #1...");
                        break;

                    case 2:
                        DEBUG(INDI::Logger::DBG_SESSION, "Calibration removed.");
                        break;

                    case 6:
                        DEBUG(INDI::Logger::DBG_SESSION, "Calibrating...");
                        break;
                }
            }

            ResetSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&ResetSP, nullptr);

            return true;
        }
    }

    return INDI::FilterWheel::ISNewSwitch(dev, name, states, names, n);
}

bool XAGYLWheel::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(OffsetNP.name, name) == 0)
        {
            bool rc_offset = true;

            for (int i = 0; i < n; i++)
            {
                if (strcmp(names[i], OffsetN[i].name) == 0)
                {
                    while (values[i] != OffsetN[i].value && rc_offset)
                    {
                        if (values[i] > OffsetN[i].value)
                            rc_offset = setOffset(i, 1);
                        else
                            rc_offset = setOffset(i, -1);
                    }
                }
            }

            OffsetNP.s = rc_offset ? IPS_OK : IPS_ALERT;
            IDSetNumber(&OffsetNP, nullptr);
            return true;
        }

        if (strcmp(SettingsNP.name, name) == 0)
        {
            double newSpeed = 0, newJitter = 0, newThreshold = 0, newPulseWidth = 0;
            for (int i = 0; i < n; i++)
            {
                if (strcmp(names[i], SettingsN[SET_SPEED].name) == 0)
                    newSpeed = values[i];
                else if (strcmp(names[i], SettingsN[SET_JITTER].name) == 0)
                    newJitter = values[i];
                if (strcmp(names[i], SettingsN[SET_THRESHOLD].name) == 0)
                    newThreshold = values[i];
                if (strcmp(names[i], SettingsN[SET_PULSE_WITDH].name) == 0)
                    newPulseWidth = values[i];
            }

            bool rc_speed = true, rc_jitter = true, rc_threshold = true, rc_pulsewidth = true;

            if (newSpeed != SettingsN[SET_SPEED].value)
            {
                rc_speed = setCommand(SET_SPEED, newSpeed);
                getMaximumSpeed();
            }

            // Jitter
            while (newJitter != SettingsN[SET_JITTER].value && rc_jitter)
            {
                if (newJitter > SettingsN[SET_JITTER].value)
                {
                    rc_jitter &= setCommand(SET_JITTER, 1);
                    getJitter();
                }
                else
                {
                    rc_jitter &= setCommand(SET_JITTER, -1);
                    getJitter();
                }
            }

            // Threshold
            while (newThreshold != SettingsN[SET_THRESHOLD].value && rc_threshold)
            {
                if (newThreshold > SettingsN[SET_THRESHOLD].value)
                {
                    rc_threshold &= setCommand(SET_THRESHOLD, 1);
                    getThreshold();
                }
                else
                {
                    rc_threshold &= setCommand(SET_THRESHOLD, -1);
                    getThreshold();
                }
            }

            // Pulse width
            while (firmwareVersion >= 3 && newPulseWidth != SettingsN[SET_PULSE_WITDH].value && rc_pulsewidth)
            {
                if (newPulseWidth > SettingsN[SET_PULSE_WITDH].value)
                {
                    rc_pulsewidth &= setCommand(SET_PULSE_WITDH, 1);
                    getPulseWidth();
                }
                else
                {
                    rc_pulsewidth &= setCommand(SET_PULSE_WITDH, -1);
                    getPulseWidth();
                }
            }

            if (rc_speed && rc_jitter && rc_threshold && rc_pulsewidth)
                SettingsNP.s = IPS_OK;
            else
                SettingsNP.s = IPS_ALERT;

            IDSetNumber(&SettingsNP, nullptr);

            return true;
        }
    }

    return INDI::FilterWheel::ISNewNumber(dev, name, values, names, n);
}

void XAGYLWheel::initOffset()
{
    free(OffsetN);
    OffsetN = (INumber *)malloc(FilterSlotN[0].max * sizeof(INumber));
    char offsetName[MAXINDINAME], offsetLabel[MAXINDILABEL];
    for (int i = 0; i < FilterSlotN[0].max; i++)
    {
        snprintf(offsetName, MAXINDINAME, "OFFSET_%d", i + 1);
        snprintf(offsetLabel, MAXINDINAME, "#%d Offset", i + 1);
        IUFillNumber(OffsetN + i, offsetName, offsetLabel, "%.f", -99, 99, 10, 0);
    }

    IUFillNumberVector(&OffsetNP, OffsetN, FilterSlotN[0].max, getDeviceName(), "Offsets", "", FILTER_TAB, IP_RW, 0,
                       IPS_IDLE);
}

bool XAGYLWheel::getCommand(GET_COMMAND cmd, char *result)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[XAGYL_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    snprintf(command, XAGYL_MAXBUF, "I%d", cmd);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    if (!sim && (rc = tty_write(PortFD, command, strlen(command), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if (sim)
    {
        switch (cmd)
        {
            case INFO_PRODUCT_NAME:
                snprintf(result, XAGYL_MAXBUF, "%s", simData.product);
                break;

            case INFO_FIRMWARE_VERSION:
                snprintf(result, XAGYL_MAXBUF, "%s", simData.version);
                break;

            case INFO_SERIAL_NUMBER:
                snprintf(result, XAGYL_MAXBUF, "%s", simData.serial);
                break;

            case INFO_FILTER_POSITION:
                snprintf(result, XAGYL_MAXBUF, "P%d", simData.position);
                break;

            case INFO_MAX_SPEED:
                snprintf(result, XAGYL_MAXBUF, "MaxSpeed %02d%%", simData.speed * 10);
                break;

            case INFO_JITTER:
                snprintf(result, XAGYL_MAXBUF, "Jitter %d", simData.jitter);
                break;

            case INFO_OFFSET:
                snprintf(result, XAGYL_MAXBUF, "P%d Offset %02d", CurrentFilter, simData.offset[CurrentFilter - 1]);
                break;

            case INFO_THRESHOLD:
                snprintf(result, XAGYL_MAXBUF, "Threshold %02d", simData.threshold);
                break;

            case INFO_MAX_SLOTS:
                snprintf(result, XAGYL_MAXBUF, "FilterSlots %d", 5);
                break;

            case INFO_PULSE_WIDTH:
                snprintf(result, XAGYL_MAXBUF, "Pulse Width %05duS", simData.pulseWidth);
                break;
        }
    }
    else
    {
        if ((rc = tty_read_section(PortFD, result, 0xA, XAGYL_MAXBUF, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
            return false;
        }

        result[nbytes_read - 1] = '\0';
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", result);

    return true;
}

bool XAGYLWheel::setCommand(SET_COMMAND cmd, int value)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[XAGYL_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    switch (cmd)
    {
        case SET_SPEED:
            snprintf(command, XAGYL_MAXBUF, "S%X", value / 10);
            break;

        case SET_JITTER:
            snprintf(command, XAGYL_MAXBUF, "%s0", value > 0 ? "]" : "[");
            break;

        case SET_THRESHOLD:
            snprintf(command, XAGYL_MAXBUF, "%s0", value > 0 ? "}" : "{");
            break;

        case SET_PULSE_WITDH:
            snprintf(command, XAGYL_MAXBUF, "%s0", value > 0 ? "M" : "N");
            break;

        case SET_POSITION:
            snprintf(command, XAGYL_MAXBUF, "G%X", value);
            break;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    if (!sim && (rc = tty_write(PortFD, command, strlen(command), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    // Commands that have no reply
    switch (cmd)
    {
        case SET_POSITION:
            simData.position = value;
            return true;
            break;

        default:
            break;
    }

    char response[XAGYL_MAXBUF];

    if (sim)
    {
        switch (cmd)
        {
            case SET_SPEED:
                simData.speed = value / 10;
                snprintf(response, XAGYL_MAXBUF, "Speed=%3d%%", simData.speed * 10);
                break;

            case SET_JITTER:
                simData.jitter += (value > 0 ? 1 : -1);
                if (simData.jitter > SettingsN[SET_JITTER].max)
                    simData.jitter = SettingsN[SET_JITTER].max;
                else if (simData.jitter < SettingsN[SET_JITTER].min)
                    simData.jitter = SettingsN[SET_JITTER].min;

                snprintf(response, XAGYL_MAXBUF, "Jitter %d", simData.jitter);
                break;

            case SET_THRESHOLD:
                simData.threshold += (value > 0 ? 1 : -1);
                if (simData.threshold > SettingsN[SET_THRESHOLD].max)
                    simData.threshold = SettingsN[SET_THRESHOLD].max;
                else if (simData.threshold < SettingsN[SET_THRESHOLD].min)
                    simData.threshold = SettingsN[SET_THRESHOLD].min;

                snprintf(response, XAGYL_MAXBUF, "Threshold %d", simData.threshold);
                break;

            case SET_PULSE_WITDH:
                simData.pulseWidth += 100 * (value > 0 ? 1 : -1);
                if (simData.pulseWidth > SettingsN[SET_PULSE_WITDH].max)
                    simData.pulseWidth = SettingsN[SET_PULSE_WITDH].max;
                else if (simData.pulseWidth < SettingsN[SET_PULSE_WITDH].min)
                    simData.pulseWidth = SettingsN[SET_PULSE_WITDH].min;

                snprintf(response, XAGYL_MAXBUF, "pulseWidth %d", simData.pulseWidth);
                break;

            default:
                break;
        }
    }
    else if ((rc = tty_read_section(PortFD, response, 0xA, XAGYL_MAXBUF, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    return true;
}

bool XAGYLWheel::SelectFilter(int f)
{
    TargetFilter = f;

    bool rc = setCommand(SET_POSITION, f);

    if (rc)
    {
        SetTimer(500);
        return true;
    }
    else
        return false;
}

void XAGYLWheel::TimerHit()
{
    bool rc = getFilterPosition();

    if (!rc)
    {
        SetTimer(500);
        return;
    }

    if (CurrentFilter == TargetFilter)
        SelectFilterDone(CurrentFilter);
    else
        SetTimer(500);
}

bool XAGYLWheel::getStartupData()
{
    bool rc1 = getFirmwareInfo();

    bool rc2 = getSettingInfo();

    for (int i = 0; i < OffsetNP.nnp; i++)
        getOffset(i);

    return (rc1 && rc2);
}

bool XAGYLWheel::getFirmwareInfo()
{
    char resp[XAGYL_MAXBUF];

    bool rc1 = getCommand(INFO_PRODUCT_NAME, resp);
    if (rc1)
        IUSaveText(&FirmwareInfoT[0], resp);

    bool rc2 = getCommand(INFO_FIRMWARE_VERSION, resp);
    if (rc2)
        IUSaveText(&FirmwareInfoT[1], resp);

    bool rc3 = getCommand(INFO_SERIAL_NUMBER, resp);
    if (rc3)
        IUSaveText(&FirmwareInfoT[2], resp);

    return (rc1 && rc2 && rc3);
}

bool XAGYLWheel::getSettingInfo()
{
    bool rc1 = getMaximumSpeed();
    bool rc2 = getJitter();
    bool rc3 = getThreshold();
    bool rc4 = true;

    if (firmwareVersion >= 3)
        rc4 = getPulseWidth();

    return (rc1 && rc2 && rc3 && rc4);
}

bool XAGYLWheel::getFilterPosition()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_FILTER_POSITION, resp))
        return false;

    int rc = sscanf(resp, "P%d", &CurrentFilter);

    if (rc > 0)
    {
        FilterSlotN[0].value = CurrentFilter;
        return true;
    }
    else
        return false;
}

bool XAGYLWheel::getMaximumSpeed()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_MAX_SPEED, resp))
        return false;

    int maxSpeed = 0;
    int rc       = sscanf(resp, "MaxSpeed %d%%", &maxSpeed);

    if (rc > 0)
    {
        SettingsN[SET_SPEED].value = maxSpeed;
        return true;
    }
    return false;
}

bool XAGYLWheel::getJitter()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_JITTER, resp))
        return false;

    int jitter = 0;
    int rc     = sscanf(resp, "Jitter %d", &jitter);

    if (rc > 0)
    {
        SettingsN[SET_JITTER].value = jitter;
        return true;
    }
    return false;
}

bool XAGYLWheel::getThreshold()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_THRESHOLD, resp))
        return false;

    int threshold = 0;
    int rc        = sscanf(resp, "Threshold %d", &threshold);

    if (rc > 0)
    {
        SettingsN[SET_THRESHOLD].value = threshold;
        return true;
    }
    return false;
}

bool XAGYLWheel::getPulseWidth()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_PULSE_WIDTH, resp))
        return false;

    int pulseWidth = 0;
    int rc         = sscanf(resp, "Pulse Width %duS", &pulseWidth);

    if (rc > 0)
    {
        SettingsN[SET_PULSE_WITDH].value = pulseWidth;
        return true;
    }
    return false;
}

bool XAGYLWheel::getMaxFilterSlots()
{
    char resp[XAGYL_MAXBUF];

    if (!getCommand(INFO_MAX_SLOTS, resp))
        return false;

    int maxFilterSlots = 0;
    int rc             = sscanf(resp, "FilterSlots %d", &maxFilterSlots);

    if (rc > 0)
    {
        FilterSlotN[0].max = maxFilterSlots;
        return true;
    }
    return false;
}

bool XAGYLWheel::reset(int value)
{
    int nbytes_written = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[XAGYL_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    snprintf(command, XAGYL_MAXBUF, "R%d", value);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    if (!sim && (rc = tty_write(PortFD, command, strlen(command), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if (value == 1)
        simData.position = 1;

    getFilterPosition();

    return true;
}

bool XAGYLWheel::setOffset(int filter, int value)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[XAGYL_MAXBUF];
    char resp[XAGYL_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    snprintf(command, XAGYL_MAXBUF, "%s", value > 0 ? "(" : ")");

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    if (!sim && (rc = tty_write(PortFD, command, strlen(command), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if (sim)
    {
        simData.offset[filter] += value;
        snprintf(resp, XAGYL_MAXBUF, "P%d Offset %02d", filter + 1, simData.offset[filter]);
    }
    else if ((rc = tty_read_section(PortFD, resp, 0xA, XAGYL_MAXBUF, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    int filter_num = 0, offset = 0;
    rc = sscanf(resp, "P%d Offset %d", &filter_num, &offset);

    if (rc > 0)
    {
        OffsetN[filter_num - 1].value = offset;
        return true;
    }
    else
        return false;
}

bool XAGYLWheel::getOffset(int filter)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[XAGYL_MAXBUF];
    char resp[XAGYL_MAXBUF];

    tcflush(PortFD, TCIOFLUSH);

    snprintf(command, XAGYL_MAXBUF, "O%d", filter + 1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    if (!sim && (rc = tty_write(PortFD, command, strlen(command), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if (sim)
        snprintf(resp, XAGYL_MAXBUF, "P%d Offset %02d", filter + 1, simData.offset[filter]);
    else
    {
        if ((rc = tty_read_section(PortFD, resp, 0xA, XAGYL_MAXBUF, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
            return false;
        }

        resp[nbytes_read - 1] = '\0';
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", resp);

    int filter_num = 0, offset = 0;
    rc = sscanf(resp, "P%d Offset %d", &filter_num, &offset);

    if (rc > 0)
    {
        OffsetN[filter_num - 1].value = offset;
        return true;
    }
    else
        return false;
}
