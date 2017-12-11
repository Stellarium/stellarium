/*
    Astro-Electronic FS-2 Driver
    Copyright (C) 2015 Jasem Mutlaq (mutlaqja@ikarustech.com)

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

#include "lx200fs2.h"

#include "indicom.h"

#include <libnova/transform.h>

#include <cmath>
#include <cstring>

LX200FS2::LX200FS2() : LX200Generic()
{
    setVersion(2, 1);

    SetTelescopeCapability(
        TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_HAS_LOCATION | TELESCOPE_CAN_ABORT, 4);
}

bool LX200FS2::initProperties()
{
    LX200Generic::initProperties();

    IUFillNumber(&SlewAccuracyN[0], "SlewRA", "RA (arcmin)", "%10.6m", 0., 60., 1., 3.0);
    IUFillNumber(&SlewAccuracyN[1], "SlewDEC", "Dec (arcmin)", "%10.6m", 0., 60., 1., 3.0);
    IUFillNumberVector(&SlewAccuracyNP, SlewAccuracyN, NARRAY(SlewAccuracyN), getDeviceName(), "Slew Accuracy", "",
                       OPTIONS_TAB, IP_RW, 0, IPS_IDLE);

    SetParkDataType(PARK_AZ_ALT);

    return true;
}

bool LX200FS2::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        defineSwitch(&SlewRateSP);
        defineNumber(&SlewAccuracyNP);

        if (InitPark())
        {
            // If loading parking data is successful, we just set the default parking values.
            SetAxis1ParkDefault(0);
            SetAxis2ParkDefault(LocationN[LOCATION_LATITUDE].value);
        }
        else
        {
            // Otherwise, we set all parking data to default in case no parking data is found.
            SetAxis1Park(0);
            SetAxis2Park(LocationN[LOCATION_LATITUDE].value);
            SetAxis1ParkDefault(0);
            SetAxis2ParkDefault(LocationN[LOCATION_LATITUDE].value);
        }
    }
    else
    {
        deleteProperty(SlewRateSP.name);
        deleteProperty(SlewAccuracyNP.name);
    }

    return true;
}

bool LX200FS2::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, SlewAccuracyNP.name))
        {
            if (IUUpdateNumber(&SlewAccuracyNP, values, names, n) < 0)
                return false;

            SlewAccuracyNP.s = IPS_OK;

            if (SlewAccuracyN[0].value < 3 || SlewAccuracyN[1].value < 3)
                IDSetNumber(&SlewAccuracyNP, "Warning: Setting the slew accuracy too low may result in a dead lock");

            IDSetNumber(&SlewAccuracyNP, nullptr);
            return true;
        }
    }

    return LX200Generic::ISNewNumber(dev, name, values, names, n);
}

const char *LX200FS2::getDefaultName()
{
    return (const char *)"Astro-Electronic FS-2";
}

bool LX200FS2::isSlewComplete()
{
    const double dx = targetRA - currentRA;
    const double dy = targetDEC - currentDEC;
    return fabs(dx) <= (SlewAccuracyN[0].value / (900.0)) && fabs(dy) <= (SlewAccuracyN[1].value / 60.0);
}

bool LX200FS2::checkConnection()
{
    return true;
}

bool LX200FS2::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &SlewAccuracyNP);

    return true;
}

bool LX200FS2::Park()
{
    double parkAZ  = GetAxis1Park();
    double parkAlt = GetAxis2Park();

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, parkAZ, 2, 3600);
    fs_sexa(AltStr, parkAlt, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Parking to Az (%s) Alt (%s)...", AzStr, AltStr);

    ln_hrz_posn horizontalPos;
    // Libnova south = 0, west = 90, north = 180, east = 270
    horizontalPos.az = parkAZ + 180;
    if (horizontalPos.az >= 360)
        horizontalPos.az -= 360;
    horizontalPos.alt = parkAlt;

    ln_lnlat_posn observer;

    observer.lat = LocationN[LOCATION_LATITUDE].value;
    observer.lng = LocationN[LOCATION_LONGITUDE].value;

    if (observer.lng > 180)
        observer.lng -= 360;

    ln_equ_posn equatorialPos;

    ln_get_equ_from_hrz(&horizontalPos, &observer, ln_get_julian_from_sys(), &equatorialPos);

    char RAStr[16], DEStr[16];
    fs_sexa(RAStr, equatorialPos.ra / 15.0, 2, 3600);
    fs_sexa(DEStr, equatorialPos.dec, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Parking to RA (%s) DEC (%s)...", RAStr, DEStr);

    if (Goto(equatorialPos.ra / 15.0, equatorialPos.dec))
    {
        TrackState = SCOPE_PARKING;
        DEBUG(INDI::Logger::DBG_SESSION, "Parking is in progress...");

        return true;
    }
    else
        return false;
}

bool LX200FS2::UnPark()
{
    double parkAZ  = GetAxis1Park();
    double parkAlt = GetAxis2Park();

    char AzStr[16], AltStr[16];
    fs_sexa(AzStr, parkAZ, 2, 3600);
    fs_sexa(AltStr, parkAlt, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Unparking from Az (%s) Alt (%s)...", AzStr, AltStr);

    ln_hrz_posn horizontalPos;
    // Libnova south = 0, west = 90, north = 180, east = 270
    horizontalPos.az = parkAZ + 180;
    if (horizontalPos.az >= 360)
        horizontalPos.az -= 360;
    horizontalPos.alt = parkAlt;

    ln_lnlat_posn observer;

    observer.lat = LocationN[LOCATION_LATITUDE].value;
    observer.lng = LocationN[LOCATION_LONGITUDE].value;

    if (observer.lng > 180)
        observer.lng -= 360;

    ln_equ_posn equatorialPos;

    ln_get_equ_from_hrz(&horizontalPos, &observer, ln_get_julian_from_sys(), &equatorialPos);

    char RAStr[16], DEStr[16];
    fs_sexa(RAStr, equatorialPos.ra / 15.0, 2, 3600);
    fs_sexa(DEStr, equatorialPos.dec, 2, 3600);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Syncing to parked coordinates RA (%s) DEC (%s)...", RAStr, DEStr);

    if (Sync(equatorialPos.ra / 15.0, equatorialPos.dec))
    {
        SetParked(false);
        return true;
    }
    else
        return false;
}

bool LX200FS2::SetCurrentPark()
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

bool LX200FS2::SetDefaultPark()
{
    // By defualt azimuth 0
    SetAxis1Park(0);

    // Altitude = latitude of observer
    SetAxis2Park(LocationN[LOCATION_LATITUDE].value);

    return true;
}

bool LX200FS2::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(latitude);
    INDI_UNUSED(longitude);
    INDI_UNUSED(elevation);
    return true;
}
