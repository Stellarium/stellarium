/*******************************************************************************
  Copyright(c) 2017 Jarno Paananen. All right reserved.

  Based on Flip Flat driver by:

  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

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

#include "snapcap.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <cerrno>
#include <cstring>
#include <memory>
#include <termios.h>
#include <sys/ioctl.h>

// We declare an auto pointer to SnapCap.
std::unique_ptr<SnapCap> snapcap(new SnapCap());

#define SNAP_CMD 7
#define SNAP_RES 8
#define SNAP_TIMEOUT 3
#define POLLMS 1000

void ISGetProperties(const char *dev)
{
    snapcap->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    snapcap->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    snapcap->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    snapcap->ISNewNumber(dev, name, values, names, n);
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
    snapcap->ISSnoopDevice(root);
}

SnapCap::SnapCap() : LightBoxInterface(this, true)
{
    setVersion(1, 0);
}

bool SnapCap::initProperties()
{
    INDI::DefaultDevice::initProperties();

    // Status
    IUFillText(&StatusT[0], "Cover", "", nullptr);
    IUFillText(&StatusT[1], "Light", "", nullptr);
    IUFillText(&StatusT[2], "Motor", "", nullptr);
    IUFillTextVector(&StatusTP, StatusT, 3, getDeviceName(), "Status", "", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Firmware version
    IUFillText(&FirmwareT[0], "Version", "", nullptr);
    IUFillTextVector(&FirmwareTP, FirmwareT, 1, getDeviceName(), "Firmware", "", MAIN_CONTROL_TAB, IP_RO, 60, IPS_IDLE);

    // Abort and force open/close buttons
    IUFillSwitch(&AbortS[0], "Abort", "", ISS_OFF);
    IUFillSwitchVector(&AbortSP, AbortS, 1, getDeviceName(), "Abort", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    IUFillSwitch(&ForceS[0], "Off", "", ISS_ON);
    IUFillSwitch(&ForceS[1], "On", "", ISS_OFF);
    IUFillSwitchVector(&ForceSP, ForceS, 2, getDeviceName(), "Force movement", "", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY,
                       0, IPS_IDLE);

    initDustCapProperties(getDeviceName(), MAIN_CONTROL_TAB);
    initLightBoxProperties(getDeviceName(), MAIN_CONTROL_TAB);

    LightIntensityN[0].min  = 25;
    LightIntensityN[0].max  = 255;
    LightIntensityN[0].step = 10;

    hasLight = true;

    setDriverInterface(AUX_INTERFACE | LIGHTBOX_INTERFACE | DUSTCAP_INTERFACE);

    addAuxControls();

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]() { return Handshake(); });
    registerConnection(serialConnection);
    serialConnection->setDefaultBaudRate(Connection::Serial::B_38400);
    return true;
}

void SnapCap::ISGetProperties(const char *dev)
{
    INDI::DefaultDevice::ISGetProperties(dev);

    // Get Light box properties
    isGetLightBoxProperties(dev);
}

bool SnapCap::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineSwitch(&ParkCapSP);
        if (hasLight)
        {
            defineSwitch(&LightSP);
            defineNumber(&LightIntensityNP);
            updateLightBoxProperties();
        }
        defineText(&StatusTP);
        defineText(&FirmwareTP);
        defineSwitch(&AbortSP);
        defineSwitch(&ForceSP);
        getStartupData();
    }
    else
    {
        deleteProperty(ParkCapSP.name);
        if (hasLight)
        {
            deleteProperty(LightSP.name);
            deleteProperty(LightIntensityNP.name);
            updateLightBoxProperties();
        }
        deleteProperty(StatusTP.name);
        deleteProperty(FirmwareTP.name);
        deleteProperty(AbortSP.name);
        deleteProperty(ForceSP.name);
    }

    return true;
}

const char *SnapCap::getDefaultName()
{
    return (const char *)"SnapScap";
}

bool SnapCap::Handshake()
{
    if (isSimulation())
    {
        DEBUGF(INDI::Logger::DBG_SESSION, "Connected successfully to simulated %s. Retrieving startup data...",
               getDeviceName());

        SetTimer(POLLMS);
        return true;
    }

    PortFD = serialConnection->getPortFD();

    /* Drop RTS */
    int i = 0;
    i |= TIOCM_RTS;
    if (ioctl(PortFD, TIOCMBIC, &i) != 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "IOCTL error %s.", strerror(errno));
        return false;
    }

    i |= TIOCM_RTS;
    if (ioctl(PortFD, TIOCMGET, &i) != 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "IOCTL error %s.", strerror(errno));
        return false;
    }

    if (!ping())
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Device ping failed.");
        return false;
    }

    return true;
}

