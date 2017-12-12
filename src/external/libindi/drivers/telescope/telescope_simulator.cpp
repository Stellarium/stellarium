/*******************************************************************************
 Copyright(c) 2015 Jasem Mutlaq. All rights reserved.

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

#include "telescope_simulator.h"

#include "indicom.h"

#include <libnova/sidereal_time.h>
#include <libnova/transform.h>

#include <cmath>
#include <cstring>
#include <memory>

// We declare an auto pointer to ScopeSim.
std::unique_ptr<ScopeSim> telescope_sim(new ScopeSim());

#define GOTO_RATE      6.5      /* slew rate, degrees/s */
#define SLEW_RATE      2.5      /* slew rate, degrees/s */
#define FINE_SLEW_RATE 0.5      /* slew rate, degrees/s */

#define GOTO_LIMIT      5       /* Move at GOTO_RATE until distance from target is GOTO_LIMIT degrees */
#define SLEW_LIMIT      1       /* Move at SLEW_LIMIT until distance from target is SLEW_LIMIT degrees */

#define POLLMS 250 /* poll period, ms */

#define RA_AXIS     0
#define DEC_AXIS    1
#define GUIDE_NORTH 0
#define GUIDE_SOUTH 1
#define GUIDE_WEST  0
#define GUIDE_EAST  1

#define MIN_AZ_FLIP 180
#define MAX_AZ_FLIP 200

void ISPoll(void *p);

void ISGetProperties(const char *dev)
{
    telescope_sim->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    telescope_sim->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    telescope_sim->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    telescope_sim->ISNewNumber(dev, name, values, names, n);
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
    telescope_sim->ISSnoopDevice(root);
}

ScopeSim::ScopeSim()
{
    DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                               TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_TRACK_MODE | TELESCOPE_CAN_CONTROL_TRACK | TELESCOPE_HAS_TRACK_RATE,
                           4);

    /* initialize random seed: */
    srand(time(nullptr));
}

const char *ScopeSim::getDefaultName()
{
    return (const char *)"Telescope Simulator";
}

