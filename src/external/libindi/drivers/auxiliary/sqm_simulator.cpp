/*******************************************************************************
  Copyright(c) 2017 Ralph Rogge. All rights reserved.

  INDI Sky Quality Meter Simulator

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

#include "sqm_simulator.h"

#include <cerrno>
#include <memory>
#include <cstring>
#include <unistd.h>

std::unique_ptr<SQMSimulator> sqmSimulator(new SQMSimulator());

#define UNIT_TAB "Unit"

enum {
    READING_BRIGHTNESS_INDEX,
    READING_FREQUENCY_INDEX,
    READING_COUNTER_INDEX,
    READING_TIME_INDEX,
    READING_TEMPERATURE_INDEX
};

enum {
    UNIT_PROTOCOL_INDEX,
    UNIT_MODEL_INDEX,
    UNIT_FEATURE_INDEX,
    UNIT_SERIAL_INDEX
};

void ISGetProperties(const char *dev)
{
    sqmSimulator->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    sqmSimulator->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    sqmSimulator->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    sqmSimulator->ISNewNumber(dev, name, values, names, n);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int n)
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
    sqmSimulator->ISSnoopDevice(root);
}

SQMSimulator::SQMSimulator()
{
    setVersion(1, 0);
}

bool SQMSimulator::Connect()
{
  readingProperties.s = getReading() ? IPS_OK : IPS_ALERT;
  IDSetNumber(&readingProperties, nullptr);

  unitProperties.s = getUnit() ? IPS_OK : IPS_ALERT;
  IDSetNumber(&unitProperties, nullptr);

  return true;
}

bool SQMSimulator::Disconnect()
{
    return true;
}

bool SQMSimulator::initProperties()
{
    INDI::DefaultDevice::initProperties();

    // Readings
    IUFillNumber(&readingValues[READING_BRIGHTNESS_INDEX], "SKY_BRIGHTNESS", "Quality (mag/arcsec^2)", "%6.2f", -20, 30, 0, 0);
    IUFillNumber(&readingValues[READING_FREQUENCY_INDEX], "SENSOR_FREQUENCY", "Freq (Hz)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&readingValues[READING_COUNTER_INDEX], "SENSOR_COUNTS", "Period (counts)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&readingValues[READING_TIME_INDEX], "SENSOR_PERIOD", "Period (s)", "%6.2f", 0, 1000000, 0, 0);
    IUFillNumber(&readingValues[READING_TEMPERATURE_INDEX], "SKY_TEMPERATURE", "Temperature (C)", "%6.2f", -50, 80, 0, 0);
    IUFillNumberVector(&readingProperties, readingValues, READING_NUMBER_OF_VALUES, getDeviceName(), "SKY_QUALITY", "Readings", MAIN_CONTROL_TAB, IP_RW, 0, IPS_IDLE);

    // Unit Info
    IUFillNumber(&unitValues[UNIT_PROTOCOL_INDEX], "Protocol", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&unitValues[UNIT_MODEL_INDEX], "Model", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&unitValues[UNIT_FEATURE_INDEX], "Feature", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumber(&unitValues[UNIT_SERIAL_INDEX], "Serial", "", "%.f", 0, 1000000, 0, 0);
    IUFillNumberVector(&unitProperties, unitValues, UNIT_NUMBER_OF_VALUES, getDeviceName(), "Unit Info", "", UNIT_TAB, IP_RW, 0, IPS_IDLE);

    addDebugControl();

    return true;
}

const char * SQMSimulator::getDefaultName()
{
    return (const char *) "SQM Simulator";
}

bool SQMSimulator::getReading()
{
    readingValues[READING_BRIGHTNESS_INDEX].value = 16.9;
    readingValues[READING_FREQUENCY_INDEX].value = 15.0;
    readingValues[READING_COUNTER_INDEX].value = 28856;
    readingValues[READING_TIME_INDEX].value = 0.063;
    readingValues[READING_TEMPERATURE_INDEX].value = 21.5;

    return true;
}

bool SQMSimulator::getUnit()
{
    unitValues[UNIT_PROTOCOL_INDEX].value = 4;
    unitValues[UNIT_MODEL_INDEX].value = 3;
    unitValues[UNIT_FEATURE_INDEX].value = 49;
    unitValues[UNIT_SERIAL_INDEX].value = 1234;

    return true;
}

bool SQMSimulator::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0) {
        if (strcmp(name, readingProperties.name) == 0) {
            IUUpdateNumber(&readingProperties, values, names, n);
            readingProperties.s = IPS_OK;
            IDSetNumber(&readingProperties, nullptr);
            return true;
        }

        if (strcmp(name, unitProperties.name) == 0) {
            IUUpdateNumber(&unitProperties, values, names, n);
            unitProperties.s = IPS_OK;
            IDSetNumber(&unitProperties, nullptr);
            return true;
        }
    }

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool SQMSimulator::updateProperties()
{
    INDI::DefaultDevice::updateProperties();

    if (isConnected()) {
        defineNumber(&readingProperties);
        defineNumber(&unitProperties);
    } else {
        deleteProperty(readingProperties.name);
        deleteProperty(unitProperties.name);
    }

    return true;
}
