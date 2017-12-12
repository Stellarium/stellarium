/*
    ioptronHC8406 INDI driver

    Copyright (C) 2017 Nacho Mas. Base on GotoNova driver by Jasem Mutlaq

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

/*
HC8406 CMD hardware TEST
V1.10 March 21, 2011
UPGRADE INFO ON: http://www.ioptron.com/Articles.asp?ID=268

INFO
----
# ->			repeat last command
:GG# 			+08:00:00    UTC OffSet
:Gg# 			-003*18:03#  longitud
:Gt# 			+41*06:56#   latitude
:GL# 			7:02:47.0#   local time
:GS# 			20:12: 3.3#  Sideral Time
:GR#  			2:12:57.4#   RA
:GD#  			+90* 0: 0#   DEC
:GA# 			+41* 6:55#   ALT
:GZ#   			0* 0: 0#     AZ
:GC#  			03:12:09#    Calendar day
:pS#  			East#        pier side
:FirmWareDate# 		:20110506#   
:V#      		V1.00#

COMMANDS
--------
:CM#  			Coordinates     matched.        #
:CMR# 			Coordinates     matched.        #

This only works if the mount is not stopped (tracking)
:RT0# --> 		Lunar
:RT1# --> 		solar
:RT2# --> 		sideral
:RT9# -->               zero but not work!!

!!!There isn't a command to start/stop tracking !!! You have to do manualy

This speeds only are taken into account for protocol buttons, not for the HC Buttons
:RG#  -->  Select guide speed for :Mn#,:Ms# ....
:RG0,1,2 -->preselect guide speed 0.25x, 0.5x, 1.0x (HC shows it)
:RC#  -->  Select center speed for :Mn#,:Ms# .... (Not Works)
:RC0,1,2 -->preselect guide speed  (HC doesn't shows it)
:Mn# :Ms# :Me# :Mw#  (move until :Q# at guiding or center speed :RG# (works)or :RC#(not work, use :RC0/1/2 instead))
:MnXXX# :MsXXX# :MeXXX# :MwXXX#  (move XXX ms at guiding speed no mather what :RCx#,:RGX# or :RSX# was issue)

Firmware update (HC v1.10 -> v1.12, also mainboard firmware)
------------------------------------------------------------------
HC8406 CMD hardware TEST
V1.12 2011-08-12
UPGRADE INFO ON: http://www.ioptron.com/Articles.asp?ID=268

:RT9#     -> stop tracking
:RT0#     -> start tracking at sidera speed. Formerly only preselect sideral speed but
             not start the tracking itself

:Me#
:Mw#
:Mexxx#
:Mwxxx#   ->Al this commands are broken. Only RA axes, the equivalent :Ms#,:Mn#... work.

Summarizing:

old firmware v1.10: is not possible to start/stop tracking. Guide/move commands OK
new firmware v1.12: is possible to start/stop tracking. Guide/move commands NOT OK

*/

/* SOCAT sniffer
socat  -v  PTY,link=/tmp/serial,wait-slave,raw /dev/ttyUSB0,raw
*/



#include "ioptronHC8406.h"

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
#define ioptronHC8406_TIMEOUT 1 /* timeout */
#define ioptronHC8406_CALDATE_RESULT "                                #                 " /* result of calendar date */


ioptronHC8406::ioptronHC8406()
{
    setVersion(1, 1);
    setLX200Capability(LX200_HAS_FOCUS | LX200_HAS_PULSE_GUIDING);
    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | 
                      TELESCOPE_CAN_ABORT | TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | 
                      TELESCOPE_HAS_TRACK_MODE | TELESCOPE_CAN_CONTROL_TRACK);

}