bool ScopeSim::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    /* Simulated periodic error in RA, DEC */
    IUFillNumber(&EqPEN[RA_AXIS], "RA_PE", "RA (hh:mm:ss)", "%010.6m", 0, 24, 0, 15.);
    IUFillNumber(&EqPEN[DEC_AXIS], "DEC_PE", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 15.);
    IUFillNumberVector(&EqPENV, EqPEN, 2, getDeviceName(), "EQUATORIAL_PE", "Periodic Error", MOTION_TAB, IP_RO, 60,
                       IPS_IDLE);

    /* Enable client to manually add periodic error northward or southward for simulation purposes */
    IUFillSwitch(&PEErrNSS[DIRECTION_NORTH], "PE_N", "North", ISS_OFF);
    IUFillSwitch(&PEErrNSS[DIRECTION_SOUTH], "PE_S", "South", ISS_OFF);
    IUFillSwitchVector(&PEErrNSSP, PEErrNSS, 2, getDeviceName(), "PE_NS", "PE N/S", MOTION_TAB, IP_RW, ISR_ATMOST1, 60,
                       IPS_IDLE);

    /* Enable client to manually add periodic error westward or easthward for simulation purposes */
    IUFillSwitch(&PEErrWES[DIRECTION_WEST], "PE_W", "West", ISS_OFF);
    IUFillSwitch(&PEErrWES[DIRECTION_EAST], "PE_E", "East", ISS_OFF);
    IUFillSwitchVector(&PEErrWESP, PEErrWES, 2, getDeviceName(), "PE_WE", "PE W/E", MOTION_TAB, IP_RW, ISR_ATMOST1, 60,
                       IPS_IDLE);

    /* How fast do we guide compared to sidereal rate */
    IUFillNumber(&GuideRateN[RA_AXIS], "GUIDE_RATE_WE", "W/E Rate", "%g", 0, 1, 0.1, 0.3);
    IUFillNumber(&GuideRateN[DEC_AXIS], "GUIDE_RATE_NS", "N/S Rate", "%g", 0, 1, 0.1, 0.3);
    IUFillNumberVector(&GuideRateNP, GuideRateN, 2, getDeviceName(), "GUIDE_RATE", "Guiding Rate", MOTION_TAB, IP_RW, 0,
                       IPS_IDLE);

    IUFillSwitch(&SlewRateS[SLEW_GUIDE], "SLEW_GUIDE", "Guide", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_CENTERING], "SLEW_CENTERING", "Centering", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_FIND], "SLEW_FIND", "Find", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_MAX], "SLEW_MAX", "Max", ISS_ON);
    IUFillSwitchVector(&SlewRateSP, SlewRateS, 4, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Add Tracking Modes
    AddTrackMode("TRACK_SIDEREAL", "Sidereal", true);
    AddTrackMode("TRACK_CUSTOM", "Custom");

    // Let's simulate it to be an F/7.5 120mm telescope
    ScopeParametersN[0].value = 120;
    ScopeParametersN[1].value = 900;
    ScopeParametersN[2].value = 120;
    ScopeParametersN[3].value = 900;

    TrackState = SCOPE_IDLE;

    SetParkDataType(PARK_RA_DEC);

    initGuiderProperties(getDeviceName(), MOTION_TAB);

    /* Add debug controls so we may debug driver if necessary */
    addDebugControl();

    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);

    double longitude=0, latitude=90;
    // Get value from config file if it exists.
    IUGetConfigNumber(getDeviceName(), "GEOGRAPHIC_COORD", "LONG", &longitude);
    currentRA  = get_local_sidereal_time(longitude);
    IUGetConfigNumber(getDeviceName(), "GEOGRAPHIC_COORD", "LAT", &latitude);
    currentDEC = latitude > 0 ? 90 : -90;

    return true;
}

void ScopeSim::ISGetProperties(const char *dev)
{
    /* First we let our parent populate */
    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
        defineNumber(&GuideRateNP);
        defineNumber(&EqPENV);
        defineSwitch(&PEErrNSSP);
        defineSwitch(&PEErrWESP);
    }
}

bool ScopeSim::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);
        defineNumber(&GuideRateNP);
        defineNumber(&EqPENV);
        defineSwitch(&PEErrNSSP);
        defineSwitch(&PEErrWESP);

        if (InitPark())
        {
            // If loading parking data is successful, we just set the default parking values.
            SetAxis1ParkDefault(currentRA);
            SetAxis2ParkDefault(currentDEC);

            if (isParked())
            {
                currentRA = ParkPositionN[AXIS_RA].value;
                currentDEC= ParkPositionN[AXIS_DE].value;
            }
        }
        else
        {
            // Otherwise, we set all parking data to default in case no parking data is found.
            SetAxis1Park(currentRA);
            SetAxis2Park(currentDEC);
            SetAxis1ParkDefault(currentRA);
            SetAxis2ParkDefault(currentDEC);
        }
    }
    else
    {
        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);
        deleteProperty(EqPENV.name);
        deleteProperty(PEErrNSSP.name);
        deleteProperty(PEErrWESP.name);
        deleteProperty(GuideRateNP.name);
    }

    return true;
}

bool ScopeSim::Connect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Telescope simulator is online.");
    SetTimer(POLLMS);
    return true;
}

bool ScopeSim::Disconnect()
{
    DEBUG(INDI::Logger::DBG_SESSION, "Telescope simulator is offline.");
    return true;
}

