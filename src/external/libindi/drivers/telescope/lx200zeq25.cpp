/*
    ZEQ25 INDI driver

    Copyright (C) 2015 Jasem Mutlaq

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

#include "lx200zeq25.h"

#include "indicom.h"
#include "lx200driver.h"

#include <libnova/transform.h>

#include <cmath>
#include <cstring>
#include <termios.h>
#include <unistd.h>

/* Simulation Parameters */
#define SLEWRATE 1        /* slew rate, degrees/s */
#define SIDRATE  0.004178 /* sidereal rate, degrees/s */

LX200ZEQ25::LX200ZEQ25()
{
    setVersion(1, 0);

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_TRACK_MODE,
                           9);
}

bool LX200ZEQ25::initProperties()
{
    LX200Generic::initProperties();

    SetParkDataType(PARK_AZ_ALT);

    strcpy(SlewRateS[0].label, "1x");
    strcpy(SlewRateS[1].label, "2x");
    strcpy(SlewRateS[2].label, "8x");
    strcpy(SlewRateS[3].label, "16x");
    strcpy(SlewRateS[4].label, "64x");
    strcpy(SlewRateS[5].label, "128x");
    strcpy(SlewRateS[6].label, "256x");
    strcpy(SlewRateS[7].label, "512x");
    strcpy(SlewRateS[8].label, "MAX");

    IUFillSwitch(&HomeS[0], "Home", "", ISS_OFF);
    IUFillSwitchVector(&HomeSP, HomeS, 1, getDeviceName(), "Home", "Home", MAIN_CONTROL_TAB, IP_RW, ISR_ATMOST1, 0,
                       IPS_IDLE);

    /* How fast do we guide compared to sidereal rate */
    IUFillNumber(&GuideRateN[0], "GUIDE_RATE", "x Sidereal", "%g", 0.1, 0.9, 0.1, 0.5);
    IUFillNumberVector(&GuideRateNP, GuideRateN, 1, getDeviceName(), "GUIDE_RATE", "Guiding Rate", MOTION_TAB, IP_RW, 0,
                       IPS_IDLE);

    return true;
}

bool LX200ZEQ25::updateProperties()
{
    LX200Generic::updateProperties();

    if (isConnected())
    {
        // Delete unsupported properties
        deleteProperty(AlignmentSP.name);
        deleteProperty(SiteSP.name);
        deleteProperty(TrackingFreqNP.name);
        deleteProperty(SiteNameTP.name);

        defineSwitch(&HomeSP);
        defineNumber(&GuideRateNP);
    }
    else
    {
        deleteProperty(HomeSP.name);
        deleteProperty(GuideRateNP.name);
    }

    return true;
}

const char *LX200ZEQ25::getDefaultName()
{
    return (const char *)"ZEQ25";
}

bool LX200ZEQ25::checkConnection()
{
    if (isSimulation())
        return true;

    char initCMD[] = ":V#";
    int errcode    = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUG(INDI::Logger::DBG_DEBUG, "Initializing IOptron using :V# CMD...");

    for (int i = 0; i < 2; i++)
    {
        if ((errcode = tty_write(PortFD, initCMD, 3, &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            usleep(50000);
            continue;
        }

        if ((errcode = tty_read_section(PortFD, response, '#', 3, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            usleep(50000);
            continue;
        }

        if (nbytes_read > 0)
        {
            response[nbytes_read] = '\0';
            DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

            if (!strcmp(response, "V1.00#"))
                return true;
        }

        usleep(50000);
    }

    return false;
}

bool LX200ZEQ25::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(HomeSP.name, name) == 0)
        {
            // If already home, nothing to be done
            //if (HomeS[0].s == ISS_ON)
            if (isZEQ25Home())
            {
                DEBUG(INDI::Logger::DBG_WARNING, "Telescope is already homed.");
                HomeS[0].s = ISS_ON;
                HomeSP.s   = IPS_OK;
                IDSetSwitch(&HomeSP, nullptr);
                return true;
            }

            if (gotoZEQ25Home() < 0)
            {
                HomeSP.s = IPS_ALERT;
                DEBUG(INDI::Logger::DBG_ERROR, "Error slewing to home position.");
            }
            else
            {
                HomeSP.s = IPS_BUSY;
                DEBUG(INDI::Logger::DBG_SESSION, "Slewing to home position.");
            }

            IDSetSwitch(&HomeSP, nullptr);
            return true;
        }
    }

    return LX200Generic::ISNewSwitch(dev, name, states, names, n);
}

bool LX200ZEQ25::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Guiding Rate
        if (!strcmp(name, GuideRateNP.name))
        {
            IUUpdateNumber(&GuideRateNP, values, names, n);

            if (setZEQ25GuideRate(GuideRateN[0].value) == TTY_OK)
                GuideRateNP.s = IPS_OK;
            else
                GuideRateNP.s = IPS_ALERT;

            IDSetNumber(&GuideRateNP, nullptr);

            return true;
        }
    }

    return LX200Generic::ISNewNumber(dev, name, values, names, n);
}