bool ioptronHC8406::initProperties()
{
    LX200Generic::initProperties();

    // Sync Type
    IUFillSwitch(&SyncCMRS[USE_REGULAR_SYNC], "USE_REGULAR_SYNC", ":CM#", ISS_ON);
    IUFillSwitch(&SyncCMRS[USE_CMR_SYNC], "USE_CMR_SYNC", ":CMR#", ISS_OFF);
    IUFillSwitchVector(&SyncCMRSP, SyncCMRS, 2, getDeviceName(), "SYNC_MODE", "Sync", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,IPS_IDLE);

    // Cursor move Guiding/Center
    IUFillSwitch(&CursorMoveSpeedS[USE_GUIDE_SPEED],"USE_GUIDE_SPEED", "Guide Speed", ISS_ON);
    IUFillSwitch(&CursorMoveSpeedS[USE_CENTERING_SPEED],"USE_CENTERING_SPEED", "Centering Speed", ISS_OFF);
    IUFillSwitchVector(&CursorMoveSpeedSP, CursorMoveSpeedS, 2, getDeviceName(),
	 "CURSOR_MOVE_MODE", "Cursor Move Speed", MOTION_TAB, IP_RO, ISR_1OFMANY, 0,IPS_IDLE);

    // Guide Rate
    IUFillSwitch(&GuideRateS[0], "0.25x", "", ISS_OFF);
    IUFillSwitch(&GuideRateS[1], "0.50x", "", ISS_ON);
    IUFillSwitch(&GuideRateS[2], "1.0x", "", ISS_OFF);
    IUFillSwitchVector(&GuideRateSP, GuideRateS, 3, getDeviceName(),
          "GUIDE_RATE", "Guide Speed", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,IPS_IDLE);

    // Center Rate
    IUFillSwitch(&CenterRateS[0], "12x", "", ISS_OFF);
    IUFillSwitch(&CenterRateS[1], "64x", "", ISS_ON);
    IUFillSwitch(&CenterRateS[2], "600x", "", ISS_OFF);
    IUFillSwitch(&CenterRateS[3], "1200x", "", ISS_OFF);
    IUFillSwitchVector(&CenterRateSP, CenterRateS, 4, getDeviceName(),
          "CENTER_RATE", "Center Speed", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,IPS_IDLE);

    // Slew Rate  //NOT WORK!!
    IUFillSwitch(&SlewRateS[0], "600x", "", ISS_OFF);
    IUFillSwitch(&SlewRateS[1], "900x", "", ISS_OFF);
    IUFillSwitch(&SlewRateS[2], "1200x", "", ISS_ON);

    IUFillSwitchVector(&SlewRateSP, SlewRateS, 3, getDeviceName(),
          "SLEW_RATE", "Slew Speed", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,IPS_IDLE);

    TrackModeSP.nsp = 3;

    return true;
}

bool ioptronHC8406::updateProperties()
{
    LX200Generic::updateProperties();

    if (isConnected())
    {
        defineSwitch(&SyncCMRSP);
        defineSwitch(&GuideRateSP);
        defineSwitch(&CenterRateSP);
        //defineSwitch(&SlewRateSP); //NOT WORK!!
        defineSwitch(&CursorMoveSpeedSP);
        ioptronHC8406Init();
    }
    else
    {
        deleteProperty(SyncCMRSP.name);
        deleteProperty(GuideRateSP.name);
        deleteProperty(CenterRateSP.name);
        //deleteProperty(SlewRateSP.name); //NOT WORK!!
        deleteProperty(CursorMoveSpeedSP.name);
    }

    return true;
}

const char *ioptronHC8406::getDefaultName()
{
    return (const char *)"IOptron HC8406";
}

bool ioptronHC8406::checkConnection()
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