bool ScopeSim::ReadScopeStatus()
{
    static struct timeval ltv { 0, 0 };
    struct timeval tv { 0, 0 };
    double dt = 0, da_ra = 0, da_dec = 0, dx = 0, dy = 0, ra_guide_dt = 0, dec_guide_dt = 0;
    static double last_dx = 0, last_dy = 0;
    int nlocked, ns_guide_dir = -1, we_guide_dir = -1;
    char RA_DISP[64], DEC_DISP[64], RA_GUIDE[64], DEC_GUIDE[64], RA_PE[64], DEC_PE[64], RA_TARGET[64], DEC_TARGET[64];

    /* update elapsed time since last poll, don't presume exactly POLLMS */
    gettimeofday(&tv, nullptr);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
        ltv = tv;

    dt  = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec) / 1e6;
    ltv = tv;

    if (fabs(targetRA - currentRA) * 15. >= GOTO_LIMIT)
        da_ra = GOTO_RATE * dt;
    else if (fabs(targetRA - currentRA) * 15. >= SLEW_LIMIT)
        da_ra = SLEW_RATE * dt;
    else
        da_ra = FINE_SLEW_RATE * dt;

    if (fabs(targetDEC - currentDEC) >= GOTO_LIMIT)
        da_dec = GOTO_RATE * dt;
    else if (fabs(targetDEC - currentDEC) >= SLEW_LIMIT)
        da_dec = SLEW_RATE * dt;
    else
        da_dec = FINE_SLEW_RATE * dt;

    if (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY)
    {
        int rate = IUFindOnSwitchIndex(&SlewRateSP);

        switch (rate)
        {
            case SLEW_GUIDE:
                da_ra  = FINE_SLEW_RATE * dt * 0.05;
                da_dec = FINE_SLEW_RATE * dt * 0.05;
                break;

            case SLEW_CENTERING:
                da_ra  = FINE_SLEW_RATE * dt * .1;
                da_dec = FINE_SLEW_RATE * dt * .1;
                break;

            case SLEW_FIND:
                da_ra  = SLEW_RATE * dt;
                da_dec = SLEW_RATE * dt;
                break;

            default:
                da_ra  = GOTO_RATE * dt;
                da_dec = GOTO_RATE * dt;
                break;
        }

        switch (MovementNSSP.s)
        {
            case IPS_BUSY:
                if (MovementNSS[DIRECTION_NORTH].s == ISS_ON)
                    currentDEC += da_dec;
                else if (MovementNSS[DIRECTION_SOUTH].s == ISS_ON)
                    currentDEC -= da_dec;
                break;

            default:
                break;
        }

        switch (MovementWESP.s)
        {
            case IPS_BUSY:

                if (MovementWES[DIRECTION_WEST].s == ISS_ON)
                    currentRA += da_ra / 15.;
                else if (MovementWES[DIRECTION_EAST].s == ISS_ON)
                    currentRA -= da_ra / 15.;
                break;

            default:
                break;
        }

        NewRaDec(currentRA, currentDEC);
        return true;
    }

    /* Process per current state. We check the state of EQUATORIAL_EOD_COORDS_REQUEST and act acoordingly */
    switch (TrackState)
    {
        /*case SCOPE_IDLE:
            EqNP.s = IPS_IDLE;
            break;*/
        case SCOPE_SLEWING:
        case SCOPE_PARKING:
            /* slewing - nail it when both within one pulse @ SLEWRATE */
            nlocked = 0;

            dx = targetRA - currentRA;

            // Always take the shortcut, don't go all around the globe
            // If the difference between target and current is more than 12 hours, then we need to take the shortest path
            if (dx > 12)
                dx -= 24;
            else if (dx < -12)
                dx += 24;

            // In meridian flip, alway force eastward motion (increasing RA) until target is reached.
            if (forceMeridianFlip)
            {
                dx = fabs(dx);
                if (dx == 0)
                {
                    dx    = 1;
                    da_ra = GOTO_LIMIT;
                }
            }

            if (fabs(dx) * 15. <= da_ra)
            {
                currentRA = targetRA;
                nlocked++;
            }
            else if (dx > 0)
                currentRA += da_ra / 15.;
            else
                currentRA -= da_ra / 15.;

            currentRA = range24(currentRA);

            dy = targetDEC - currentDEC;
            if (fabs(dy) <= da_dec)
            {
                currentDEC = targetDEC;
                nlocked++;
            }
            else if (dy > 0)
                currentDEC += da_dec;
            else
                currentDEC -= da_dec;

            EqNP.s = IPS_BUSY;

            if (nlocked == 2)
            {
                forceMeridianFlip = false;

                if (TrackState == SCOPE_SLEWING)
                {
                    // Initially no PE in both axis.
                    EqPEN[0].value = currentRA;
                    EqPEN[1].value = currentDEC;

                    IDSetNumber(&EqPENV, nullptr);

                    TrackState = SCOPE_TRACKING;

                    EqNP.s = IPS_OK;
                    DEBUG(INDI::Logger::DBG_SESSION, "Telescope slew is complete. Tracking...");
                }
                else
                {
                    SetParked(true);
                    EqNP.s = IPS_IDLE;
                }
            }

            break;

        case SCOPE_IDLE:
                 //currentRA += (TRACKRATE_SIDEREAL/3600.0 * dt) / 15.0;
                currentRA += (TrackRateN[AXIS_RA].value/3600.0 * dt) / 15.0;
                currentRA = range24(currentRA);
        break;

        case SCOPE_TRACKING:
            // In case of custom tracking rate
            if (TrackModeS[1].s == ISS_ON)
            {
                currentRA  += ( ((TRACKRATE_SIDEREAL/3600.0) - (TrackRateN[AXIS_RA].value/3600.0)) * dt) / 15.0;
                currentDEC += ( (TrackRateN[AXIS_DE].value/3600.0) * dt);
            }

            dt *= 1000;

            if (guiderNSTarget[GUIDE_NORTH] > 0)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "Commanded to GUIDE NORTH for %g ms", guiderNSTarget[GUIDE_NORTH]);
                ns_guide_dir = GUIDE_NORTH;
            }
            else if (guiderNSTarget[GUIDE_SOUTH] > 0)
            {
                DEBUGF(INDI::Logger::DBG_DEBUG, "Commanded to GUIDE SOUTH for %g ms", guiderNSTarget[GUIDE_SOUTH]);
                ns_guide_dir = GUIDE_SOUTH;
            }

            // WE Guide Selection
            if (guiderEWTarget[GUIDE_WEST] > 0)
            {
                we_guide_dir = GUIDE_WEST;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Commanded to GUIDE WEST for %g ms", guiderEWTarget[GUIDE_WEST]);
            }
            else if (guiderEWTarget[GUIDE_EAST] > 0)
            {
                we_guide_dir = GUIDE_EAST;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Commanded to GUIDE EAST for %g ms", guiderEWTarget[GUIDE_EAST]);
            }

            if (ns_guide_dir != -1)
            {
                dec_guide_dt = TrackRateN[AXIS_RA].value/3600.0 * GuideRateN[DEC_AXIS].value * guiderNSTarget[ns_guide_dir] / 1000.0 *
                               (ns_guide_dir == GUIDE_NORTH ? 1 : -1);

                // If time remaining is more that dt, then decrement and
                if (guiderNSTarget[ns_guide_dir] >= dt)
                    guiderNSTarget[ns_guide_dir] -= dt;
                else
                    guiderNSTarget[ns_guide_dir] = 0;

                if (guiderNSTarget[ns_guide_dir] == 0)
                {
                    GuideNSNP.s = IPS_IDLE;
                    IDSetNumber(&GuideNSNP, nullptr);
                }

                EqPEN[DEC_AXIS].value += dec_guide_dt;
            }

            if (we_guide_dir != -1)
            {
                ra_guide_dt = (TrackRateN[AXIS_RA].value/3600.0) / 15.0 * GuideRateN[RA_AXIS].value * guiderEWTarget[we_guide_dir] / 1000.0 *
                              (we_guide_dir == GUIDE_WEST ? -1 : 1);

                if (guiderEWTarget[we_guide_dir] >= dt)
                    guiderEWTarget[we_guide_dir] -= dt;
                else
                    guiderEWTarget[we_guide_dir] = 0;

                if (guiderEWTarget[we_guide_dir] == 0)
                {
                    GuideWENP.s = IPS_IDLE;
                    IDSetNumber(&GuideWENP, nullptr);
                }

                EqPEN[RA_AXIS].value += ra_guide_dt;
            }

            //Mention the followng:
            // Current RA displacemet and direction
            // Current DEC displacement and direction
            // Amount of RA GUIDING correction and direction
            // Amount of DEC GUIDING correction and direction

            dx = EqPEN[RA_AXIS].value - targetRA;
            dy = EqPEN[DEC_AXIS].value - targetDEC;
            fs_sexa(RA_DISP, fabs(dx), 2, 3600);
            fs_sexa(DEC_DISP, fabs(dy), 2, 3600);

            fs_sexa(RA_GUIDE, fabs(ra_guide_dt), 2, 3600);
            fs_sexa(DEC_GUIDE, fabs(dec_guide_dt), 2, 3600);

            fs_sexa(RA_PE, EqPEN[RA_AXIS].value, 2, 3600);
            fs_sexa(DEC_PE, EqPEN[DEC_AXIS].value, 2, 3600);

            fs_sexa(RA_TARGET, targetRA, 2, 3600);
            fs_sexa(DEC_TARGET, targetDEC, 2, 3600);

            if (dx != last_dx || dy != last_dy || ra_guide_dt != 0.0 || dec_guide_dt != 0.0)
            {
                last_dx = dx;
                last_dy = dy;
                //DEBUGF(INDI::Logger::DBG_DEBUG, "dt is %g\n", dt);
                DEBUGF(INDI::Logger::DBG_DEBUG, "RA Displacement (%c%s) %s -- %s of target RA %s", dx >= 0 ? '+' : '-',
                       RA_DISP, RA_PE, (EqPEN[RA_AXIS].value - targetRA) > 0 ? "East" : "West", RA_TARGET);
                DEBUGF(INDI::Logger::DBG_DEBUG, "DEC Displacement (%c%s) %s -- %s of target RA %s", dy >= 0 ? '+' : '-',
                       DEC_DISP, DEC_PE, (EqPEN[DEC_AXIS].value - targetDEC) > 0 ? "North" : "South", DEC_TARGET);
                DEBUGF(INDI::Logger::DBG_DEBUG, "RA Guide Correction (%g) %s -- Direction %s", ra_guide_dt, RA_GUIDE,
                       ra_guide_dt > 0 ? "East" : "West");
                DEBUGF(INDI::Logger::DBG_DEBUG, "DEC Guide Correction (%g) %s -- Direction %s", dec_guide_dt, DEC_GUIDE,
                       dec_guide_dt > 0 ? "North" : "South");
            }

            if (ns_guide_dir != -1 || we_guide_dir != -1)
                IDSetNumber(&EqPENV, nullptr);

            break;

        default:
            break;
    }

    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, currentRA, 2, 3600);
    fs_sexa(DecStr, currentDEC, 2, 3600);

    DEBUGF(DBG_SCOPE, "Current RA: %s Current DEC: %s", RAStr, DecStr);

    NewRaDec(currentRA, currentDEC);
    return true;
}

