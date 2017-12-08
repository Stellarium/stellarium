/*
    GotoNova INDI driver

    Copyright (C) 2017 Jasem Mutlaq

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

#include "lx200gotonova.h"

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
#define GOTONOVA_TIMEOUT 5 /* timeout */
#define GOTONOVA_CALDATE_RESULT "                                #                                #" /* result of calendar date */

LX200GotoNova::LX200GotoNova()
{
    setVersion(1, 0);

    setLX200Capability(LX200_HAS_FOCUS);

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                           TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_TRACK_MODE,
                           4);
}

bool LX200GotoNova::initProperties()
{
    LX200Generic::initProperties();


    strcpy(SlewRateS[0].label, "16x");
    strcpy(SlewRateS[1].label, "64x");
    strcpy(SlewRateS[2].label, "256x");
    strcpy(SlewRateS[3].label, "512x");

    // Sync Type
    IUFillSwitch(&SyncCMRS[USE_REGULAR_SYNC], ":CM#", ":CM#", ISS_ON);
    IUFillSwitch(&SyncCMRS[USE_CMR_SYNC], ":CMR#", ":CMR#", ISS_OFF);
    IUFillSwitchVector(&SyncCMRSP, SyncCMRS, 2, getDeviceName(), "SYNCCMR", "Sync", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Park Position
    IUFillSwitch(&ParkPositionS[PS_NORTH_POLE], "North Pole", "", ISS_ON);
    IUFillSwitch(&ParkPositionS[PS_LEFT_VERTICAL], "Left and Vertical", "", ISS_OFF);
    IUFillSwitch(&ParkPositionS[PS_LEFT_HORIZON], "Left and Horizon", "", ISS_OFF);
    IUFillSwitch(&ParkPositionS[PS_RIGHT_VERTICAL], "Right and Vertical", "", ISS_OFF);
    IUFillSwitch(&ParkPositionS[PS_RIGHT_HORIZON], "Right and Horizon", "", ISS_OFF);
    IUFillSwitchVector(&ParkPositionSP, ParkPositionS, 5, getDeviceName(), "PARKING_POSITION", "Parking Position", SITE_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Guide Rate
    IUFillSwitch(&GuideRateS[0], "1.0x", "", ISS_ON);
    IUFillSwitch(&GuideRateS[1], "0.8x", "", ISS_OFF);
    IUFillSwitch(&GuideRateS[2], "0.6x", "", ISS_OFF);
    IUFillSwitch(&GuideRateS[3], "0.4x", "", ISS_OFF);
    IUFillSwitchVector(&GuideRateSP, GuideRateS, 4, getDeviceName(), "GUIDE_RATE", "Guide Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Track Mode -- We do not support Custom so let's just define the first 3 properties
    TrackModeSP.nsp = 3;

    return true;
}

bool LX200GotoNova::updateProperties()
{
    LX200Generic::updateProperties();

    if (isConnected())
    {
        defineSwitch(&SyncCMRSP);
        defineSwitch(&ParkPositionSP);
        defineSwitch(&GuideRateSP);
    }
    else
    {
        deleteProperty(SyncCMRSP.name);
        deleteProperty(ParkPositionSP.name);
        deleteProperty(GuideRateSP.name);
    }

    return true;
}

const char *LX200GotoNova::getDefaultName()
{
    return (const char *)"GotoNova";
}

bool LX200GotoNova::checkConnection()
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

bool LX200GotoNova::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Park Position
        if (!strcmp(ParkPositionSP.name, name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&ParkPositionSP);
            IUUpdateSwitch(&ParkPositionSP, states, names, n);
            if (setGotoNovaParkPosition(IUFindOnSwitchIndex(&ParkPositionSP)) == TTY_OK)
                ParkPositionSP.s = IPS_OK;
            else
            {
                IUResetSwitch(&ParkPositionSP);
                ParkPositionS[currentSwitch].s = ISS_ON;
                ParkPositionSP.s = IPS_ALERT;
            }

            IDSetSwitch(&ParkPositionSP, nullptr);
            return true;
        }

        // Guide Rate
        if (!strcmp(GuideRateSP.name, name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&ParkPositionSP);
            IUUpdateSwitch(&GuideRateSP, states, names, n);
            if (setGotoNovaGuideRate(IUFindOnSwitchIndex(&GuideRateSP)) == TTY_OK)
                GuideRateSP.s = IPS_OK;
            else
            {
                IUResetSwitch(&GuideRateSP);
                GuideRateS[currentSwitch].s = ISS_ON;
                GuideRateSP.s = IPS_ALERT;
            }

            IDSetSwitch(&GuideRateSP, nullptr);
            return true;
        }

        // Sync type
        if (!strcmp(name, SyncCMRSP.name))
        {
            IUResetSwitch(&SyncCMRSP);
            IUUpdateSwitch(&SyncCMRSP, states, names, n);
            IUFindOnSwitchIndex(&SyncCMRSP);
            SyncCMRSP.s = IPS_OK;
            IDSetSwitch(&SyncCMRSP, nullptr);
            return true;
        }
    }

    return LX200Generic::ISNewSwitch(dev, name, states, names, n);
}

bool LX200GotoNova::isSlewComplete()
{
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    const char *cmd = ":SE?#";

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

        if (response[0] == '0')
            return true;
        else
            return false;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return false;
}

void LX200GotoNova::getBasicData()
{
    int guideRate=-1;
    int rc = getGotoNovaGuideRate(&guideRate);
    if (rc == TTY_OK)
    {
        IUResetSwitch(&GuideRateSP);
        GuideRateS[guideRate].s = ISS_ON;
        GuideRateSP.s = IPS_OK;
        IDSetSwitch(&GuideRateSP, nullptr);
    }
}

bool LX200GotoNova::Goto(double r, double d)
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

        if (slewGotoNova() == 0)
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

bool LX200GotoNova::Sync(double ra, double dec)
{
    char syncString[256];

    int syncType = IUFindOnSwitchIndex(&SyncCMRSP);

    if (!isSimulation())
    {
        if (setObjectRA(PortFD, ra) < 0 || setObjectDEC(PortFD, dec) < 0)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error setting RA/DEC. Unable to Sync.");
            return false;
        }

        bool syncOK = true;

        switch (syncType)
        {
        case USE_REGULAR_SYNC:
            if (::Sync(PortFD, syncString) < 0)
                syncOK = false;
            break;

        case USE_CMR_SYNC:
            if (GotonovaSyncCMR(syncString) < 0)
                syncOK = false;
            break;

        default:
            break;
        }

        if (syncOK == false)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Synchronization failed.");
            return false;
        }

    }

    currentRA  = ra;
    currentDEC = dec;

    DEBUGF(INDI::Logger::DBG_DEBUG, "%s Synchronization successful %s", (syncType == USE_REGULAR_SYNC ? "CM" : "CMR"), syncString);
    DEBUG(INDI::Logger::DBG_SESSION, "Synchronization successful.");

    EqNP.s     = IPS_OK;

    NewRaDec(currentRA, currentDEC);

    return true;
}