bool ioptronHC8406::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {

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

        // Cursor move type
        if (!strcmp(name, CursorMoveSpeedSP.name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&CursorMoveSpeedSP);
	    IUUpdateSwitch(&CursorMoveSpeedSP, states, names, n);
            if (setioptronHC8406CursorMoveSpeed(IUFindOnSwitchIndex(&CursorMoveSpeedSP)) == TTY_OK)
                CursorMoveSpeedSP.s = IPS_OK;
            else
            {
                IUResetSwitch(&CursorMoveSpeedSP);
                CursorMoveSpeedS[currentSwitch].s = ISS_ON;
                CursorMoveSpeedSP.s = IPS_ALERT;
            }
            return true;
        }

        // Guide Rate
        if (!strcmp(GuideRateSP.name, name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&GuideRateSP);
            IUUpdateSwitch(&GuideRateSP, states, names, n);
            if (setioptronHC8406GuideRate(IUFindOnSwitchIndex(&GuideRateSP)) == TTY_OK)
            {
                GuideRateSP.s = IPS_OK;
		//Shows guide speed selected
		CursorMoveSpeedS[USE_GUIDE_SPEED].s = ISS_ON;
		CursorMoveSpeedS[USE_CENTERING_SPEED].s = ISS_OFF;
	        CursorMoveSpeedSP.s = IPS_OK;
	        IDSetSwitch(&CursorMoveSpeedSP, nullptr);
            } else {
                IUResetSwitch(&GuideRateSP);
                GuideRateS[currentSwitch].s = ISS_ON;
                GuideRateSP.s = IPS_ALERT;
            }

            IDSetSwitch(&GuideRateSP, nullptr);
            return true;
        }

        // Center Rate
        if (!strcmp(CenterRateSP.name, name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&CenterRateSP);
            IUUpdateSwitch(&CenterRateSP, states, names, n);
            if (setioptronHC8406CenterRate(IUFindOnSwitchIndex(&CenterRateSP)) == TTY_OK)
	    {
                CenterRateSP.s = IPS_OK;
		//Shows centering speed selected
		CursorMoveSpeedS[USE_GUIDE_SPEED].s = ISS_OFF;
		CursorMoveSpeedS[USE_CENTERING_SPEED].s = ISS_ON;
	        CursorMoveSpeedSP.s = IPS_OK;
	        IDSetSwitch(&CursorMoveSpeedSP, nullptr);
            } else {
                IUResetSwitch(&CenterRateSP);
                CenterRateS[currentSwitch].s = ISS_ON;
                CenterRateSP.s = IPS_ALERT;
            }

            IDSetSwitch(&CenterRateSP, nullptr);
            return true;
        }

        // Slew Rate
        if (!strcmp(SlewRateSP.name, name))
        {
            int currentSwitch = IUFindOnSwitchIndex(&SlewRateSP);
            IUUpdateSwitch(&SlewRateSP, states, names, n);
            if (setioptronHC8406SlewRate(IUFindOnSwitchIndex(&SlewRateSP)) == TTY_OK)
                SlewRateSP.s = IPS_OK;
            else
            {
                IUResetSwitch(&SlewRateSP);
                SlewRateS[currentSwitch].s = ISS_ON;
                SlewRateSP.s = IPS_ALERT;
            }

            IDSetSwitch(&SlewRateSP, nullptr);
            return true;
        }

    }

    return LX200Generic::ISNewSwitch(dev, name, states, names, n);
}

bool ioptronHC8406::isSlewComplete()
{
    /* HC8406 doesn't have :SE# or :SE? command, thus we check if the slew is 
       completed comparing targetRA/DEC with actual RA/DEC */

    float tolerance=1/3600.;  // 5 arcsec

    if (fabs(currentRA-targetRA) <= tolerance && fabs(currentDEC-targetDEC) <= tolerance) 
	return true;

    return false;
}

void ioptronHC8406::getBasicData()
{
    checkLX200Format(PortFD);
    sendScopeLocation();
    sendScopeTime();
}

void ioptronHC8406::ioptronHC8406Init()
{
    //This mount doesn't report anything so we send some CMD
    //just to get syncronize with the GUI at start time
    DEBUG(INDI::Logger::DBG_WARNING, "Sending init CMDs. Unpark, Stop tracking");
    UnPark();
    TrackState = SCOPE_IDLE;
    SetTrackEnabled(false);    
    setioptronHC8406GuideRate(1);
}

bool ioptronHC8406::Goto(double r, double d)
{
    targetRA  = r;
    targetDEC = d;
    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, targetRA, 2, 3600);
    fs_sexa(DecStr, targetDEC, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "<GOTO RA/DEC> %s/%s",RAStr,DecStr);

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

    // If parking/parked, let's unpark it first.
    /*
    if (TrackState == SCOPE_PARKING || TrackState == SCOPE_PARKED)
    {
	UnPark();
    }*/

    if (!isSimulation())
    {
        if (setObjectRA(PortFD, targetRA) < 0 || (setObjectDEC(PortFD, targetDEC)) < 0)
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error setting RA/DEC.");
            return false;
        }

        if (slewioptronHC8406() == 0)  //action
        {
            EqNP.s = IPS_ALERT;
            IDSetNumber(&EqNP, "Error Slewing to JNow RA %s - DEC %s\n", RAStr, DecStr);
            slewError(1);
            return false;
        }
    }

    TrackState = SCOPE_SLEWING;
    EqNP.s     = IPS_BUSY;
    DEBUGF(INDI::Logger::DBG_DEBUG, "Slewing to RA: %s - DEC: %s",RAStr,DecStr);
    return true;
}

