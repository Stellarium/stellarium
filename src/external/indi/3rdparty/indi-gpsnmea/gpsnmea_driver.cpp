/*******************************************************************************
  Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

  INDI GPS NMEA Driver

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

#include "gpsnmea_driver.h"
#include "minmea.h"

#include "config.h"

#include <connectionplugins/connectiontcp.h>
#include <indicom.h>
#include <libnova/julian_day.h>
#include <libnova/sidereal_time.h>

#include <memory>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#define MAX_NMEA_PARSES     50              // Read 50 streams before giving up
#define MAX_TIMEOUT_COUNT   5               // Maximum timeout before auto-connect

// We declare an auto pointer to GPSD.
std::unique_ptr<GPSNMEA> gpsnema(new GPSNMEA());

void ISGetProperties(const char *dev)
{
    gpsnema->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    gpsnema->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    gpsnema->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    gpsnema->ISNewNumber(dev, name, values, names, num);
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
    INDI_UNUSED(root);
}

GPSNMEA::GPSNMEA()
{
    setVersion(GPSNMEA_VERSION_MAJOR, GPSNMEA_VERSION_MINOR);
}

const char *GPSNMEA::getDefaultName()
{
    return (char *)"GPS NMEA";
}

bool GPSNMEA::initProperties()
{
    // We init parent properties first
    INDI::GPS::initProperties();

    IUFillText(&GPSstatusT[0], "GPS_FIX", "Fix Mode", NULL);
    IUFillTextVector(&GPSstatusTP, GPSstatusT, 1, getDeviceName(), "GPS_STATUS", "GPS Status", MAIN_CONTROL_TAB, IP_RO,
                     60, IPS_IDLE);

    tcpConnection = new Connection::TCP(this);
    tcpConnection->setDefaultHost("192.168.1.1");
    tcpConnection->setDefaultPort(50000);
    tcpConnection->registerHandshake([&]()
    {
        PortFD = tcpConnection->getPortFD();
        return isNMEA();
    });

    registerConnection(tcpConnection);

    addDebugControl();

    setDriverInterface(AUX_INTERFACE);

    return true;
}

bool GPSNMEA::updateProperties()
{
    // Call parent update properties first
    INDI::GPS::updateProperties();

    if (isConnected())
    {
        defineText(&GPSstatusTP);

        pthread_create(&nmeaThread, nullptr, &GPSNMEA::parseNMEAHelper, this);
    }
    else
    {
        // We're disconnected
        deleteProperty(GPSstatusTP.name);
    }
    return true;
}

IPState GPSNMEA::updateGPS()
{
    IPState rc = IPS_BUSY;

    pthread_mutex_lock(&lock);
    if (locationPending == false && timePending == false)
    {
        rc = IPS_OK;
        locationPending = true;
        timePending = true;
    }
    pthread_mutex_unlock(&lock);

    return rc;
}

bool GPSNMEA::isNMEA()
{
    char line[MINMEA_MAX_LENGTH];

    int bytes_read=0;
    int tty_rc = tty_nread_section(PortFD, line, MINMEA_MAX_LENGTH, 0xA, 3, &bytes_read);
    if (tty_rc < 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error getting device readings: %s", strerror(errno));
        return false;
    }
    line[bytes_read] = '\0';

    return (minmea_sentence_id(line, false) != MINMEA_INVALID);
}

void* GPSNMEA::parseNMEAHelper(void *obj)
{
    static_cast<GPSNMEA*>(obj)->parseNEMA();
    return nullptr;
}

void GPSNMEA::parseNEMA()
{
    static char ts[32]={0};

    char line[MINMEA_MAX_LENGTH];

    while (isConnected())
    {
        int bytes_read=0;
        int tty_rc = tty_nread_section(PortFD, line, MINMEA_MAX_LENGTH, 0xA, 3, &bytes_read);
        if (tty_rc < 0)
        {
            if (tty_rc == TTY_OVERFLOW)
                continue;
            else
            {
                char errmsg[MAXRBUF];
                tty_error_msg(tty_rc, errmsg, MAXRBUF);
                if (tty_rc == TTY_TIME_OUT || errno == ECONNREFUSED)
                {
                    if (errno == ECONNREFUSED)
                    {
                        // sleep for 10 seconds
                        tcpConnection->Disconnect();
                        usleep(10*1e6);
                        tcpConnection->Connect();
                        PortFD = tcpConnection->getPortFD();
                    }
                    else if (timeoutCounter++ > MAX_TIMEOUT_COUNT)
                    {
                        DEBUG(INDI::Logger::DBG_WARNING, "Timeout limit reached, reconnecting...");

                        tcpConnection->Disconnect();
                        // sleep for 5 seconds
                        usleep(5*1e6);
                        tcpConnection->Connect();
                        PortFD = tcpConnection->getPortFD();
                        timeoutCounter=0;
                    }
                }
                continue;
            }
        }
        line[bytes_read] = '\0';

        DEBUGF(INDI::Logger::DBG_DEBUG, "%s", line);
        switch (minmea_sentence_id(line, false))
        {
        case MINMEA_SENTENCE_RMC:
        {
            struct minmea_sentence_rmc frame;
            if (minmea_parse_rmc(&frame, line))
            {
                if (frame.valid)
                {
                    LocationN[LOCATION_LATITUDE].value  = minmea_tocoord(&frame.latitude);
                    LocationN[LOCATION_LONGITUDE].value = minmea_tocoord(&frame.longitude);
                    if (LocationN[LOCATION_LONGITUDE].value < 0)
                        LocationN[LOCATION_LONGITUDE].value += 360;

                    struct timespec timesp;
                    time_t raw_time;
                    struct tm *utc, *local;

                    if (minmea_gettime(&timesp, &frame.date, &frame.time) == -1)
                        break;

                    raw_time = timesp.tv_sec;
                    utc = gmtime(&raw_time);
                    strftime(ts, 32, "%Y-%m-%dT%H:%M:%S", utc);
                    IUSaveText(&TimeT[0], ts);

                    local = localtime(&raw_time);
                    snprintf(ts, 32, "%4.2f", (local->tm_gmtoff / 3600.0));
                    IUSaveText(&TimeT[1], ts);

                    pthread_mutex_lock(&lock);
                    locationPending = false;
                    timePending = false;
                    DEBUG(INDI::Logger::DBG_DEBUG, "Threaded Location and Time updates complete.");
                    pthread_mutex_unlock(&lock);
                }
            }
            else
            {
                DEBUG(INDI::Logger::DBG_DEBUG, "$xxRMC sentence is not parsed");
            }
        }
        break;

        case MINMEA_SENTENCE_GGA:
        {
            struct minmea_sentence_gga frame;
            if (minmea_parse_gga(&frame, line))
            {
                if (frame.fix_quality == 1)
                {
                    LocationN[LOCATION_LATITUDE].value  = minmea_tocoord(&frame.latitude);
                    LocationN[LOCATION_LONGITUDE].value = minmea_tocoord(&frame.longitude);
                    if (LocationN[LOCATION_LONGITUDE].value < 0)
                        LocationN[LOCATION_LONGITUDE].value += 360;

                    LocationN[LOCATION_ELEVATION].value = minmea_tofloat(&frame.altitude);

                    struct timespec timesp;
                    time_t raw_time;
                    struct tm *utc, *local;
                    minmea_date gmt_date;

                    time(&raw_time);
                    utc = gmtime(&raw_time);
                    gmt_date.day = utc->tm_mday;
                    gmt_date.month = utc->tm_mon+1;
                    gmt_date.year = utc->tm_year;

                    minmea_gettime(&timesp, &gmt_date, &frame.time);

                    raw_time = timesp.tv_sec;
                    utc = gmtime(&raw_time);
                    strftime(ts, 32, "%Y-%m-%dT%H:%M:%S", utc);
                    IUSaveText(&TimeT[0], ts);

                    local = localtime(&raw_time);
                    snprintf(ts, 32, "%4.2f", (local->tm_gmtoff / 3600.0));
                    IUSaveText(&TimeT[1], ts);

                    pthread_mutex_lock(&lock);
                    timePending = false;
                    locationPending = false;
                    DEBUG(INDI::Logger::DBG_DEBUG, "Threaded Location and Time updates complete.");
                    pthread_mutex_unlock(&lock);
                }
            }
            else
            {
                DEBUG(INDI::Logger::DBG_DEBUG, "$xxGGA sentence is not parsed");
            }
        } break;

        case MINMEA_SENTENCE_GSA:
        {
            struct minmea_sentence_gsa frame;
            if (minmea_parse_gsa(&frame, line))
            {
                if (frame.fix_type == 1)
                {
                    GPSstatusTP.s = IPS_BUSY;
                    IUSaveText(&GPSstatusT[0], "NO FIX");
                }
                else if (frame.fix_type == 2)
                {
                    GPSstatusTP.s = IPS_OK;
                    IUSaveText(&GPSstatusT[0], "2D FIX");
                }
                else if (frame.fix_type == 3)
                {
                    GPSstatusTP.s = IPS_OK;
                    IUSaveText(&GPSstatusT[0], "3D FIX");
                }
                IDSetText(&GPSstatusTP, nullptr);

            }
            else
            {
              DEBUG(INDI::Logger::DBG_DEBUG, "$xxGSA sentence is not parsed.");
            }
        }
            break;
        case MINMEA_SENTENCE_ZDA:
        {
            struct minmea_sentence_zda frame;
            if (minmea_parse_zda(&frame, line))
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "$xxZDA: %d:%d:%d %02d.%02d.%d UTC%+03d:%02d",
                       frame.time.hours,
                       frame.time.minutes,
                       frame.time.seconds,
                       frame.date.day,
                       frame.date.month,
                       frame.date.year,
                       frame.hour_offset,
                       frame.minute_offset);

                struct timespec timesp;
                time_t raw_time;
                struct tm *utc, *local;

                minmea_gettime(&timesp, &frame.date, &frame.time);

                raw_time = timesp.tv_sec;
                utc = gmtime(&raw_time);
                strftime(ts, 32, "%Y-%m-%dT%H:%M:%S", utc);
                IUSaveText(&TimeT[0], ts);

                local = localtime(&raw_time);
                snprintf(ts, 32, "%4.2f", (local->tm_gmtoff / 3600.0));
                IUSaveText(&TimeT[1], ts);

                pthread_mutex_lock(&lock);
                timePending = false;
                DEBUG(INDI::Logger::DBG_DEBUG, "Threaded Time update complete.");
                pthread_mutex_unlock(&lock);
            }
            else {
                DEBUG(INDI::Logger::DBG_DEBUG, "$xxZDA sentence is not parsed");
            }
        } break;

        case MINMEA_INVALID:
        {
            //DEBUG(INDI::Logger::DBG_WARNING, "$xxxxx sentence is not valid");
        } break;

        default:
        {
            DEBUG(INDI::Logger::DBG_DEBUG, "$xxxxx sentence is not parsed");
        }
            break;
        }
    }

    pthread_exit(nullptr);
}