int LX200GotoNova::GotonovaSyncCMR(char *matchedObject)
{
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", "#:CMR#");

    if ((error_type = tty_write_string(PortFD, ":CMR#", &nbytes_write)) != TTY_OK)
        return error_type;

    if ((error_type = tty_read_section(PortFD, matchedObject, '#', 3, &nbytes_read)) != TTY_OK)
        return error_type;

    matchedObject[nbytes_read - 1] = '\0';

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES <%s>", matchedObject);

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);

    tcflush(PortFD, TCIFLUSH);

    return 0;
}


int LX200GotoNova::slewGotoNova()
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

bool LX200GotoNova::SetSlewRate(int index)
{
    if (isSimulation())
        return true;

    char cmd[8];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    snprintf(cmd, 8, ":RC%d#", index);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return false;
    }

    return true;
}

bool LX200GotoNova::updateTime(ln_date *utc, double utc_offset)
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

    if (setGotoNovaUTCOffset(utc_offset) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC Offset.");
        return false;
    }

    return true;
}

int LX200GotoNova::setCalenderDate(int fd, int dd, int mm, int yy)
{
    char read_buffer[16];
    char response[67];
    char good_result[] = GOTONOVA_CALDATE_RESULT;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    yy = yy % 100;

    snprintf(read_buffer, sizeof(read_buffer), ":SC %02d/%02d/%02d#", mm, dd, yy);

    DEBUGF(DBG_SCOPE, "CMD <%s>", read_buffer);

    tcflush(fd, TCIFLUSH);

    if ((error_type = tty_write_string(fd, read_buffer, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, response, sizeof(response), GOTONOVA_TIMEOUT, &nbytes_read);

    tcflush(fd, TCIFLUSH);

    if (nbytes_read < 1)
    {   
        DEBUG(INDI::Logger::DBG_ERROR, "Unable to read response");
        return error_type;
    }

    response[nbytes_read] = '\0';

    DEBUGF(DBG_SCOPE, "RES <%s>", response);

    if (strncmp(response, good_result, strlen(good_result)) == 0) {
        return 0;
    }

    /* Sleep 10ms before flushing. This solves some issues with LX200 compatible devices. */
    usleep(10000);
    tcflush(fd, TCIFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Set date failed! Response: <%s>", response);

    return -1;
}

bool LX200GotoNova::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    if (isSimulation())
        return true;

    double final_longitude;

    if (longitude > 180)
        final_longitude = longitude - 360.0;
    else
        final_longitude = longitude;

    if (!isSimulation() && setGotoNovaLongitude(final_longitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site longitude coordinates");
        return false;
    }

    if (!isSimulation() && setGotoNovaLatitude(latitude) < 0)
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

int LX200GotoNova::setGotoNovaLongitude(double Long)
{
    int d, m, s;
    char sign;
    char temp_string[32];

    if (Long > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(Long, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sg %c%03d*%02d:%02d#", sign, abs(d), m, s);

    return (setGotoNovaStandardProcedure(PortFD, temp_string));
}

int LX200GotoNova::setGotoNovaLatitude(double Lat)
{
    int d, m, s;
    char sign;
    char temp_string[32];

    if (Lat > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(Lat, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":St %c%02d*%02d:%02d#", sign, abs(d), m, s);

    return (setGotoNovaStandardProcedure(PortFD, temp_string));
}

int LX200GotoNova::setGotoNovaUTCOffset(double hours)
{
    char temp_string[16];
    char sign;
    int h = 0, m = 0, s = 0;

    if (hours > 0)
        sign = '+';
    else
        sign = '-';

    getSexComponents(hours, &h, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":SG %c%02d#", sign, abs(h));

    return (setGotoNovaStandardProcedure(PortFD, temp_string));
}

int LX200GotoNova::setGotoNovaStandardProcedure(int fd, const char *data)
{
    char bool_return[2];
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGF(DBG_SCOPE, "CMD <%s>", data);

    if ((error_type = tty_write_string(fd, data, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, bool_return, 1, 5, &nbytes_read);

    // JM: Hack from Jon in the INDI forums to fix longitude/latitude settings failure on GotoNova
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

bool LX200GotoNova::SetTrackMode(uint8_t mode)
{
    return (setGotoNovaTrackMode(mode) == 0);
}

int LX200GotoNova::setGotoNovaTrackMode(int mode)
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);

    char cmd[8];
    snprintf(cmd, 8, ":STR%d#", mode);

    return setGotoNovaStandardProcedure(PortFD, cmd);
}

bool LX200GotoNova::Park()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(PortFD, ":PK#", &nbytes_write)) != TTY_OK)
        return error_type;

    tcflush(PortFD, TCIFLUSH);

    EqNP.s     = IPS_BUSY;
    TrackState = SCOPE_PARKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Parking is in progress...");

    return true;
}

bool LX200GotoNova::UnPark()
{
    SetParked(false);
    return true;
}

bool LX200GotoNova::ReadScopeStatus()
{
    if (!isConnected())
        return false;

    if (isSimulation())
    {
        mountSim();
        return true;
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

    syncSideOfPier();

    return true;
}

void LX200GotoNova::mountSim()
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

int LX200GotoNova::getGotoNovaGuideRate(int *rate)
{
    char cmd[]  = ":GGS#";
    int errcode = 0;
    char errmsg[MAXRBUF];
    char response[8];
    int nbytes_read    = 0;
    int nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        snprintf(response, 8, "%d#", IUFindOnSwitchIndex(&GuideRateSP));
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

        *rate = atoi(response);

        return 0;
    }

    DEBUGF(INDI::Logger::DBG_ERROR, "Only received #%d bytes, expected 1.", nbytes_read);
    return -1;
}

int LX200GotoNova::setGotoNovaGuideRate(int rate)
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    snprintf(cmd, 16, ":SGS%0d#", rate);

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);

    if (isSimulation())
    {
        return 0;
    }

    tcflush(PortFD, TCIFLUSH);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    return 0;
}

int LX200GotoNova::setGotoNovaParkPosition(int position)
{
    char temp_string[16];

    snprintf(temp_string, sizeof(temp_string), ":STPKP%d#", position);

    return (setGotoNovaStandardProcedure(PortFD, temp_string));
}

void LX200GotoNova::syncSideOfPier()
{
    const char *cmd = ":pS#";
    // Response
    char response[16] = { 0 };
    int rc = 0, nbytes_read = 0, nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: <%s>", cmd);

    tcflush(PortFD, TCIOFLUSH);

    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        char errmsg[256];
        tty_error_msg(rc, errmsg, 256);
        DEBUGF(INDI::Logger::DBG_ERROR, "Error writing to device %s (%d)", errmsg, rc);
        return;
    }

    // Read Side
    if ((rc = tty_read_section(PortFD, response, '#', 3, &nbytes_read)) != TTY_OK)
    {
        char errmsg[256];
        tty_error_msg(rc, errmsg, 256);
        DEBUGF(INDI::Logger::DBG_ERROR, "Error reading from device %s (%d)", errmsg, rc);
        return;
    }

    response[nbytes_read - 1] = '\0';

    tcflush(PortFD, TCIOFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES: <%s>", response);

    if (!strcmp(response, "East"))
        setPierSide(INDI::Telescope::PIER_EAST);
    else
        setPierSide(INDI::Telescope::PIER_WEST);

}

bool LX200GotoNova::saveConfigItems(FILE *fp)
{
    LX200Generic::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &SyncCMRSP);
    IUSaveConfigSwitch(fp, &ParkPositionSP);

    return true;
}