bool ioptronHC8406::Sync(double ra, double dec)
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
            if (ioptronHC8406SyncCMR(syncString) < 0)
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

int ioptronHC8406::ioptronHC8406SyncCMR(char *matchedObject)
{
    int error_type;
    int nbytes_write = 0;
    int nbytes_read  = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", ":CMR#");

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


int ioptronHC8406::slewioptronHC8406()
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



bool ioptronHC8406::updateTime(ln_date *utc, double utc_offset)
{
    struct ln_zonedate ltm;

    if (isSimulation())
        return true;

    ln_date_to_zonedate(utc, &ltm, utc_offset * 3600.0);

    JD = ln_get_julian_day(utc);

    DEBUGF(INDI::Logger::DBG_DEBUG, "New JD is %.2f", JD);

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

    if (setioptronHC8406UTCOffset(utc_offset) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC Offset.");
        return false;
    }

    return true;
}

int ioptronHC8406::setCalenderDate(int fd, int dd, int mm, int yy)
{
    char read_buffer[16];
    char response[67];
    char good_result[] = ioptronHC8406_CALDATE_RESULT;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;
    yy = yy % 100;

    snprintf(read_buffer, sizeof(read_buffer), ":SC %02d:%02d:%02d#", mm, dd, yy);

    DEBUGF(DBG_SCOPE, "CMD <%s>", read_buffer);

    tcflush(fd, TCIFLUSH);
    /* Sleep 100ms before flushing. This solves some issues with LX200 compatible devices. */
    //usleep(10);
    if ((error_type = tty_write_string(fd, read_buffer, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, response, sizeof(response), ioptronHC8406_TIMEOUT, &nbytes_read);

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


    tcflush(fd, TCIFLUSH);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Set date failed! Response: <%s>", response);

    return -1;
}

bool ioptronHC8406::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    if (isSimulation())
        return true;

    double final_longitude;

    if (longitude > 180)
        final_longitude = longitude - 360.0;
    else
        final_longitude = longitude;

    if (!isSimulation() && setioptronHC8406Longitude(final_longitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site longitude coordinates");
        return false;
    }

    if (!isSimulation() && setioptronHC8406Latitude(latitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site latitude coordinates");
        return false;
    }

    char l[32], L[32];
    fs_sexa(l, latitude, 3, 3600);
    fs_sexa(L, longitude, 4, 3600);

    IDMessage(getDeviceName(), "Site location updated to Lat %.32s - Long %.32s", l, L);

    return true;
}

int ioptronHC8406::setioptronHC8406Longitude(double Long)
{
    int d, m, s;
    char sign;
    char temp_string[32];

    if (Long > 0)
        sign = '+';
    else
        sign = '-';

    Long=360-Long;

    getSexComponents(Long, &d, &m, &s);

    snprintf(temp_string, sizeof(temp_string), ":Sg %03d*%02d:%02d#", abs(d), m, s);

    return (setioptronHC8406StandardProcedure(PortFD, temp_string));
}

int ioptronHC8406::setioptronHC8406Latitude(double Lat)
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

    return (setioptronHC8406StandardProcedure(PortFD, temp_string));
}

int ioptronHC8406::setioptronHC8406UTCOffset(double hours)
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

    return (setioptronHC8406StandardProcedure(PortFD, temp_string));
}

int ioptronHC8406::setioptronHC8406StandardProcedure(int fd, const char *data)
{
    char bool_return[2];
    int error_type=0;
    int nbytes_write = 0, nbytes_read = 0;

    DEBUGF(DBG_SCOPE, "CMD <%s>", data);

    if ((error_type = tty_write_string(fd, data, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read(fd, bool_return, 1, 5, &nbytes_read);

    // JM: Hack from Jon in the INDI forums to fix longitude/latitude settings failure 
    
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

bool ioptronHC8406::SetTrackEnabled(bool enabled)
{
    if (enabled) {
     DEBUG(INDI::Logger::DBG_WARNING, "<SetTrackEnabled> START TRACKING AT SIDERAL SPEED (:RT2#)");
     return setioptronHC8406TrackMode(0);
    } else {
     DEBUG(INDI::Logger::DBG_WARNING, "<SetTrackEnabled> STOP TRACKING (:RT9#)");
     return setioptronHC8406TrackMode(3);
    }
}

bool ioptronHC8406::SetTrackMode(uint8_t mode)
{
    return (setioptronHC8406TrackMode(mode));
}

int ioptronHC8406::setioptronHC8406TrackMode(int mode)
{

    char cmd[8];
    int mmode=0;
    int error_type=0;
    int nbytes_write = 0 ;

    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);

    if (mode == 0 ) {
	mmode=2;
    } else if (mode ==1) {
	mmode=1;
    } else if (mode ==2) {
	mmode=0;
    } else if (mode ==3) {
	mmode=9;
    }
    snprintf(cmd, 8, ":RT%d#", mmode);

    DEBUGF(DBG_SCOPE, "CMD <%s>", cmd);
    
    //None return value so just write cmd and exit without reading the response
    if ((error_type = tty_write_string(PortFD, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    return 1;
}

bool ioptronHC8406::Park()
{
    DEBUGF(DBG_SCOPE, "<%s>", __FUNCTION__);
    int error_type;
    int nbytes_write = 0;

    if ((error_type = tty_write_string(PortFD, ":KA#", &nbytes_write)) != TTY_OK)
        return error_type;
    tcflush(PortFD, TCIFLUSH);
    DEBUG(DBG_SCOPE, "CMD <:KA#>");

    EqNP.s     = IPS_BUSY;
    TrackState = SCOPE_PARKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Parking is in progress...");

    return true;
}

bool ioptronHC8406::UnPark()
{
    SetParked(false);
    return true;
}

bool ioptronHC8406::ReadScopeStatus()
{
    
    //return true; //for debug 

    if (!isConnected())
        return false;

    if (isSimulation())
    {
        mountSim();
        return true;
    }

    switch (TrackState) {
	case SCOPE_IDLE:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> IDLE");		
	    break;
	case SCOPE_SLEWING:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> SLEWING");		
	    break;
	case SCOPE_TRACKING:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> TRACKING");		
	    break;
	case SCOPE_PARKING:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> PARKING");		
	    break;
	case SCOPE_PARKED:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> PARKED");		
	    break;
	default:
	    DEBUG(INDI::Logger::DBG_WARNING, "<ReadScopeStatus> UNDEFINED");		
	    break;
    }

    if (TrackState == SCOPE_SLEWING )
    {
        // Check if LX200 is done slewing
        if (isSlewComplete())
        {
            usleep(1000000); //Wait until :MS# finish
            if (IUFindSwitch(&CoordSP, "SYNC")->s == ISS_ON || IUFindSwitch(&CoordSP, "SLEW")->s == ISS_ON)  {
	            TrackState = SCOPE_IDLE;
	            DEBUG(INDI::Logger::DBG_WARNING, "Slew is complete. IDLE");
		    SetTrackEnabled(false);
	    } else {
	            TrackState = SCOPE_TRACKING;
	            DEBUG(INDI::Logger::DBG_WARNING, "Slew is complete. TRACKING");
		    SetTrackEnabled(true);
	    }
        }
    }
    else if (TrackState == SCOPE_PARKING)
    {
        // isSlewComplete() not work because is base on actual RA/DEC vs target RA/DEC. DO ALWAYS
        if (true || isSlewComplete()) 
        {
            SetParked(true);
	    TrackState = SCOPE_PARKED;
        }
    }

    if (getLX200RA(PortFD, &currentRA) < 0 || getLX200DEC(PortFD, &currentDEC) < 0)
    {
        EqNP.s = IPS_ALERT;
        IDSetNumber(&EqNP, "Error reading RA/DEC.");
        return false;
    }

    NewRaDec(currentRA, currentDEC);

    //sendScopeTime();
    //syncSideOfPier();

    return true;
}

void ioptronHC8406::mountSim()
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



int ioptronHC8406::setioptronHC8406GuideRate(int rate)
{
	return setMoveRate(rate,USE_GUIDE_SPEED);
}

int ioptronHC8406::setioptronHC8406CenterRate(int rate)
{
	return setMoveRate(rate,USE_CENTERING_SPEED);
}

int ioptronHC8406::setioptronHC8406SlewRate(int rate)
{
	return setMoveRate(rate,USE_SLEW_SPEED);
}

int ioptronHC8406::setioptronHC8406CursorMoveSpeed(int type)
{
	return setMoveRate(-1,type);
}

int ioptronHC8406::setMoveRate(int rate,int move_type) 
{
    char cmd[16];
    int errcode = 0;
    char errmsg[MAXRBUF];
    int nbytes_written = 0;

    if (isSimulation())
    {
        return 0;
    }

    if (rate>=0)
    {
	    switch (move_type)
	    {
	    case USE_GUIDE_SPEED:
		snprintf(cmd, 16, ":RG%0d#", rate);
        	break;
	    case USE_CENTERING_SPEED:
		snprintf(cmd, 16, ":RC%0d#", rate);
        	break;
	    case USE_SLEW_SPEED:
		snprintf(cmd, 16, ":RS%0d#", rate);  //NOT WORK!!
        	break;

	    default:
        	break;
	    }
    } else {
	    switch (move_type)
	    {
	    case USE_GUIDE_SPEED:
		snprintf(cmd, 16, ":RG#");
        	break;
	    case USE_CENTERING_SPEED:
		snprintf(cmd, 16, ":RC#"); //NOT WORK!!
        	break;
	    case USE_SLEW_SPEED:
		snprintf(cmd, 16, ":RS#"); //NOT WORK!!
        	break;
	    default:
        	break;
	    }
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD (%s)", cmd);


    tcflush(PortFD, TCIFLUSH);

    if ((errcode = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        tty_error_msg(errcode, errmsg, MAXRBUF);
        DEBUGF(INDI::Logger::DBG_ERROR, "%s", errmsg);
        return errcode;
    }

    return 0;
}


void ioptronHC8406::syncSideOfPier()
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

bool ioptronHC8406::saveConfigItems(FILE *fp)
{
    LX200Generic::saveConfigItems(fp);

    IUSaveConfigSwitch(fp, &SyncCMRSP);

    return true;
}

int ioptronHC8406::getCommandString(int fd, char *data, const char *cmd)
{
    char *term;
    int error_type;
    int nbytes_write = 0, nbytes_read = 0;


    if ((error_type = tty_write_string(fd, cmd, &nbytes_write)) != TTY_OK)
        return error_type;

    error_type = tty_read_section(fd, data, '#', ioptronHC8406_TIMEOUT, &nbytes_read);
    tcflush(fd, TCIFLUSH);

    if (error_type != TTY_OK)
        return error_type;

    term = strchr(data, '#');
    if (term)
        *term = '\0';



    return 0;

}

void ioptronHC8406::sendScopeTime()
{
    char cdate[32]={0};
    double ctime;
    int h, m, s;
    int utc_h, utc_m, utc_s;
    double lx200_utc_offset = 0;
    char utc_offset_res[32]={0};
    int day, month, year, result;
    struct tm ltm;
    struct tm utm;
    time_t time_epoch;

    if (isSimulation())
    {
        snprintf(cdate, 32, "%d-%02d-%02dT%02d:%02d:%02d", 1979, 6, 25, 3, 30, 30);
        IDLog("Telescope ISO date and time: %s\n", cdate);
        IUSaveText(&TimeT[0], cdate);
        IUSaveText(&TimeT[1], "3");
        IDSetText(&TimeTP, nullptr);
        return;
    }

    //getCommandSexa(PortFD, &lx200_utc_offset, ":GG#");
    //tcflush(PortFD, TCIOFLUSH);
    getCommandString(PortFD, utc_offset_res, ":GG#");

    f_scansexa(utc_offset_res,&lx200_utc_offset);
    result = sscanf(utc_offset_res, "%d%*c%d%*c%d", &utc_h, &utc_m, &utc_s);
    if (result != 3)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error reading UTC offset from Telescope.");
        return;
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "<VAL> UTC offset: %d:%d:%d --->%g",utc_h,utc_m, utc_s, lx200_utc_offset);
    // LX200 TimeT Offset is defined at the number of hours added to LOCAL TIME to get TimeT. This is contrary to the normal definition.
    DEBUGF(INDI::Logger::DBG_DEBUG, "<VAL> UTC offset str: %s",utc_offset_res);
    IUSaveText(&TimeT[1], utc_offset_res);
    //IUSaveText(&TimeT[1], lx200_utc_offset);

    getLocalTime24(PortFD, &ctime);
    getSexComponents(ctime, &h, &m, &s);

    getCalendarDate(PortFD, cdate);
    result = sscanf(cdate, "%d%*c%d%*c%d", &year, &month, &day);
    if (result != 3)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error reading date from Telescope.");
        return;
    }

    // Let's fill in the local time
    ltm.tm_sec  = s;
    ltm.tm_min  = m;
    ltm.tm_hour = h;
    ltm.tm_mday = day;
    ltm.tm_mon  = month - 1;
    ltm.tm_year = year - 1900;

    // Get time epoch
    time_epoch = mktime(&ltm);

    // Convert to TimeT
    //time_epoch -= (int)(atof(TimeT[1].text) * 3600.0);
    time_epoch -= (int)(lx200_utc_offset * 3600.0);

    // Get UTC (we're using localtime_r, but since we shifted time_epoch above by UTCOffset, we should be getting the real UTC time
    localtime_r(&time_epoch, &utm);

    /* Format it into ISO 8601 */
    strftime(cdate, 32, "%Y-%m-%dT%H:%M:%S", &utm);
    IUSaveText(&TimeT[0], cdate);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Mount controller Local Time: %02d:%02d:%02d", h, m, s);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Mount controller UTC Time: %s", TimeT[0].text);

    // Let's send everything to the client
    IDSetText(&TimeTP, nullptr);
}

int ioptronHC8406::SendPulseCmd(int direction, int Tduration_msec)
{
    DEBUGF(INDI::Logger::DBG_DEBUG, "<%s>", __FUNCTION__);
    int rc = 0,  nbytes_written = 0;
    char cmd[20];
    int duration_msec,Rduration;
    if (Tduration_msec >=1000) {
	    duration_msec=999;    		    //limited to 999
	    Rduration=Tduration_msec-duration_msec; //pending ms
    } else {
	    duration_msec=Tduration_msec;
	    Rduration=0;
       	    DEBUGF(INDI::Logger::DBG_DEBUG, "Pulse %d <999 Sent only one",Tduration_msec);
    }

    switch (direction)
    {
        case LX200_NORTH:
            sprintf(cmd, ":Mn%03d#", duration_msec);
            break;
        case LX200_SOUTH:
            sprintf(cmd, ":Ms%03d#", duration_msec);
            break;
        case LX200_EAST:
            sprintf(cmd, ":Me%03d#", duration_msec);
            break;
        case LX200_WEST:
            sprintf(cmd, ":Mw%03d#", duration_msec);
            break;
        default:
            return 1;
    }
    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD <%s>", cmd);

    if ((rc = tty_write(PortFD, cmd, strlen(cmd), &nbytes_written)) != TTY_OK)
    {
        char errmsg[256];
        tty_error_msg(rc, errmsg, 256);
        DEBUGF(INDI::Logger::DBG_ERROR, "Error writing to device %s (%d)", errmsg, rc);
        return 1;
    }
    tcflush(PortFD, TCIFLUSH);

    if (Rduration!=0) {
    	DEBUGF(INDI::Logger::DBG_DEBUG, "pulse guide. Pulse >999. ms left:%d",Rduration);
        usleep(1000000);   //wait until the previous one has fineshed
        return SendPulseCmd(direction,Rduration);
    }
    return 0;
}

