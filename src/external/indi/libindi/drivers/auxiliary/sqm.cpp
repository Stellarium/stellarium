/*******************************************************************************
  Copyright(c) 2016 Jasem Mutlaq. All rights reserved.

  INDI Sky Quality Meter Driver

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

#include "sqm.h"

#include "connectionplugins/connectiontcp.h"
#include "connectionplugins/connectionserial.h"

#include <cerrno>
#include <memory>
#include <cstring>
#include <unistd.h>

// We declare an auto pointer to SQM.
std::unique_ptr<SQM> sqm(new SQM());

#define UNIT_TAB    "Unit"
#define POLLMS      1000

void ISGetProperties(const char *dev)
{
    sqm->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    sqm->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    sqm->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    sqm->ISNewNumber(dev, name, values, names, n);
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
    sqm->ISSnoopDevice(root);
}

SQM::SQM()
{
    setVersion(1, 0);
}

bool SQM::initProperties()
{
    INDI::DefaultDevice::initProperties();

    // Average Readings
    IUFillNumber(&AverageReadingN[0], "SKY_BRIGHTNESS", "Quality (mag/arcsec^2)", "%6.2f", -20, 30, 0, 0);
    IUFillNumber(&AverageReadingN[1], "SENSOR_FREQUENCY", "Freq (Hz)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&AverageReadingN[2], "SENSOR_COUNTS", "Period (counts)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&AverageReadingN[3], "SENSOR_PERIOD", "Period (s)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&AverageReadingN[4], "SKY_TEMPERATURE", "Temperature (C)", "%6.2f", -50, 80, 0, 0);
    IUFillNumberVector(&AverageReadingNP, AverageReadingN, 5, getDeviceName(), "SKY_QUALITY", "Readings",
                       MAIN_CONTROL_TAB, IP_RO, 0, IPS_IDLE);

    // Unit Info
    IUFillNumber(&UnitInfoN[0], "Protocol", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&UnitInfoN[1], "Model", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&UnitInfoN[2], "Feature", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&UnitInfoN[3], "Serial", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumberVector(&UnitInfoNP, UnitInfoN, 4, getDeviceName(), "Unit Info", "", UNIT_TAB, IP_RW, 0, IPS_IDLE);



    if (sqmConnection & CONNECTION_SERIAL)
    {
        serialConnection = new Connection::Serial(this);
        serialConnection->registerHandshake([&]() { return getDeviceInfo(); });
        registerConnection(serialConnection);
    }

    if (sqmConnection & CONNECTION_TCP)
    {
        tcpConnection = new Connection::TCP(this);
        tcpConnection->setDefaultHost("192.168.1.1");
        tcpConnection->setDefaultPort(10001);
        tcpConnection->registerHandshake([&]() { return getDeviceInfo(); });

        registerConnection(tcpConnection);
    }

    addDebugControl();

    return true;
}

bool SQM::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        // Already called by handshake
        //getDeviceInfo();

        defineNumber(&AverageReadingNP);
        defineNumber(&UnitInfoNP);
    }
    else
    {
        deleteProperty(AverageReadingNP.name);
        deleteProperty(UnitInfoNP.name);
    }

    return true;
}

bool SQM::getReadings()
{
    const char *cmd = "rx";
    char buffer[57]={0};

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s", cmd);

    ssize_t written = write(PortFD, cmd, 2);

    if (written < 2)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error getting device readings: %s", strerror(errno));
        return false;
    }

    ssize_t received = 0;

    while (received < 57)
    {
        ssize_t response = read(PortFD, buffer + received, 57 - received);
        if (response < 0)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error getting device readings: %s", strerror(errno));
            return false;
        }

        received += response;
    }

    if (received < 57)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error getting device readings");
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s", buffer);

    float mpsas, period_seconds, temperature;
    int frequency, period_counts;
    int rc =
        sscanf(buffer, "r,%fm,%dHz,%dc,%fs,%fC", &mpsas, &frequency, &period_counts, &period_seconds, &temperature);

    if (rc < 5)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Failed to parse input %s", buffer);
        return false;
    }

    AverageReadingN[0].value = mpsas;
    AverageReadingN[1].value = frequency;
    AverageReadingN[2].value = period_counts;
    AverageReadingN[3].value = period_seconds;
    AverageReadingN[4].value = temperature;

    return true;
}

const char *SQM::getDefaultName()
{
    return (const char *)"SQM";
}

bool SQM::getDeviceInfo()
{
    const char *cmd = "ix";
    char buffer[39]={0};

    if (getActiveConnection() == serialConnection) {
        PortFD = serialConnection->getPortFD();
    } else if (getActiveConnection() == tcpConnection) {
        PortFD = tcpConnection->getPortFD();
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %s", cmd);

    ssize_t written = write(PortFD, cmd, 2);

    if (written < 2)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error getting device info while writing to device: %s", strerror(errno));
        return false;
    }

    ssize_t received = 0;

    while (received < 39)
    {
        ssize_t response = read(PortFD, buffer + received, 39 - received);
        if (response < 0)
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error getting device info while reading response: %s", strerror(errno));
            return false;
        }

        received += response;
    }

    if (received < 39)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error getting device info");
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s", buffer);

    int protocol, model, feature, serial;
    int rc = sscanf(buffer, "i,%d,%d,%d,%d", &protocol, &model, &feature, &serial);

    if (rc < 4)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Failed to parse input %s", buffer);
        return false;
    }

    UnitInfoN[0].value = protocol;
    UnitInfoN[1].value = model;
    UnitInfoN[2].value = feature;
    UnitInfoN[3].value = serial;

    return true;
}

void SQM::TimerHit()
{
    if (!isConnected())
        return;

    bool rc = getReadings();

    AverageReadingNP.s = rc ? IPS_OK : IPS_ALERT;
    IDSetNumber(&AverageReadingNP, nullptr);

    SetTimer(POLLMS);
}