bool SnapCap::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (processLightBoxNumber(dev, name, values, names, n))
        return true;

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool SnapCap::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (processLightBoxText(dev, name, texts, names, n))
            return true;
    }

    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool SnapCap::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(AbortSP.name, name) == 0)
        {
            IUResetSwitch(&AbortSP);
            AbortSP.s = Abort();
            IDSetSwitch(&AbortSP, nullptr);
            return true;
        }
        if (strcmp(ForceSP.name, name) == 0)
        {
            IUUpdateSwitch(&ForceSP, states, names, n);
            IDSetSwitch(&AbortSP, nullptr);
            return true;
        }

        if (processDustCapSwitch(dev, name, states, names, n))
            return true;

        if (processLightBoxSwitch(dev, name, states, names, n))
            return true;
    }

    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool SnapCap::ISSnoopDevice(XMLEle *root)
{
    snoopLightBox(root);

    return INDI::DefaultDevice::ISSnoopDevice(root);
}

bool SnapCap::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);

    return saveLightBoxConfigItems(fp);
}

bool SnapCap::ping()
{
    return getFirmwareVersion();
}

bool SnapCap::getStartupData()
{
    bool rc1 = getFirmwareVersion();
    bool rc2 = getStatus();
    bool rc3 = getBrightness();

    return (rc1 && rc2 && rc3);
}

IPState SnapCap::ParkCap()
{
    if (isSimulation())
    {
        simulationWorkCounter = 3;
        return IPS_BUSY;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    if (ForceS[1].s == ISS_ON)
        strncpy(command, ">c000", SNAP_CMD); // Force close command
    else
        strncpy(command, ">C000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return IPS_ALERT;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return IPS_ALERT;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    if (strcmp(response, "*C000") == 0 || strcmp(response, "*c000") == 0)
    {
        // Set cover status to random value outside of range to force it to refresh
        prevCoverStatus   = 10;
        targetCoverStatus = 2;
        return IPS_BUSY;
    }
    else
        return IPS_ALERT;
}

IPState SnapCap::UnParkCap()
{
    if (isSimulation())
    {
        simulationWorkCounter = 3;
        return IPS_BUSY;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    if (ForceS[1].s == ISS_ON)
        strncpy(command, ">o000", SNAP_CMD); // Force open command
    else
        strncpy(command, ">O000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return IPS_ALERT;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return IPS_ALERT;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    if (strcmp(response, "*O000") == 0 || strcmp(response, "*o000") == 0)
    {
        // Set cover status to random value outside of range to force it to refresh
        prevCoverStatus   = 10;
        targetCoverStatus = 1;
        return IPS_BUSY;
    }
    else
        return IPS_ALERT;
}

IPState SnapCap::Abort()
{
    if (isSimulation())
    {
        simulationWorkCounter = 0;
        return IPS_OK;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    strncpy(command, ">A000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return IPS_ALERT;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return IPS_ALERT;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    if (strcmp(response, "*A000") == 0)
    {
        // Set cover status to random value outside of range to force it to refresh
        prevCoverStatus = 10;
        return IPS_OK;
    }
    else
        return IPS_ALERT;
}

bool SnapCap::EnableLightBox(bool enable)
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    if (hasLight && ParkCapS[CAP_UNPARK].s == ISS_ON)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot control light while cap is unparked.");
        return false;
    }

    if (isSimulation())
        return true;

    tcflush(PortFD, TCIOFLUSH);

    if (enable)
        strncpy(command, ">L000", SNAP_CMD);
    else
        strncpy(command, ">D000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char expectedResponse[SNAP_RES];
    if (enable)
        snprintf(expectedResponse, SNAP_RES, "*L000");
    else
        snprintf(expectedResponse, SNAP_RES, "*D000");

    if (strcmp(response, expectedResponse) == 0)
        return true;

    return false;
}

bool SnapCap::getStatus()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    if (isSimulation())
    {
        if (ParkCapSP.s == IPS_BUSY && --simulationWorkCounter <= 0)
        {
            ParkCapSP.s = IPS_IDLE;
            IDSetSwitch(&ParkCapSP, nullptr);
            simulationWorkCounter = 0;
        }

        if (ParkCapSP.s == IPS_BUSY)
        {
            response[2] = '1';
            response[4] = '0';
        }
        else
        {
            response[2] = '0';
            // Parked/Closed
            if (ParkCapS[CAP_PARK].s == ISS_ON)
                response[4] = '2';
            else
                response[4] = '1';
        }

        response[3] = (LightS[FLAT_LIGHT_ON].s == ISS_ON) ? '1' : '0';
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);

        strncpy(command, ">S000", SNAP_CMD);

        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

        command[SNAP_CMD - 2] = 0xD;
        command[SNAP_CMD - 1] = 0xA;

        if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
            return false;
        }

        if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
            return false;
        }

        response[nbytes_read - 2] = '\0';

        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);
    }

    char motorStatus = response[2] - '0';
    char lightStatus = response[3] - '0';
    char coverStatus = response[4] - '0';

    // Force cover status as it doesn't reflect moving state otherwise...
    if (motorStatus)
    {
        coverStatus = 0;
    }
    bool statusUpdated = false;

    if (coverStatus != prevCoverStatus)
    {
        prevCoverStatus = coverStatus;

        statusUpdated = true;

        switch (coverStatus)
        {
            case 0:
                IUSaveText(&StatusT[0], "Opening/closing");
                break;

            case 1:
                if ((targetCoverStatus == 1 && ParkCapSP.s == IPS_BUSY) || ParkCapSP.s == IPS_IDLE)
                {
                    IUSaveText(&StatusT[0], "Open");
                    IUResetSwitch(&ParkCapSP);
                    ParkCapS[CAP_UNPARK].s = ISS_ON;
                    ParkCapSP.s            = IPS_OK;
                    DEBUG(INDI::Logger::DBG_SESSION, "Cover open.");
                    IDSetSwitch(&ParkCapSP, nullptr);
                }
                break;

            case 2:
                if ((targetCoverStatus == 2 && ParkCapSP.s == IPS_BUSY) || ParkCapSP.s == IPS_IDLE)
                {
                    IUSaveText(&StatusT[0], "Closed");
                    IUResetSwitch(&ParkCapSP);
                    ParkCapS[CAP_PARK].s = ISS_ON;
                    ParkCapSP.s          = IPS_OK;
                    DEBUG(INDI::Logger::DBG_SESSION, "Cover closed.");
                    IDSetSwitch(&ParkCapSP, nullptr);
                }
                break;

            case 3:
                IUSaveText(&StatusT[0], "Timed out");
                break;

            case 4:
                IUSaveText(&StatusT[0], "Open circuit");
                break;

            case 5:
                IUSaveText(&StatusT[0], "Overcurrent");
                break;

            case 6:
                IUSaveText(&StatusT[0], "User abort");
                break;
        }
    }

    if (lightStatus != prevLightStatus)
    {
        prevLightStatus = lightStatus;

        statusUpdated = true;

        switch (lightStatus)
        {
            case 0:
                IUSaveText(&StatusT[1], "Off");
                if (LightS[0].s == ISS_ON)
                {
                    LightS[0].s = ISS_OFF;
                    LightS[1].s = ISS_ON;
                    IDSetSwitch(&LightSP, nullptr);
                }
                break;

            case 1:
                IUSaveText(&StatusT[1], "On");
                if (LightS[1].s == ISS_ON)
                {
                    LightS[0].s = ISS_ON;
                    LightS[1].s = ISS_OFF;
                    IDSetSwitch(&LightSP, nullptr);
                }
                break;
        }
    }

    if (motorStatus != prevMotorStatus)
    {
        prevMotorStatus = motorStatus;

        statusUpdated = true;

        switch (motorStatus)
        {
            case 0:
                IUSaveText(&StatusT[2], "Stopped");
                break;

            case 1:
                IUSaveText(&StatusT[2], "Running");
                break;
        }
    }

    if (statusUpdated)
        IDSetText(&StatusTP, nullptr);

    return true;
}