bool ScopeSim::Goto(double r, double d)
{
    targetRA  = r;
    targetDEC = d;
    char RAStr[64], DecStr[64];

    fs_sexa(RAStr, targetRA, 2, 3600);
    fs_sexa(DecStr, targetDEC, 2, 3600);

    ln_equ_posn lnradec { 0, 0 };

    lnradec.ra  = (currentRA * 360) / 24.0;
    lnradec.dec = currentDEC;

    ln_get_hrz_from_equ(&lnradec, &lnobserver, ln_get_julian_from_sys(), &lnaltaz);
    /* libnova measures azimuth from south towards west */
    double current_az = range360(lnaltaz.az + 180);
    //double current_alt =lnaltaz.alt;

    if (current_az > MIN_AZ_FLIP && current_az < MAX_AZ_FLIP)
    {
        lnradec.ra  = (r * 360) / 24.0;
        lnradec.dec = d;

        ln_get_hrz_from_equ(&lnradec, &lnobserver, ln_get_julian_from_sys(), &lnaltaz);

        double target_az = range360(lnaltaz.az + 180);

        //if (targetAz > currentAz && target_az > MIN_AZ_FLIP && target_az < MAX_AZ_FLIP)
        if (target_az >= current_az && target_az > MIN_AZ_FLIP)
        {
            forceMeridianFlip = true;
        }
    }

    TrackState = SCOPE_SLEWING;

    EqNP.s = IPS_BUSY;

    DEBUGF(INDI::Logger::DBG_SESSION, "Slewing to RA: %s - DEC: %s", RAStr, DecStr);
    return true;
}

