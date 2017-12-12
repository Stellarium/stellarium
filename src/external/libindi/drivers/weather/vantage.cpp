/*******************************************************************************
  Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

  INDI Davis Vantage Pro/Pro2/Vue Weather Driver

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

#include "vantage.h"

#include "indicom.h"
#include "connectionplugins/connectionserial.h"

#include <memory>
#include <cstring>
#include <termios.h>

#define VANTAGE_CMD     8
#define VANTAGE_RES     128
#define VANTAGE_TIMEOUT 2

static uint16_t crc_table[] = {
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
    0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a,
    0xd3bd, 0xc39c, 0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485, 0xa56a, 0xb54b,
    0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
    0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96,
    0x1a71, 0x0a50, 0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a, 0x6ca6, 0x7c87,
    0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
    0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3,
    0x5004, 0x4025, 0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e, 0x02b1, 0x1290,
    0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
    0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f,
    0x99c8, 0x89e9, 0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3, 0xcb7d, 0xdb5c,
    0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
    0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74,
    0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

uint16_t crc16(const void *c_ptr, size_t len)
{
    const uint8_t *c = (uint8_t *)c_ptr;
    uint16_t crc     = 0;

    while (len--)
        crc = crc_table[((crc >> 8) ^ *c++)] ^ (crc << 8);

    return crc;
}

// We declare an auto pointer to Vantage.
std::unique_ptr<Vantage> vantage(new Vantage());

void ISGetProperties(const char *dev)
{
    vantage->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    vantage->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    vantage->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    vantage->ISNewNumber(dev, name, values, names, n);
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
    vantage->ISSnoopDevice(root);
}

Vantage::Vantage()
{
    setVersion(1, 0);
}

const char *Vantage::getDefaultName()
{
    return (const char *)"Vantage";
}

bool Vantage::initProperties()
{
    INDI::Weather::initProperties();

    addParameter("WEATHER_FORECAST", "Forecast", 0, 0, 0, 1);
    addParameter("WEATHER_TEMPERATURE", "Temperature (C)", -10, 30, -20, 40);
    addParameter("WEATHER_BAROMETER", "Barometer (mbar)", 20, 32.5, 20, 32.5);
    addParameter("WEATHER_WIND_SPEED", "Wind (kph)", 0, 20, 0, 40);
    addParameter("WEAHTER_WIND_DIRECTION", "Wind Direction", 0, 360, 0, 360);
    addParameter("WEATHER_HUMIDITY", "Humidity %", 0, 100, 0, 100);
    addParameter("WEATHER_RAIN_RATE", "Rain (mm/h)", 0, 0, 0, 0);
    addParameter("WEATHER_SOLAR_RADIATION", "Solar Radiation (w/m^2)", 0, 10000, 0, 10000);

    setCriticalParameter("WEATHER_FORECAST");
    setCriticalParameter("WEATHER_TEMPERATURE");
    setCriticalParameter("WEATHER_WIND_SPEED");
    setCriticalParameter("WEATHER_RAIN_RATE");

    addDebugControl();

    serialConnection->setDefaultBaudRate(Connection::Serial::B_19200);

    return true;
}

bool Vantage::Handshake()
{
    return ack();
}

IPState Vantage::updateWeather()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[VANTAGE_CMD];
    char response[VANTAGE_RES];

    if (!wakeup())
        return IPS_ALERT;

    strncpy(command, "LOOP 1", VANTAGE_CMD);
    command[6] = 0;

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[6] = 0xA;

    if ((rc = tty_write(PortFD, command, 7, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Loop error: %s.", errstr);
        return IPS_ALERT;
    }

    if ((rc = tty_read(PortFD, response, 1, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Loop error: %s.", errstr);
        return IPS_ALERT;
    }

    if (response[0] != 0x06)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Expecting 0x06, received %#X", response[0]);
        return IPS_ALERT;
    }

    if ((rc = tty_read(PortFD, response, 99, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Loop error: %s.", errstr);
        return IPS_ALERT;
    }

    uint16_t crc = crc16(response, 99);

    if (crc != 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "CRC check failed.");
        return IPS_ALERT;
    }

    uint8_t *loopData = (uint8_t *)response;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Packet Type (%d)", loopData[4]);

    uint8_t forecastValue = loopData[89];

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Forecast (%d)", forecastValue);

    switch (forecastValue)
    {
        // Clear
        case 0x08:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Mostly Clear.");
            setParameterValue("WEATHER_FORECAST", 0);
            break;

        case 0x06:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Partly Cloudy.");
            setParameterValue("WEATHER_FORECAST", 1);
            break;

        case 0x02:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Mostly Cloudy.");
            setParameterValue("WEATHER_FORECAST", 2);
            break;

        case 0x03:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Mostly Cloudy. Rain within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 2);
            break;

        case 0x12:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Mostly Cloudy. Snow within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 2);
            break;

        case 0x13:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Mostly Cloudy. Rain or Snow within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 2);
            break;

        case 0x07:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Partly Cloudy. Rain within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 1);
            break;

        case 0x16:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Partly Cloudy. Snow within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 1);
            break;

        case 0x17:
            DEBUG(INDI::Logger::DBG_SESSION, "Forecast: Partly Cloudy. Rain or Snow within 12 hours.");
            setParameterValue("WEATHER_FORECAST", 1);
            break;
    }

    // Inside Temperature
    uint16_t temperatureValue = loopData[10] << 8 | loopData[9];

    setParameterValue("WEATHER_TEMPERATURE", ((temperatureValue / 10.0) - 32) / 1.8);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Temperature (%d) [%#4X %#4X]", temperatureValue, loopData[9], loopData[10]);

    // Barometer
    uint16_t barometerValue = loopData[8] << 8 | loopData[7];

    setParameterValue("WEATHER_BAROMETER", (barometerValue / 1000.0) * 33.8639);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Barometer (%d) [%#4X %#4X]", barometerValue, loopData[7], loopData[8]);

    // Wind Speed
    uint8_t windValue = loopData[14];

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Wind Speed (%d) [%#X4]", windValue, loopData[14]);

    setParameterValue("WEATHER_WIND_SPEED", windValue / 0.62137);

    // Wind Direction
    uint16_t windDir = loopData[17] << 8 | loopData[16];

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Wind Direction (%d) [%#4X,%#4X]", windDir, loopData[16], loopData[17]);

    setParameterValue("WEATHER_WIND_DIRECTION", windDir);

    // Rain Rate
    uint16_t rainRate = loopData[42] << 8 | loopData[41];

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Rain Rate (%d) [%#4X,%#4X]", rainRate, loopData[41], loopData[42]);

    setParameterValue("WEATHER_RAIN_RATE", rainRate / (100 * 0.039370));

    // Solar Radiation
    uint16_t solarRadiation = loopData[45] << 8 | loopData[44];

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Solar Radiation (%d) [%#4X,%#4X]", solarRadiation, loopData[44], loopData[45]);

    if (solarRadiation == 32767)
        solarRadiation = 0;

    setParameterValue("WEATHER_SOLAR_RADIATION", solarRadiation);

    return IPS_OK;
}

bool Vantage::wakeup()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[VANTAGE_CMD];
    char response[VANTAGE_RES];

    tcflush(PortFD, TCIOFLUSH);

    command[0] = 0xA;

    for (int i = 0; i < 3; i++)
    {
        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%#X)", command[0]);

        if ((rc = tty_write(PortFD, command, 1, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(rc, errstr, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "Wakup error: %s.", errstr);
            return false;
        }

        if ((rc = tty_read_section(PortFD, response, 0xD, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
            continue;
        else
        {
            if (nbytes_read == 2)
            {
                DEBUG(INDI::Logger::DBG_DEBUG, "Console is awake.");
                return true;
            }
        }
    }

    return false;
}

bool Vantage::ack()
{
    int nbytes_written = 0, nbytes_read = 0, rc = -1;
    char errstr[MAXRBUF];
    char command[VANTAGE_CMD];
    char response[VANTAGE_RES];

    if (!wakeup())
        return false;

    command[0] = 'V';
    command[1] = 'E';
    command[2] = 'R';
    command[3] = 0;

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", command);

    command[3] = 0xA;
    command[4] = 0;

    if ((rc = tty_write(PortFD, command, 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xD, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    if (response[1] != 0xD)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Expecting 0xD, received %#X", response[1]);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xD, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    response[nbytes_read - 2] = 0;

    if (strcmp(response, "OK") != 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error response: %s", response);
        return false;
    }

    if ((rc = tty_read_section(PortFD, response, 0xD, VANTAGE_TIMEOUT, &nbytes_read)) != TTY_OK)
    {
        tty_error_msg(rc, errstr, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "Ack error: %s.", errstr);
        return false;
    }

    response[nbytes_read - 2] = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

    return true;
}