bool SnapCap::getFirmwareVersion()
{
    if (isSimulation())
    {
        IUSaveText(&FirmwareT[0], "Simulation");
        IDSetText(&FirmwareTP, nullptr);
        return true;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    strncpy(command, ">V000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char versionString[4] = { 0 };
    snprintf(versionString, 4, "%s", response + 2);
    IUSaveText(&FirmwareT[0], versionString);
    IDSetText(&FirmwareTP, nullptr);

    return true;
}

void SnapCap::TimerHit()
{
    if (!isConnected())
        return;

    getStatus();

    SetTimer(POLLMS);
}

bool SnapCap::getBrightness()
{
    if (isSimulation())
    {
        return true;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    strncpy(command, ">J000", SNAP_CMD);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char brightnessString[4] = { 0 };
    snprintf(brightnessString, 4, "%s", response + 2);

    int brightnessValue = 0;
    rc                  = sscanf(brightnessString, "%d", &brightnessValue);

    if (rc <= 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unable to parse brightness value (%s)", response);
        return false;
    }

    if (brightnessValue != prevBrightness)
    {
        prevBrightness           = brightnessValue;
        LightIntensityN[0].value = brightnessValue;
        IDSetNumber(&LightIntensityNP, nullptr);
    }

    return true;
}

bool SnapCap::SetLightBoxBrightness(uint16_t value)
{
    if (isSimulation())
    {
        LightIntensityN[0].value = value;
        IDSetNumber(&LightIntensityNP, nullptr);
        return true;
    }

    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[SNAP_CMD];
    char response[SNAP_RES];

    tcflush(PortFD, TCIOFLUSH);

    snprintf(command, SNAP_CMD, ">B%03d", value);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[SNAP_CMD - 2] = 0xD;
    command[SNAP_CMD - 1] = 0xA;

    if ((rc = tty_write(PortFD, command, SNAP_CMD, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s error: %s.", command, errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xA, SNAP_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s: %s.", command, errstr);
        return false;
    }

    response[nbytes_read - 2] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    char brightnessString[4] = { 0 };
    snprintf(brightnessString, 4, "%s", response + 2);

    int brightnessValue = 0;
    rc                  = sscanf(brightnessString, "%d", &brightnessValue);

    if (rc <= 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Unable to parse brightness value (%s)", response);
        return false;
    }

    if (brightnessValue != prevBrightness)
    {
        prevBrightness           = brightnessValue;
        LightIntensityN[0].value = brightnessValue;
        IDSetNumber(&LightIntensityNP, nullptr);
    }

    return true;
}