bool ScopeSim::Sync(double ra, double dec)
{
    currentRA  = ra;
    currentDEC = dec;

    EqPEN[RA_AXIS].value  = ra;
    EqPEN[DEC_AXIS].value = dec;
    IDSetNumber(&EqPENV, nullptr);

    DEBUG(INDI::Logger::DBG_SESSION, "Sync is successful.");

    EqNP.s = IPS_OK;

    NewRaDec(currentRA, currentDEC);

    return true;
}

bool ScopeSim::Park()
{
    targetRA   = GetAxis1Park();
    targetDEC  = GetAxis2Park();
    TrackState = SCOPE_PARKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Parking telescope in progress...");
    return true;
}

bool ScopeSim::UnPark()
{
    SetParked(false);
    return true;
}

bool ScopeSim::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, "GUIDE_RATE") == 0)
        {
            IUUpdateNumber(&GuideRateNP, values, names, n);
            GuideRateNP.s = IPS_OK;
            IDSetNumber(&GuideRateNP, nullptr);
            return true;
        }

        if (strcmp(name, GuideNSNP.name) == 0 || strcmp(name, GuideWENP.name) == 0)
        {
            processGuiderProperties(name, values, names, n);
            return true;
        }
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool ScopeSim::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Slew mode
        if (strcmp(name, SlewRateSP.name) == 0)
        {
            if (IUUpdateSwitch(&SlewRateSP, states, names, n) < 0)
                return false;

            SlewRateSP.s = IPS_OK;
            IDSetSwitch(&SlewRateSP, nullptr);
            return true;
        }

        if (strcmp(name, "PE_NS") == 0)
        {
            IUUpdateSwitch(&PEErrNSSP, states, names, n);

            PEErrNSSP.s = IPS_OK;

            if (PEErrNSS[DIRECTION_NORTH].s == ISS_ON)
            {
                EqPEN[DEC_AXIS].value += TRACKRATE_SIDEREAL/3600.0 * GuideRateN[DEC_AXIS].value;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Simulating PE in NORTH direction for value of %g", TRACKRATE_SIDEREAL/3600.0);
            }
            else
            {
                EqPEN[DEC_AXIS].value -= TRACKRATE_SIDEREAL/3600.0 * GuideRateN[DEC_AXIS].value;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Simulating PE in SOUTH direction for value of %g", TRACKRATE_SIDEREAL/3600.0);
            }

            IUResetSwitch(&PEErrNSSP);
            IDSetSwitch(&PEErrNSSP, nullptr);
            IDSetNumber(&EqPENV, nullptr);

            return true;
        }

        if (strcmp(name, "PE_WE") == 0)
        {
            IUUpdateSwitch(&PEErrWESP, states, names, n);

            PEErrWESP.s = IPS_OK;

            if (PEErrWES[DIRECTION_WEST].s == ISS_ON)
            {
                EqPEN[RA_AXIS].value -= TRACKRATE_SIDEREAL/3600.0 / 15. * GuideRateN[RA_AXIS].value;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Simulator PE in WEST direction for value of %g", TRACKRATE_SIDEREAL/3600.0);
            }
            else
            {
                EqPEN[RA_AXIS].value += TRACKRATE_SIDEREAL/3600.0 / 15. * GuideRateN[RA_AXIS].value;
                DEBUGF(INDI::Logger::DBG_DEBUG, "Simulator PE in EAST direction for value of %g", TRACKRATE_SIDEREAL/3600.0);
            }

            IUResetSwitch(&PEErrWESP);
            IDSetSwitch(&PEErrWESP, nullptr);
            IDSetNumber(&EqPENV, nullptr);

            return true;
        }
    }

    //  Nobody has claimed this, so, ignore it
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool ScopeSim::Abort()
{
    return true;
}

