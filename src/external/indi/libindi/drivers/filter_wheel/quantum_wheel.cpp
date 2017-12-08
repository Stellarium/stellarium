/*******************************************************************************
  Copyright(c) 2016 Radek Kaczorek  <rkaczorek AT gmail DOT com>

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

#include "quantum_wheel.h"

#include "connectionplugins/connectionserial.h"

#include <memory>
#include <cstring>
#include <unistd.h>

#define VERSION_MAJOR 0
#define VERSION_MINOR 2

std::unique_ptr<QFW> qfw(new QFW());

void ISGetProperties(const char *dev)
{
    qfw->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    qfw->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    qfw->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    qfw->ISNewNumber(dev, name, values, names, n);
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
    qfw->ISSnoopDevice(root);
}

QFW::QFW()
{
    setDeviceName(QFW::getDefaultName());
    setVersion(VERSION_MAJOR, VERSION_MINOR);
    setFilterConnection(CONNECTION_SERIAL | CONNECTION_TCP);
}

void QFW::debugTriggered(bool enable)
{
    INDI_UNUSED(enable);
}

void QFW::simulationTriggered(bool enable)
{
    INDI_UNUSED(enable);
}

const char *QFW::getDefaultName()
{
    return (const char *)"Quantum Wheel";
}

bool QFW::Handshake()
{
    if (isSimulation())
    {
        IDMessage(getDeviceName(), "Simulation: connected");
        PortFD = 1;
    }
    else
    {
        // check serial connection
        if (PortFD < 0 || isatty(PortFD) == 0)
        {
            IDMessage(getDeviceName(), "Device /dev/ttyACM0 is not available\n");
            return false;
        }
    }
    return true;
}

bool QFW::initProperties()
{
    INDI::FilterWheel::initProperties();
    // addDebugControl();
    addSimulationControl();

    serialConnection->setDefaultPort("/dev/ttyACM0");

    FilterSlotN[0].min = 1;
    FilterSlotN[0].max = 7;
    CurrentFilter      = 1;

    return true;
}

void QFW::ISGetProperties(const char *dev)
{
    INDI::FilterWheel::ISGetProperties(dev);
}

int QFW::QueryFilter()
{
    return CurrentFilter;
}

bool QFW::SelectFilter(int position)
{
    // count from 0 to 6 for positions 1 to 7
    position = position - 1;

    if (position < 0 || position > 6)
        return false;

    if (isSimulation())
    {
        CurrentFilter = position + 1;
        SelectFilterDone(CurrentFilter);
        return true;
    }
    // goto
    char targetpos[255]={0};
    char curpos[255]={0};
    int res;

    // format target position G[0-6]
    sprintf(targetpos, "G%d\r\n\n", position);

    // write command
    res = write(PortFD, targetpos, strlen(targetpos));

    // format target marker P[0-6]
    sprintf(targetpos, "P%d\r\n", position);

    // check current position
    do
    {
        usleep(100 * 1000);
        res         = read(PortFD, curpos, 255);
        curpos[res] = 0;
    } while (strncmp(targetpos, curpos, 2) != 0);

    // return current position to indi
    CurrentFilter = position + 1;
    SelectFilterDone(CurrentFilter);

    return true;
}