bool LX200ZEQ25::isZEQ25Home()
{
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    if (isSimulation())
        return true;

    DEBUG(DBG_SCOPE, "CMD <:AH#>");

    if ((error_type = tty_write_string(PortFD, ":AH#", &nbytes_write)) != TTY_OK)
        return false;

    error_type = tty_read(PortFD, bool_return, 1, 5, &nbytes_read);

    // JM: Hack from Jon in the INDI forums to fix longitude/latitude settings failure on ZEQ25
    usleep(10000);
    tcflush(PortFD, TCIFLUSH);
    usleep(10000);

    if (nbytes_read < 1)
        return false;

    DEBUGF(DBG_SCOPE, "RES <%c>", bool_return[0]);

    return (bool_return[0] == '1');
}

int LX200ZEQ25::gotoZEQ25Home()
{
    return setZEQ25StandardProcedure(PortFD, ":MH#");
}

bool LX200ZEQ25::isSlewComplete()
{    
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    //strncpy(cmd, ":SE#", 16);
    const char *cmd = ":SE#";

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, 4, &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if ((errcode = tty_read(PortFD, response, 1, 3, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(PortFD, TCIFLUSH);

        if (response[0] == '0')
            return true;
        else
            return false;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool LX200ZEQ25::getMountInfo()
{
    char cmd[]  = ":MountInfo#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[16];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if ((errcode = tty_read(PortFD, response, 4, 3, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        if (nbytes_read == 4)
        {
            if (!strcmp(response, "8407"))
                DEBUG(INDI::Logger::DBG_SESSION, "Detected iEQ45/iEQ30 Mount.");
            else if (!strcmp(response, "8497"))
                DEBUG(INDI::Logger::DBG_SESSION, "Detected iEQ45 AA Mount.");
            else if (!strcmp(response, "8408"))
                DEBUG(INDI::Logger::DBG_SESSION, "Detected ZEQ25 Mount.");
            else if (!strcmp(response, "8498"))
                DEBUG(INDI::Logger::DBG_SESSION, "Detected SmartEQ Mount.");
            else
                DEBUG(INDI::Logger::DBG_SESSION, "Unknown mount detected.");

            tcflush(PortFD, TCIFLUSH);

            return true;
        }
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 4.", nbytes_read);
    return false;
}

void LX200ZEQ25::getBasicData()
{
    getMountInfo();

    int moveRate = getZEQ25MoveRate();
    if (moveRate >= 0)
    {
        IUResetSwitch(&SlewRateSP);
        SlewRateS[moveRate].s = ISS_ON;
        SlewRateSP.s          = IPS_OK;
        IDSetSwitch(&SlewRateSP, nullptr);
    }

    if (InitPark())
    {
        // If loading parking data is successful, we just set the default parking values.
        SetAxis1ParkDefault(LocationN[LOCATION_LATITUDE].value >= 0 ? 0 : 180);
        SetAxis2ParkDefault(LocationN[LOCATION_LATITUDE].value);
    }
    else
    {
        // Otherwise, we set all parking data to default in case no parking data is found.
        SetAxis1Park(LocationN[LOCATION_LATITUDE].value >= 0 ? 0 : 180);
        SetAxis2Park(LocationN[LOCATION_LATITUDE].value);
        SetAxis1ParkDefault(LocationN[LOCATION_LATITUDE].value >= 0 ? 0 : 180);
        SetAxis2ParkDefault(LocationN[LOCATION_LATITUDE].value);
    }

    bool isMountParked = isZEQ25Parked();
    if (isMountParked != isParked())
        SetParked(isMountParked);

    // Is home?
    DEBUG(INDI::Logger::DBG_DEBUG, "Checking if mount is at home position...");
    if (isZEQ25Home())
    {
        HomeS[0].s = ISS_ON;
        HomeSP.s   = IPS_OK;
        IDSetSwitch(&HomeSP, nullptr);
    }

    DEBUG(INDI::Logger::DBG_DEBUG, "Getting guiding rate...");
    double guideRate = 0;
    if (getZEQ25GuideRate(&guideRate) == TTY_OK)
    {
        GuideRateN[0].value = guideRate;
        IDSetNumber(&GuideRateNP, nullptr);
    }
}

bool LX200ZEQ25::Goto(double r, double d)
{
    targetRA  = r;
    targetDEC = d;
    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, targetRA, 2, 3600);
    fs_sexa(DecStr, targetDEC, 2, 3600);

    // If moving, let's stop it first.
    if (EqNP.s == IPS_BUSY)
    {
        if (!isSimulation() && abortSlew(PortFD) < 0)
        {
            AbortSP.s = IPS_ALERT;
            IDSetSwitch(&AbortSP, "Abort slew failed.");
            return false;
        }

        AbortSP.s = IPS_OK;
        EqNP.s    = IPS_IDLE;
        IDSetSwitch(&AbortSP, "Slew aborted.");
        IDSetNumber(&EqNP, nullptr);

        if (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY)
        {
            MovementNSSP.s = MovementWESP.s = IPS_IDLE;
            EqNP.s                          = IPS_IDLE;
            IUResetSwitch(&MovementNSSP);
            IUResetSwitch(&MovementWESP);
            IDSetSwitch(&MovementNSSP, nullptr);
            IDSetSwitch(&MovementWESP, nullptr);
        }

        // sleep for 100 mseconds
        usleep(100000);
    }

    if (!isSimulation())
    {
        if (setObjectRA(PortFD, targetRA) < 0 || (setObjectDEC(PortFD, targetDEC)) < 0)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error setting RA/DEC.");
            return false;
        }

        if (slewZEQ25() == 0)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error Slewing to JNow RA %s - DEC %s\n", RAStr, DecStr);
            slewError(1);
            return false;
        }
    }

    TrackState = SCOPE_SLEWING;
    EqNP.s     = IPS_BUSY;

    DEBUGF(INDI::Logger::DBG_SESSION, "Slewing to RA: %s - DEC: %s", RAStr, DecStr);
    return true;
}

int LX200ZEQ25::slewZEQ25()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    char slewNum[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGF(DBG_SCOPE, "CMD <%s>", ":MS#");

    if ((error_type = tty_write_string(PortFD, ":MS#", &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(PortFD, slewNum, 1, 3, &nbytes_read);

    if (nbytes_read < 1)
    {
        DEBUGF(DBG_SCOPE, "RES ERROR <%d>", error_type);
        return error_type;
    }

    /* We don't need to read the string message, just return corresponding error code */
    tcflush(PortFD, TCIFLUSH);

    DEBUGF(DBG_SCOPE, "RES <%c>", slewNum[0]);

    return slewNum[0];
}

bool LX200ZEQ25::SetSlewRate(int index)
{
    if (isSimulation())
        return true;

    char cmd[8];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[2];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    snprintf(cmd, 8, ":SR%d#", index + 1);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if ((errcode = tty_read(PortFD, response, 1, 3, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(PortFD, TCIFLUSH);

        return (response[0] == '1');
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

int LX200ZEQ25::getZEQ25MoveRate()
{
    if (isSimulation())
    {
        return IUFindOnSwitchIndex(&SlewRateSP);
    }

    char cmd[]  = ":Gr#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[3];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return -1;
    }

    if ((errcode = tty_read_section(PortFD, response, '#', 3, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return -1;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(PortFD, TCIFLUSH);

        int moveRate = -1;

        sscanf(response, "%d", &moveRate);

        return moveRate;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 2.", nbytes_read);
    return -1;
}

bool LX200ZEQ25::updateTime(ln_date *utc, double utc_offset)
{
    struct ln_zonedate ltm;

    if (isSimulation())
        return true;

    ln_date_to_zonedate(utc, &ltm, utc_offset * 3600.0);

    JD = ln_get_julian_day(utc);

    DEBUGF(INDI::Logger::DBG_DEBUG, "New JD is %f", (float)JD);

    // Set Local Time
    if (setLocalTime(PortFD, ltm.hours, ltm.minutes, ltm.seconds) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting local time.");
        return false;
    }

    if (setCalenderDate(PortFD, ltm.days, ltm.months, ltm.years) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting local date.");
        return false;
    }

    if (setZEQ25UTCOffset(utc_offset) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC Offset.");
        return false;
    }

    return true;
}

bool LX200ZEQ25::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    if (isSimulation())
        return true;

    double final_longitude;

    if (longitude > 180)
        final_longitude = longitude - 360.0;
    else
        final_longitude = longitude;

    if (!isSimulation() && setZEQ25Longitude(final_longitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site longitude coordinates");
        return false;
    }

    if (!isSimulation() && setZEQ25Latitude(latitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site latitude coordinates");
        return false;
    }

    char l[32], L[32];
    fs_sexa(l, latitude, 3, 3600);
    fs_sexa(L, longitude, 4, 3600);

    DEBUGF(INDI::Logger::DBG_SESSION, "Site location updated to Lat %.32s - Long %.32s", l, L);

    return true;
}

int LX200ZEQ25::setZEQ25Longitude(double Long)
{
    int d, m, s;
    char sign;
    char temp_string[32];

    if (Long > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(Long, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sg %c%03d:%02d:%02d#", sign, abs(d), m, s);

    return (setZEQ25StandardProcedure(PortFD, temp_string));
}

int LX200ZEQ25::setZEQ25Latitude(double Lat)
{
    int d, m, s;
    char sign;
    char temp_string[32];

    if (Lat > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(Lat, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":St %c%02d:%02d:%02d#", sign, abs(d), m, s);

    return (setZEQ25StandardProcedure(PortFD, temp_string));
}

int LX200ZEQ25::setZEQ25UTCOffset(double hours)
{
    char temp_string[16];
    char sign;
    int h = 0, m = 0, s = 0;

    if (hours > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(hours, &h, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":SG %c%02d:%02d#", sign, abs(h), m);

    return (setZEQ25StandardProcedure(PortFD, temp_string));
}

int LX200ZEQ25::setZEQ25StandardProcedure(int fd, const char *data)
{
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGF(DBG_SCOPE, "CMD <%s>", data);

    if ((error_type = tty_write_string(fd, data, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, bool_return, 1, 5, &nbytes_read);

    // JM: Hack from Jon in the INDI forums to fix longitude/latitude settings failure on ZEQ25
    usleep(10000);
    tcflush(fd, TCIFLUSH);
    usleep(10000);

    if (nbytes_read < 1)
        return error_type;

    DEBUGF(DBG_SCOPE, "RES <%c>", bool_return[0]);

    if (bool_return[0] == '0')
    {
        DEBUGF(DBG_SCOPE, "CMD <%s> failed.", data);
        return -1;
    }

    DEBUGF(DBG_SCOPE, "CMD <%s> successful.", data);

    return 0;
}

bool LX200ZEQ25::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    int current_move = (dir == DIRECTION_NORTH) ? LX200_NORTH : LX200_SOUTH;

    switch (command)
    {
        case MOTION_START:
            if (!isSimulation() && moveZEQ25To(current_move) < 0)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error setting N/S motion direction.");
                return false;
            }
            else
                DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.",
                       (current_move == LX200_NORTH) ? "North" : "South");
            break;

        case MOTION_STOP:
            if (!isSimulation() && haltZEQ25Movement() < 0)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error stopping N/S motion.");
                return false;
            }
            else
                DEBUGF(INDI::Logger::DBG_SESSION, "Movement toward %s halted.",
                       (current_move == LX200_NORTH) ? "North" : "South");
            break;
    }

    return true;
}

bool LX200ZEQ25::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    int current_move = (dir == DIRECTION_WEST) ? LX200_WEST : LX200_EAST;

    switch (command)
    {
        case MOTION_START:
            if (!isSimulation() && moveZEQ25To(current_move) < 0)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error setting W/E motion direction.");
                return false;
            }
            else
                DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.", (current_move == LX200_WEST) ? "West" : "East");
            break;

        case MOTION_STOP:
            if (!isSimulation() && haltZEQ25Movement() < 0)
            {
                DEBUG(INDI::Logger::DBG_ERROR, "Error stopping W/E motion.");
                return false;
            }
            else
                DEBUGF(INDI::Logger::DBG_SESSION, "Movement toward %s halted.",
                       (current_move == LX200_WEST) ? "West" : "East");
            break;
    }

    return true;
}

int LX200ZEQ25::moveZEQ25To(int direction)
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int nbytes_write = 0;

    switch (direction)
    {
        case LX200_NORTH:
            DEBUGF(DBG_SCOPE, "CMD <%s>", ":mn#");
            tty_write_string(PortFD, ":mn#", &nbytes_write);
            break;
        case LX200_WEST:
            DEBUGF(DBG_SCOPE, "CMD <%s>", ":mw#");
            tty_write_string(PortFD, ":mw#", &nbytes_write);
            break;
        case LX200_EAST:
            DEBUGF(DBG_SCOPE, "CMD <%s>", ":me#");
            tty_write_string(PortFD, ":me#", &nbytes_write);
            break;
        case LX200_SOUTH:
            DEBUGF(DBG_SCOPE, "CMD <%s>", ":ms#");
            tty_write_string(PortFD, ":ms#", &nbytes_write);
            break;
        default:
            break;
    }

    tcflush(PortFD, TCIFLUSH);
    return 0;
}

int LX200ZEQ25::haltZEQ25Movement()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(PortFD, ":q#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(PortFD, TCIFLUSH);
    return 0;
}

bool LX200ZEQ25::SetTrackMode(uint8_t mode)
{
    return (setZEQ25TrackMode(mode) == 0);
}

int LX200ZEQ25::setZEQ25TrackMode(int mode)
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);

    // We don't support KING mode :RT3, so we turn mode=3 to custom :RT4#
    if (mode == 3)
        mode = 4;

    char cmd[6];
    snprintf(cmd, 6, ":RT%d#", mode);

    return setZEQ25StandardProcedure(PortFD, cmd);
}

int LX200ZEQ25::setZEQ25Park()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(PortFD, ":MP1#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(PortFD, TCIFLUSH);
    return 0;
}

int LX200ZEQ25::setZEQ25UnPark()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(PortFD, ":MP0#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(PortFD, TCIFLUSH);
    return 0;
}

bool LX200ZEQ25::isZEQ25Parked()
{
    if (isSimulation())
    {
        return isParked();
    }

    char cmd[]  = ":AP#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[2];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if ((errcode = tty_read(PortFD, response, 1, 3, &nbytes_read)))
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(PortFD, TCIFLUSH);

        return (response[0] == '1');
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

bool LX200ZEQ25::SetCurrentPark()
{
    ln_hrz_posn horizontalPos;
    // Libnova south = 0, west = 90, north = 180, east = 270

    ln_lnlat_posn observer;
    observer.lat = LocationN[LOCATION_LATITUDE].value;
    observer.lng = LocationN[LOCATION_LONGITUDE].value;
    if (observer.lng > 180)
        observer.lng -= 360;

    ln_equ_posn equatorialPos;
    equatorialPos.ra  = currentRA * 15;
    equatorialPos.dec = currentDEC;
    ln_get_hrz_from_equ(&equatorialPos, &observer, ln_get_julian_from_sys(), &horizontalPos);

    double parkAZ = horizontalPos.az - 180;
    if (parkAZ < 0)
        parkAZ += 360;
    double parkAlt = horizontalPos.alt;

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, parkAZ, 2, 3600);
    fs_sexa(AltStr, parkAlt, 2, 3600);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Setting current parking position to coordinates Az (%s) Alt (%s)...", AzStr,
           AltStr);

    SetAxis1Park(parkAZ);
    SetAxis2Park(parkAlt);

    return true;
}

bool LX200ZEQ25::SetDefaultPark()
{
    // Az = 0 for North hemisphere
    SetAxis1Park(LocationN[LOCATION_LATITUDE].value > 0 ? 0 : 180);

    // Alt = Latitude
    SetAxis2Park(LocationN[LOCATION_LATITUDE].value);

    return true;
}

bool LX200ZEQ25::Park()
{
    double parkAz  = GetAxis1Park();
    double parkAlt = GetAxis2Park();

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, parkAz, 2, 3600);
    fs_sexa(AltStr, parkAlt, 2, 3600);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Parking to Az (%s) Alt (%s)...", AzStr, AltStr);

    ln_hrz_posn horizontalPos;
    // Libnova south = 0, west = 90, north = 180, east = 270

    horizontalPos.alt = parkAlt;
    horizontalPos.az  = parkAz + 180;
    if (horizontalPos.az > 360)
        horizontalPos.az -= 360;

    ln_lnlat_posn observer;
    observer.lat = LocationN[LOCATION_LATITUDE].value;
    observer.lng = LocationN[LOCATION_LONGITUDE].value;
    if (observer.lng > 180)
        observer.lng -= 360;

    ln_equ_posn equatorialPos;
    ln_get_equ_from_hrz(&horizontalPos, &observer, ln_get_julian_from_sys(), &equatorialPos);
    equatorialPos.ra /= 15.0;

    if (setObjectRA(PortFD, equatorialPos.ra) < 0 || (setObjectDEC(PortFD, equatorialPos.dec)) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting RA/Dec.");
        return false;
    }

    int err = 0;
    /* Slew reads the '0', that is not the end of the slew */
    if (slewZEQ25() == 0)
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error Slewing to Az %s - Alt %s", AzStr, AltStr);
        slewError(err);
        return false;
    }

    EqNP.s     = IPS_BUSY;
    TrackState = SCOPE_PARKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Parking is in progress...");

    return true;
}

bool LX200ZEQ25::UnPark()
{
    // First we unpark astrophysics
    if (!isSimulation())
    {
        if (setZEQ25UnPark() < 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "UnParking Failed.");
            return false;
        }
    }

    // Then we sync with to our last stored position
    double parkAz  = GetAxis1Park();
    double parkAlt = GetAxis2Park();

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, parkAz, 2, 3600);
    fs_sexa(AltStr, parkAlt, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Syncing to parked coordinates Az (%s) Alt (%s)...", AzStr, AltStr);

    ln_hrz_posn horizontalPos;
    // Libnova south = 0, west = 90, north = 180, east = 270

    horizontalPos.alt = parkAlt;
    horizontalPos.az  = parkAz + 180;
    if (horizontalPos.az > 360)
        horizontalPos.az -= 360;

    ln_lnlat_posn observer;
    observer.lat = LocationN[LOCATION_LATITUDE].value;
    observer.lng = LocationN[LOCATION_LONGITUDE].value;
    if (observer.lng > 180)
        observer.lng -= 360;

    ln_equ_posn equatorialPos;
    ln_get_equ_from_hrz(&horizontalPos, &observer, ln_get_julian_from_sys(), &equatorialPos);
    equatorialPos.ra /= 15.0;

    if (setObjectRA(PortFD, equatorialPos.ra) < 0 || (setObjectDEC(PortFD, equatorialPos.dec)) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting RA/DEC.");
        return false;
    }

    if (Sync(equatorialPos.ra, equatorialPos.dec) == false)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Sync failed.");
        return false;
    }

    SetParked(false);
    return true;
}

bool LX200ZEQ25::ReadScopeStatus()
{
    if (!isConnected())
        return false;

    if (isSimulation())
    {
        mountSim();
        return true;
    }

    //if (check_lx200_connection(PortFD))
    //return false;

    if (HomeSP.s == IPS_BUSY)
    {
        if (isZEQ25Home())
        {
            HomeS[0].s = ISS_ON;
            HomeSP.s   = IPS_OK;
            DEBUG(INDI::Logger::DBG_SESSION, "Telescope arrived at home position.");
            IDSetSwitch(&HomeSP, nullptr);
        }
    }

    if (TrackState == SCOPE_SLEWING)
    {
        // Check if LX200 is done slewing
        if (isSlewComplete())
        {
            TrackState = SCOPE_TRACKING;
            DEBUG(INDI::Logger::DBG_SESSION, "Slew is complete. Tracking...");
        }
    }
    else if (TrackState == SCOPE_PARKING)
    {
        if (isSlewComplete())
        {
            setZEQ25Park();
            SetParked(true);
        }
    }

    if (getLX200RA(PortFD, &currentRA) < 0 || getLX200DEC(PortFD, &currentDEC) < 0)
    {
        EqNP.s = IPS_ALERT;
        IDSetNumber(&EqNP, "Error reading RA/DEC.");
        return false;
    }

    NewRaDec(currentRA, currentDEC);

    return true;
}

void LX200ZEQ25::mountSim()
{
    static struct timeval ltv;
    struct timeval tv;
    double dt, da, dx;
    int nlocked;

    /* update elapsed time since last poll, don't presume exactly POLLMS */
    gettimeofday(&tv, nullptr);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
        ltv = tv;

    dt  = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec) / 1e6;
    ltv = tv;
    da  = SLEWRATE * dt;

    /* Process per current state. We check the state of EQUATORIAL_COORDS and act acoordingly */
    switch (TrackState)
    {
        case SCOPE_TRACKING:
            /* RA moves at sidereal, Dec stands still */
            currentRA += (SIDRATE * dt / 15.);
            break;

        case SCOPE_SLEWING:
        case SCOPE_PARKING:
            /* slewing - nail it when both within one pulse @ SLEWRATE */
            nlocked = 0;

            dx = targetRA - currentRA;

            if (fabs(dx) <= da)
            {
                currentRA = targetRA;
                nlocked++;
            }
            else if (dx > 0)
                currentRA += da / 15.;
            else
                currentRA -= da / 15.;

            dx = targetDEC - currentDEC;
            if (fabs(dx) <= da)
            {
                currentDEC = targetDEC;
                nlocked++;
            }
            else if (dx > 0)
                currentDEC += da;
            else
                currentDEC -= da;

            if (nlocked == 2)
            {
                if (TrackState == SCOPE_SLEWING)
                    TrackState = SCOPE_TRACKING;
                else
                    SetParked(true);
            }

            break;

        default:
            break;
    }

    NewRaDec(currentRA, currentDEC);
}

int LX200ZEQ25::getZEQ25GuideRate(double *rate)
{
    char cmd[]  = ":AG#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        snprintf(response, 8, "%3d#", (int)(GuideRateN[0].value * 100));
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return errcode;
        }

        if ((errcode = tty_read(PortFD, response, 4, 3, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return errcode;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        int rate_num;

        if (sscanf(response, "%d#", &rate_num) > 0)
        {
            *rate = rate_num / 100.0;
            tcflush(PortFD, TCIFLUSH);
            return TTY_OK;
        }
        else
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error: Malformed result (%s).", response);
            return -1;
        }
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return -1;
}

int LX200ZEQ25::setZEQ25GuideRate(double rate)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    int num = rate * 100;
    snprintf(cmd, 16, ":RG%03d#", num);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        strcpy(response, "1");
        nbytes_read = strlen(response);
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return errcode;
        }

        if ((errcode = tty_read(PortFD, response, 1, 3, &nbytes_read)))
        {
            tty_error_msg(errcode, errmsg, MAXRBUF);
            DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
            return errcode;
        }
    }

    if (nbytes_read > 0)
    {
        response[nbytes_read] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES (%s)", response);

        tcflush(PortFD, TCIFLUSH);
        return true;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return -1;
}

int LX200ZEQ25::SendPulseCmd(int direction, int duration_msec)
{
    int nbytes_write = 0;
    char cmd[20];
    switch (direction)
    {
        case LX200_NORTH:
            sprintf(cmd, ":Mn%04d#", duration_msec);
            break;
        case LX200_SOUTH:
            sprintf(cmd, ":Ms%04d#", duration_msec);
            break;
        case LX200_EAST:
            sprintf(cmd, ":Me%04d#", duration_msec);
            break;
        case LX200_WEST:
            sprintf(cmd, ":Mw%04d#", duration_msec);
            break;
        default:
            return 1;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    tty_write_string(PortFD, cmd, &nbytes_write);

    tcflush(PortFD, TCIFLUSH);
    return 0;
}