bool ScopeSim::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    if (TrackState == SCOPE_PARKED)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Please unpark the mount before issuing any motion commands.");
        return false;
    }

    return true;
}

bool ScopeSim::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    if (TrackState == SCOPE_PARKED)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Please unpark the mount before issuing any motion commands.");
        return false;
    }

    return true;
}

IPState ScopeSim::GuideNorth(float ms)
{
    guiderNSTarget[GUIDE_NORTH] = ms;
    guiderNSTarget[GUIDE_SOUTH] = 0;
    return IPS_BUSY;
}

IPState ScopeSim::GuideSouth(float ms)
{
    guiderNSTarget[GUIDE_SOUTH] = ms;
    guiderNSTarget[GUIDE_NORTH] = 0;
    return IPS_BUSY;
}

IPState ScopeSim::GuideEast(float ms)
{
    guiderEWTarget[GUIDE_EAST] = ms;
    guiderEWTarget[GUIDE_WEST] = 0;
    return IPS_BUSY;
}

IPState ScopeSim::GuideWest(float ms)
{
    guiderEWTarget[GUIDE_WEST] = ms;
    guiderEWTarget[GUIDE_EAST] = 0;
    return IPS_BUSY;
}

bool ScopeSim::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);
    // JM: INDI Longitude is 0 to 360 increasing EAST. libnova East is Positive, West is negative
    lnobserver.lng = longitude;

    if (lnobserver.lng > 180)
        lnobserver.lng -= 360;
    lnobserver.lat = latitude;

    DEBUGF(INDI::Logger::DBG_SESSION, "Location updated: Longitude (%g) Latitude (%g)", lnobserver.lng, lnobserver.lat);
    return true;
}

bool ScopeSim::SetCurrentPark()
{
    SetAxis1Park(currentRA);
    SetAxis2Park(currentDEC);

    return true;
}

bool ScopeSim::SetDefaultPark()
{
    // By default set RA to HA
    SetAxis1Park(get_local_sidereal_time(LocationN[LOCATION_LONGITUDE].value));

    // Set DEC to 90 or -90 depending on the hemisphere
    SetAxis2Park((LocationN[LOCATION_LATITUDE].value > 0) ? 90 : -90);

    return true;
}

bool ScopeSim::SetTrackMode(uint8_t mode)
{
    INDI_UNUSED(mode);
    return true;
}

bool ScopeSim::SetTrackEnabled(bool enabled)
{
    INDI_UNUSED(enabled);
    return true;
}

bool ScopeSim::SetTrackRate(double raRate, double deRate)
{
    INDI_UNUSED(raRate);
    INDI_UNUSED(deRate);
    return true;
}
