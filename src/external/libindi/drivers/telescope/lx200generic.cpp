#if 0
LX200 Generic
Copyright (C) 2003 - 2017 Jasem Mutlaq (mutlaqja@ikarustech.com)

This library is free software;
you can redistribute it and / or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation;
either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY;
without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library;
if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301  USA

2013 - 10 - 27:
    Updated driver to use INDI::Telescope (JM)
  2015 - 11 - 25:
  Use variable updatePeriodMS instead of static POLLMS

  #endif

#include "lx200generic.h"

#include "indicom.h"
#include "lx200_10micron.h"
#include "lx200_16.h"
#include "lx200_OnStep.h"
#include "lx200ap.h"
#include "lx200classic.h"
#include "lx200driver.h"
#include "lx200fs2.h"
#include "lx200gemini.h"
#include "lx200pulsar2.h"
#include "lx200ss2000pc.h"
#include "lx200zeq25.h"
#include "lx200gotonova.h"
#include "ioptronHC8406.h"

#include <libnova/sidereal_time.h>

#include <cmath>
#include <memory>
#include <cstring>
#include <unistd.h>
#include <time.h>

// We declare an auto pointer to LX200Generic.
std::unique_ptr<LX200Generic> telescope;

/* There is _one_ binary for all LX200 drivers, but each binary is renamed
** to its device name (i.e. lx200gps, lx200_16..etc). The main function will
** fetch from std args the binary name and ISInit will create the apporpiate
** device afterwards. If the binary name does not match any known devices,
** we simply create a generic device.
*/
extern char *me;

#define LX200_TRACK 0
#define LX200_SYNC  1

/* Simulation Parameters */
#define LX200_GENERIC_SLEWRATE 5        /* slew rate, degrees/s */
#define SIDRATE  0.004178 /* sidereal rate, degrees/s */

/* send client definitions of all properties */
void ISInit()
{
    static int isInit = 0;

    if (isInit)
        return;

    isInit = 1;

    if (strstr(me, "indi_lx200classic"))
    {
        IDLog("initializing from LX200 classic device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200Classic());
    }
    if (strstr(me, "indi_lx200_OnStep"))
    {
        IDLog("initializing from LX200 OnStep device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200_OnStep());
    }
    else if (strstr(me, "indi_lx200gps"))
    {
        IDLog("initializing from LX200 GPS device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200GPS());
    }
    else if (strstr(me, "indi_lx200_16"))
    {
        IDLog("Initializing from LX200 16 device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200_16());
    }
    else if (strstr(me, "indi_lx200autostar"))
    {
        IDLog("initializing from Autostar device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200Autostar());
    }
    else if (strstr(me, "indi_lx200ap"))
    {
        IDLog("initializing from Astrophysics device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200AstroPhysics());
    }
    else if (strstr(me, "indi_lx200gemini"))
    {
        IDLog("initializing from Losmandy Gemini device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200Gemini());
    }
    else if (strstr(me, "indi_lx200zeq25"))
    {
        IDLog("initializing from ZEQ25 device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200ZEQ25());
    }
    else if (strstr(me, "indi_lx200gotonova"))
    {
        IDLog("initializing from GotoNova device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200GotoNova());
    }
    else if (strstr(me, "indi_ioptronHC8406"))
    {
        IDLog("initializing from ioptron telescope Hand Controller HC8406 device...\n");

        if (telescope.get() == 0)
            telescope.reset(new ioptronHC8406());
    }
    else if (strstr(me, "indi_lx200pulsar2"))
    {
        IDLog("initializing from pulsar2 device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200Pulsar2());
    }
    else if (strstr(me, "indi_lx200ss2000pc"))
    {
        IDLog("initializing from skysensor2000pc device...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200SS2000PC());
    }
    else if (strstr(me, "indi_lx200fs2"))
    {
        IDLog("initializing from Astro-Electronic FS-2...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200FS2());
    }
    else if (strstr(me, "indi_lx200_10micron"))
    {
        IDLog("initializing for 10Micron mount...\n");

        if (telescope.get() == 0)
            telescope.reset(new LX200_10MICRON());
    }
    // be nice and give them a generic device
    else if (telescope.get() == 0)
        telescope.reset(new LX200Generic());
}

void ISGetProperties(const char *dev)
{
    ISInit();
    telescope->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    ISInit();
    telescope->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ISInit();
    telescope->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    ISInit();
    telescope->ISNewNumber(dev, name, values, names, n);
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
    ISInit();
    telescope->ISSnoopDevice(root);
}

/**************************************************
*** LX200 Generic Implementation
***************************************************/

LX200Generic::LX200Generic()
{
    setVersion(2, 0);

    currentSiteNum = 1;
    trackingMode   = LX200_TRACK_SIDEREAL;
    GuideNSTID     = 0;
    GuideWETID     = 0;

    DBG_SCOPE = INDI::Logger::getInstance().addDebugLevel("Scope Verbose", "SCOPE");

    setLX200Capability(LX200_HAS_FOCUS | LX200_HAS_TRACKING_FREQ | LX200_HAS_ALIGNMENT_TYPE | LX200_HAS_SITES |
                       LX200_HAS_PULSE_GUIDING);

    SetTelescopeCapability(TELESCOPE_CAN_PARK | TELESCOPE_CAN_SYNC | TELESCOPE_CAN_GOTO | TELESCOPE_CAN_ABORT |
                           TELESCOPE_HAS_TIME | TELESCOPE_HAS_LOCATION | TELESCOPE_HAS_TRACK_MODE,
                           4);

    DEBUG(INDI::Logger::DBG_DEBUG, "Initializing from Generic LX200 device...");
}


void LX200Generic::debugTriggered(bool enable)
{
    INDI_UNUSED(enable);
    setLX200Debug(getDeviceName(), DBG_SCOPE);
}

const char *LX200Generic::getDefaultName()
{
    return (const char *)"LX200 Generic";
}

const char *LX200Generic::getDriverName()
{
    return LX200Generic::getDefaultName();
}

bool LX200Generic::initProperties()
{
    /* Make sure to init parent properties first */
    INDI::Telescope::initProperties();

    IUFillSwitch(&AlignmentS[0], "Polar", "", ISS_ON);
    IUFillSwitch(&AlignmentS[1], "AltAz", "", ISS_OFF);
    IUFillSwitch(&AlignmentS[2], "Land", "", ISS_OFF);
    IUFillSwitchVector(&AlignmentSP, AlignmentS, 3, getDeviceName(), "Alignment", "", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

#if 0
    IUFillSwitch(&SlewRateS[SLEW_GUIDE], "SLEW_GUIDE", "Guide", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_CENTERING], "SLEW_CENTERING", "Centering", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_FIND], "SLEW_FIND", "Find", ISS_OFF);
    IUFillSwitch(&SlewRateS[SLEW_MAX], "SLEW_MAX", "Max", ISS_ON);
    IUFillSwitchVector(&SlewRateSP, SlewRateS, 4, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate", MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
#endif

#if 0
    IUFillSwitch(&TrackModeS[0], "TRACK_SIDEREAL", "Sidereal", ISS_ON);
    IUFillSwitch(&TrackModeS[1], "TRACK_SOLAR", "Solar", ISS_OFF);
    IUFillSwitch(&TrackModeS[2], "TRACK_LUNAR", "Lunar", ISS_OFF);
    IUFillSwitch(&TrackModeS[3], "TRACK_CUSTOM", "Custom", ISS_OFF);
    IUFillSwitchVector(&TrackModeSP, TrackModeS, 4, getDeviceName(), "TELESCOPE_TRACK_MODE", "Track Mode", MOTION_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
#endif

    AddTrackMode("TRACK_SIDEREAL", "Sidereal", true);
    AddTrackMode("TRACK_SOLAR", "Solar");
    AddTrackMode("TRACK_LUNAR", "Lunar");
    AddTrackMode("TRACK_CUSTOM", "Custom");

    IUFillNumber(&TrackFreqN[0], "trackFreq", "Freq", "%g", 56.4, 60.1, 0.1, 60.1);
    IUFillNumberVector(&TrackingFreqNP, TrackFreqN, 1, getDeviceName(), "Tracking Frequency", "", MOTION_TAB, IP_RW, 0,
                       IPS_IDLE);

    IUFillSwitch(&UsePulseCmdS[0], "Off", "", ISS_ON);
    IUFillSwitch(&UsePulseCmdS[1], "On", "", ISS_OFF);
    IUFillSwitchVector(&UsePulseCmdSP, UsePulseCmdS, 2, getDeviceName(), "Use Pulse Cmd", "", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&SiteS[0], "Site 1", "", ISS_ON);
    IUFillSwitch(&SiteS[1], "Site 2", "", ISS_OFF);
    IUFillSwitch(&SiteS[2], "Site 3", "", ISS_OFF);
    IUFillSwitch(&SiteS[3], "Site 4", "", ISS_OFF);
    IUFillSwitchVector(&SiteSP, SiteS, 4, getDeviceName(), "Sites", "", SITE_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillText(&SiteNameT[0], "Name", "", "");
    IUFillTextVector(&SiteNameTP, SiteNameT, 1, getDeviceName(), "Site Name", "", SITE_TAB, IP_RW, 0, IPS_IDLE);

    IUFillSwitch(&FocusMotionS[0], "IN", "Focus in", ISS_OFF);
    IUFillSwitch(&FocusMotionS[1], "OUT", "Focus out", ISS_OFF);
    IUFillSwitchVector(&FocusMotionSP, FocusMotionS, 2, getDeviceName(), "FOCUS_MOTION", "Motion", FOCUS_TAB, IP_RW,
                       ISR_ATMOST1, 0, IPS_IDLE);

    IUFillNumber(&FocusTimerN[0], "TIMER", "Timer (ms)", "%g", 0, 10000., 1000., 0);
    IUFillNumberVector(&FocusTimerNP, FocusTimerN, 1, getDeviceName(), "FOCUS_TIMER", "Focus Timer", FOCUS_TAB, IP_RW,
                       0, IPS_IDLE);

    IUFillSwitch(&FocusModeS[0], "FOCUS_HALT", "Halt", ISS_ON);
    IUFillSwitch(&FocusModeS[1], "FOCUS_SLOW", "Slow", ISS_OFF);
    IUFillSwitch(&FocusModeS[2], "FOCUS_FAST", "Fast", ISS_OFF);
    IUFillSwitchVector(&FocusModeSP, FocusModeS, 3, getDeviceName(), "FOCUS_MODE", "Mode", FOCUS_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    TrackState = SCOPE_IDLE;

    initGuiderProperties(getDeviceName(), GUIDE_TAB);

    /* Add debug/simulation/config controls so we may debug driver if necessary */
    addAuxControls();

    setDriverInterface(getDriverInterface() | GUIDER_INTERFACE);

    double longitude=0, latitude=90;
    // Get value from config file if it exists.
    IUGetConfigNumber(getDeviceName(), "GEOGRAPHIC_COORD", "LONG", &longitude);
    currentRA  = get_local_sidereal_time(longitude);
    IUGetConfigNumber(getDeviceName(), "GEOGRAPHIC_COORD", "LAT", &latitude);
    currentDEC = latitude > 0 ? 90 : -90;

    return true;
}

void LX200Generic::ISGetProperties(const char *dev)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) != 0)
        return;

    INDI::Telescope::ISGetProperties(dev);

    if (isConnected())
    {
        if (genericCapability & LX200_HAS_ALIGNMENT_TYPE)
            defineSwitch(&AlignmentSP);

        if (genericCapability & LX200_HAS_TRACKING_FREQ)
            defineNumber(&TrackingFreqNP);

        if (genericCapability & LX200_HAS_PULSE_GUIDING)
            defineSwitch(&UsePulseCmdSP);

        if (genericCapability & LX200_HAS_SITES)
        {
            defineSwitch(&SiteSP);
            defineText(&SiteNameTP);
        }

        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);

        if (genericCapability & LX200_HAS_FOCUS)
        {
            defineSwitch(&FocusMotionSP);
            defineNumber(&FocusTimerNP);
            defineSwitch(&FocusModeSP);
        }
    }
}

bool LX200Generic::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        if (genericCapability & LX200_HAS_ALIGNMENT_TYPE)
            defineSwitch(&AlignmentSP);

        if (genericCapability & LX200_HAS_TRACKING_FREQ)
            defineNumber(&TrackingFreqNP);

        if (genericCapability & LX200_HAS_PULSE_GUIDING)
            defineSwitch(&UsePulseCmdSP);

        if (genericCapability & LX200_HAS_SITES)
        {
            defineSwitch(&SiteSP);
            defineText(&SiteNameTP);
        }

        defineNumber(&GuideNSNP);
        defineNumber(&GuideWENP);

        if (genericCapability & LX200_HAS_FOCUS)
        {
            defineSwitch(&FocusMotionSP);
            defineNumber(&FocusTimerNP);
            defineSwitch(&FocusModeSP);
        }

        getBasicData();
    }
    else
    {
        if (genericCapability & LX200_HAS_ALIGNMENT_TYPE)
            deleteProperty(AlignmentSP.name);

        if (genericCapability & LX200_HAS_TRACKING_FREQ)
            deleteProperty(TrackingFreqNP.name);

        if (genericCapability & LX200_HAS_PULSE_GUIDING)
            deleteProperty(UsePulseCmdSP.name);

        if (genericCapability & LX200_HAS_SITES)
        {
            deleteProperty(SiteSP.name);
            deleteProperty(SiteNameTP.name);
        }

        deleteProperty(GuideNSNP.name);
        deleteProperty(GuideWENP.name);

        if (genericCapability & LX200_HAS_FOCUS)
        {
            deleteProperty(FocusMotionSP.name);
            deleteProperty(FocusTimerNP.name);
            deleteProperty(FocusModeSP.name);
        }
    }

    return true;
}

bool LX200Generic::checkConnection()
{
    if (isSimulation())
        return true;

    return (check_lx200_connection(PortFD) == 0);
}

bool LX200Generic::Handshake()
{
    return checkConnection();
}

bool LX200Generic::isSlewComplete()
{
    return (::isSlewComplete(PortFD) == 1);
}

bool LX200Generic::ReadScopeStatus()
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

    if (TrackState == SCOPE_SLEWING)
    {
        // Check if LX200 is done slewing
        if (isSlewComplete())
        {
            // Set slew mode to "Centering"
            IUResetSwitch(&SlewRateSP);
            SlewRateS[SLEW_CENTERING].s = ISS_ON;
            IDSetSwitch(&SlewRateSP, nullptr);

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

    return true;
}

bool LX200Generic::Goto(double ra, double dec)
{
    targetRA  = ra;
    targetDEC = dec;
    char RAStr[64]={0}, DecStr[64]={0};
    int fracbase = 0;

    switch (getLX200Format())
    {
    case LX200_LONGER_FORMAT:
        fracbase = 360000;
        break;
    case LX200_LONG_FORMAT:
    case LX200_SHORT_FORMAT:
    default:
        fracbase = 3600;
        break;
    }

    fs_sexa(RAStr, targetRA, 2, fracbase);
    fs_sexa(DecStr, targetDEC, 2, fracbase);

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

        int err = 0;

        /* Slew reads the '0', that is not the end of the slew */
        if ((err = Slew(PortFD)))
        {
            DEBUGF(INDI::Logger::DBG_ERROR, "Error Slewing to JNow RA %s - DEC %s", RAStr, DecStr);
            slewError(err);
            return false;
        }
    }

    TrackState = SCOPE_SLEWING;
    EqNP.s     = IPS_BUSY;

    DEBUGF(INDI::Logger::DBG_SESSION, "Slewing to RA: %s - DEC: %s", RAStr, DecStr);

    return true;
}

bool LX200Generic::Sync(double ra, double dec)
{
    char syncString[256]={0};

    if (!isSimulation() && (setObjectRA(PortFD, ra) < 0 || (setObjectDEC(PortFD, dec)) < 0))
    {
        EqNP.s = IPS_ALERT;
        IDSetNumber(&EqNP, "Error setting RA/DEC. Unable to Sync.");
        return false;
    }

    if (!isSimulation() && ::Sync(PortFD, syncString) < 0)
    {
        EqNP.s = IPS_ALERT;
        IDSetNumber(&EqNP, "Synchronization failed.");
        return false;
    }

    currentRA  = ra;
    currentDEC = dec;

    DEBUG(INDI::Logger::DBG_SESSION, "Synchronization successful.");

    EqNP.s     = IPS_OK;

    NewRaDec(currentRA, currentDEC);

    return true;
}

bool LX200Generic::Park()
{
    if (!isSimulation())
    {
        // If scope is moving, let's stop it first.
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

            // sleep for 100 msec
            usleep(100000);
        }

        if (!isSimulation() && slewToPark(PortFD) < 0)
        {
            ParkSP.s = IPS_ALERT;
            IDSetSwitch(&ParkSP, "Parking Failed.");
            return false;
        }
    }

    ParkSP.s   = IPS_BUSY;
    TrackState = SCOPE_PARKING;
    DEBUG(INDI::Logger::DBG_SESSION, "Parking telescope in progress...");
    return true;
}

bool LX200Generic::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    int current_move = (dir == DIRECTION_NORTH) ? LX200_NORTH : LX200_SOUTH;

    switch (command)
    {
    case MOTION_START:
        if (!isSimulation() && MoveTo(PortFD, current_move) < 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Error setting N/S motion direction.");
            return false;
        }
        else
            DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.",
                   (current_move == LX200_NORTH) ? "North" : "South");
        break;

    case MOTION_STOP:
        if (!isSimulation() && HaltMovement(PortFD, current_move) < 0)
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

bool LX200Generic::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    int current_move = (dir == DIRECTION_WEST) ? LX200_WEST : LX200_EAST;

    switch (command)
    {
    case MOTION_START:
        if (!isSimulation() && MoveTo(PortFD, current_move) < 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Error setting W/E motion direction.");
            return false;
        }
        else
            DEBUGF(INDI::Logger::DBG_SESSION, "Moving toward %s.", (current_move == LX200_WEST) ? "West" : "East");
        break;

    case MOTION_STOP:
        if (!isSimulation() && HaltMovement(PortFD, current_move) < 0)
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

bool LX200Generic::Abort()
{
    if (!isSimulation() && abortSlew(PortFD) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Failed to abort slew.");
        return false;
    }

    if (GuideNSNP.s == IPS_BUSY || GuideWENP.s == IPS_BUSY)
    {
        GuideNSNP.s = GuideWENP.s = IPS_IDLE;
        GuideNSN[0].value = GuideNSN[1].value = 0.0;
        GuideWEN[0].value = GuideWEN[1].value = 0.0;

        if (GuideNSTID)
        {
            IERmTimer(GuideNSTID);
            GuideNSTID = 0;
        }

        if (GuideWETID)
        {
            IERmTimer(GuideWETID);
            GuideNSTID = 0;
        }

        DEBUG(INDI::Logger::DBG_SESSION, "Guide aborted.");
        IDSetNumber(&GuideNSNP, nullptr);
        IDSetNumber(&GuideWENP, nullptr);

        return true;
    }

    return true;
}

bool LX200Generic::setLocalDate(uint8_t days, uint8_t months, uint8_t years)
{
    return (setCalenderDate(PortFD, days, months, years) == 0);
}

bool LX200Generic::setLocalTime24(uint8_t hour, uint8_t minute, uint8_t second)
{
    return (setLocalTime(PortFD, hour, minute, second) == 0);
}

bool LX200Generic::setUTCOffset(double offset)
{
    return (::setUTCOffset(PortFD, (offset * -1.0)) == 0);
}

bool LX200Generic::updateTime(ln_date *utc, double utc_offset)
{
    struct ln_zonedate ltm;

    if (isSimulation())
        return true;

    ln_date_to_zonedate(utc, &ltm, utc_offset * 3600.0);

    JD = ln_get_julian_day(utc);

    DEBUGF(INDI::Logger::DBG_DEBUG, "New JD is %.2f", JD);

    // Meade defines UTC Offset as the offset ADDED to local time to yield UTC, which
    // is the opposite of the standard definition of UTC offset!
    if (setUTCOffset(utc_offset) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting UTC Offset.");
        return false;
    }

    // Set Local Time
    if (setLocalTime24(ltm.hours, ltm.minutes, ltm.seconds) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting local time.");
        return false;
    }

    if (setLocalDate(ltm.days, ltm.months, ltm.years) == false)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting local date.");
        return false;
    }

    DEBUG(INDI::Logger::DBG_SESSION, "Time updated, updating planetary data...");
    return true;
}

bool LX200Generic::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);

    if (isSimulation())
        return true;

    if (!isSimulation() && setSiteLongitude(PortFD, 360.0 - longitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site longitude coordinates");
        return false;
    }

    if (!isSimulation() && setSiteLatitude(PortFD, latitude) < 0)
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Error setting site latitude coordinates");
        return false;
    }

    char l[32]={0}, L[32]={0};
    fs_sexa(l, latitude, 3, 3600);
    fs_sexa(L, longitude, 4, 3600);

    DEBUGF(INDI::Logger::DBG_SESSION, "Site location updated to Lat %.32s - Long %.32s", l, L);

    return true;
}

bool LX200Generic::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, SiteNameTP.name))
        {
            if (!isSimulation() && setSiteName(PortFD, texts[0], currentSiteNum) < 0)
            {
                SiteNameTP.s = IPS_ALERT;
                IDSetText(&SiteNameTP, "Setting site name");
                return false;
            }

            SiteNameTP.s = IPS_OK;
            IText *tp    = IUFindText(&SiteNameTP, names[0]);
            IUSaveText(tp, texts[0]);
            IDSetText(&SiteNameTP, "Site name updated");
            return true;
        }
    }

    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool LX200Generic::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Update Frequency
        if (!strcmp(name, TrackingFreqNP.name))
        {
            DEBUGF(INDI::Logger::DBG_DEBUG, "Trying to set track freq of: %f\n", values[0]);

            if (!isSimulation() && setTrackFreq(PortFD, values[0]) < 0)
            {
                TrackingFreqNP.s = IPS_ALERT;
                IDSetNumber(&TrackingFreqNP, "Error setting tracking frequency");
                return false;
            }

            TrackingFreqNP.s           = IPS_OK;
            TrackingFreqNP.np[0].value = values[0];
            IDSetNumber(&TrackingFreqNP, "Tracking frequency set to %04.1f", values[0]);

            if (trackingMode != LX200_TRACK_MANUAL)
            {
                trackingMode    = LX200_TRACK_MANUAL;
                TrackModeS[0].s = ISS_OFF;
                TrackModeS[1].s = ISS_OFF;
                TrackModeS[2].s = ISS_OFF;
                TrackModeS[3].s = ISS_ON;
                TrackModeSP.s   = IPS_OK;
                selectTrackingMode(PortFD, trackingMode);
                IDSetSwitch(&TrackModeSP, nullptr);
            }

            return true;
        }

        if (!strcmp(name, FocusTimerNP.name))
        {
            // Don't update if busy
            if (FocusTimerNP.s == IPS_BUSY)
                return true;

            IUUpdateNumber(&FocusTimerNP, values, names, n);

            FocusTimerNP.s = IPS_OK;

            IDSetNumber(&FocusTimerNP, nullptr);

            if (isDebug())
                IDLog("Setting focus timer to %g\n", FocusTimerN[0].value);

            return true;
        }

        processGuiderProperties(name, values, names, n);
    }

    //  if we didn't process it, continue up the chain, let somebody else
    //  give it a shot
    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool LX200Generic::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    int index = 0;

    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        // Alignment
        if (!strcmp(name, AlignmentSP.name))
        {
            if (IUUpdateSwitch(&AlignmentSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&AlignmentSP);

            if (!isSimulation() && setAlignmentMode(PortFD, index) < 0)
            {
                AlignmentSP.s = IPS_ALERT;
                IDSetSwitch(&AlignmentSP, "Error setting alignment mode.");
                return false;
            }

            AlignmentSP.s = IPS_OK;
            IDSetSwitch(&AlignmentSP, nullptr);
            return true;
        }

        // Sites
        if (!strcmp(name, SiteSP.name))
        {
            if (IUUpdateSwitch(&SiteSP, states, names, n) < 0)
                return false;

            currentSiteNum = IUFindOnSwitchIndex(&SiteSP) + 1;

            if (!isSimulation() && selectSite(PortFD, currentSiteNum) < 0)
            {
                SiteSP.s = IPS_ALERT;
                IDSetSwitch(&SiteSP, "Error selecting sites.");
                return false;
            }

            if (isSimulation())
                IUSaveText(&SiteNameTP.tp[0], "Sample Site");
            else
                getSiteName(PortFD, SiteNameTP.tp[0].text, currentSiteNum);

            if (GetTelescopeCapability() & TELESCOPE_HAS_LOCATION)
                sendScopeLocation();

            SiteNameTP.s = SiteSP.s = IPS_OK;

            IDSetText(&SiteNameTP, nullptr);
            IDSetSwitch(&SiteSP, nullptr);

            return false;
        }

        // Focus Motion
        if (!strcmp(name, FocusMotionSP.name))
        {
            // If mode is "halt"
            if (FocusModeS[0].s == ISS_ON)
            {
                FocusMotionSP.s = IPS_IDLE;
                IDSetSwitch(&FocusMotionSP, "Focus mode is halt. Select slow or fast mode");
                return true;
            }

            int last_motion = IUFindOnSwitchIndex(&FocusMotionSP);

            if (IUUpdateSwitch(&FocusMotionSP, states, names, n) < 0)
                return false;

            index = IUFindOnSwitchIndex(&FocusMotionSP);

            // If same direction and we're busy, stop
            if (last_motion == index && FocusMotionSP.s == IPS_BUSY)
            {
                IUResetSwitch(&FocusMotionSP);
                FocusMotionSP.s = IPS_IDLE;
                setFocuserSpeedMode(PortFD, 0);
                IDSetSwitch(&FocusMotionSP, nullptr);
                return true;
            }

            if (!isSimulation() && setFocuserMotion(PortFD, index) < 0)
            {
                FocusMotionSP.s = IPS_ALERT;
                IDSetSwitch(&FocusMotionSP, "Error setting focuser speed.");
                return false;
            }

            // with a timer
            if (FocusTimerN[0].value > 0)
            {
                FocusTimerNP.s  = IPS_BUSY;
                FocusMotionSP.s = IPS_BUSY;
                IEAddTimer(50, LX200Generic::updateFocusHelper, this);
            }

            FocusMotionSP.s = IPS_OK;
            IDSetSwitch(&FocusMotionSP, nullptr);
            return true;
        }

        // Focus speed
        if (!strcmp(name, FocusModeSP.name))
        {
            IUResetSwitch(&FocusModeSP);
            IUUpdateSwitch(&FocusModeSP, states, names, n);

            index = IUFindOnSwitchIndex(&FocusModeSP);

            /* disable timer and motion */
            if (index == 0)
            {
                IUResetSwitch(&FocusMotionSP);
                FocusMotionSP.s = IPS_IDLE;
                FocusTimerNP.s  = IPS_IDLE;
                IDSetSwitch(&FocusMotionSP, nullptr);
                IDSetNumber(&FocusTimerNP, nullptr);
            }

            if (!isSimulation())
                setFocuserSpeedMode(PortFD, index);
            FocusModeSP.s = IPS_OK;
            IDSetSwitch(&FocusModeSP, nullptr);
            return true;
        }

        // Pulse-Guide command support
        if (!strcmp(name, UsePulseCmdSP.name))
        {
            IUResetSwitch(&UsePulseCmdSP);
            IUUpdateSwitch(&UsePulseCmdSP, states, names, n);

            UsePulseCmdSP.s = IPS_OK;
            IDSetSwitch(&UsePulseCmdSP, nullptr);
            return true;
        }
    }

    //  Nobody has claimed this, so pass it to the parent
    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool LX200Generic::SetTrackMode(uint8_t mode)
{
    if (isSimulation())
        return true;

    bool rc = (selectTrackingMode(PortFD, mode) == 0);

    // Only update tracking frequency if it is defined and not deleted by child classes
    if (rc &&  (genericCapability & LX200_HAS_TRACKING_FREQ))
    {
        getTrackFreq(PortFD, &TrackFreqN[0].value);
        IDSetNumber(&TrackingFreqNP, nullptr);
    }
    return rc;
}

bool LX200Generic::SetSlewRate(int index)
{
    // Convert index to Meade format
    index = 3 - index;

    if (!isSimulation() && setSlewMode(PortFD, index) < 0)
    {
        SlewRateSP.s = IPS_ALERT;
        IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
        return false;
    }

    SlewRateSP.s = IPS_OK;
    IDSetSwitch(&SlewRateSP, nullptr);
    return true;
}

void LX200Generic::updateFocusHelper(void *p)
{
    ((LX200Generic *)p)->updateFocusTimer();
}

void LX200Generic::updateFocusTimer()
{
    switch (FocusTimerNP.s)
    {
    case IPS_IDLE:
        break;

    case IPS_BUSY:
        if (isDebug())
            IDLog("Focus Timer Value is %g\n", FocusTimerN[0].value);

        FocusTimerN[0].value -= 50;

        if (FocusTimerN[0].value <= 0)
        {
            if (isDebug())
                IDLog("Focus Timer Expired\n");

            if (!isSimulation() && setFocuserSpeedMode(telescope->PortFD, 0) < 0)
            {
                FocusModeSP.s = IPS_ALERT;
                IDSetSwitch(&FocusModeSP, "Error setting focuser mode.");

                if (isDebug())
                    IDLog("Error setting focuser mode\n");

                return;
            }

            FocusMotionSP.s = IPS_IDLE;
            FocusTimerNP.s  = IPS_OK;
            FocusModeSP.s   = IPS_OK;

            IUResetSwitch(&FocusMotionSP);
            IUResetSwitch(&FocusModeSP);
            FocusModeS[0].s = ISS_ON;

            IDSetSwitch(&FocusModeSP, nullptr);
            IDSetSwitch(&FocusMotionSP, nullptr);
        }

        IDSetNumber(&FocusTimerNP, nullptr);

        if (FocusTimerN[0].value > 0)
            IEAddTimer(50, LX200Generic::updateFocusHelper, this);
        break;

    case IPS_OK:
        break;

    case IPS_ALERT:
        break;
    }
}

void LX200Generic::mountSim()
{
    static struct timeval ltv;
    struct timeval tv;
    double dt=0, da=0, dx=0;
    int nlocked=0;

    /* update elapsed time since last poll, don't presume exactly POLLMS */
    gettimeofday(&tv, nullptr);

    if (ltv.tv_sec == 0 && ltv.tv_usec == 0)
        ltv = tv;

    dt  = tv.tv_sec - ltv.tv_sec + (tv.tv_usec - ltv.tv_usec) / 1e6;
    ltv = tv;
    da  = LX200_GENERIC_SLEWRATE * dt;

    /* Process per current state. We check the state of EQUATORIAL_COORDS and act acoordingly */
    switch (TrackState)
    {

    case SCOPE_IDLE:
        currentRA  += (TRACKRATE_SIDEREAL/3600.0 * dt / 15.);
        break;

    case SCOPE_TRACKING:
        switch (IUFindOnSwitchIndex(&TrackModeSP))
        {
        case TRACK_SIDEREAL:
            da = 0;
            dx = 0;
            break;

        case TRACK_LUNAR:
            da = ((TRACKRATE_LUNAR-TRACKRATE_SIDEREAL)/3600.0 * dt / 15.);
            dx = 0;
            break;

        case TRACK_SOLAR:
            da = ((TRACKRATE_SOLAR-TRACKRATE_SIDEREAL)/3600.0 * dt / 15.);
            dx = 0;
            break;

        case TRACK_CUSTOM:
            da = ((TrackRateN[AXIS_RA].value-TRACKRATE_SIDEREAL)/3600.0 * dt / 15.);
            dx = (TrackRateN[AXIS_DE].value/3600.0 * dt);
            break;

        }

        currentRA  += da;
        currentDEC += dx;
        break;

    case SCOPE_SLEWING:
    case SCOPE_PARKING:
        /* slewing - nail it when both within one pulse @ LX200_GENERIC_SLEWRATE */
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

void LX200Generic::getBasicData()
{
    if (!isSimulation())
    {
        checkLX200Format(PortFD);

        if (genericCapability & LX200_HAS_ALIGNMENT_TYPE)
            getAlignment();

        // Only check time format if it is not already initialized by the class
        if ( (GetTelescopeCapability() & TELESCOPE_HAS_TIME) && timeFormat == -1)
        {
            if (getTimeFormat(PortFD, &timeFormat) < 0)
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to retrieve time format from device.");
            else
            {
                int ret = 0;

                timeFormat = (timeFormat == 24) ? LX200_24 : LX200_AM;
                // We always do 24 hours
                if (timeFormat != LX200_24)
                    ret = toggleTimeFormat(PortFD);
            }
        }

        if (genericCapability & LX200_HAS_SITES)
        {
            SiteNameT[0].text = new char[64];

            if (getSiteName(PortFD, SiteNameT[0].text, currentSiteNum) < 0)
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to get site name from device");
            else
                IDSetText(&SiteNameTP, nullptr);
        }


        if (genericCapability & LX200_HAS_TRACKING_FREQ)
        {
            if (getTrackFreq(PortFD, &TrackFreqN[0].value) < 0)
                DEBUG(INDI::Logger::DBG_ERROR, "Failed to get tracking frequency from device.");
            else
                IDSetNumber(&TrackingFreqNP, nullptr);
        }

    }

    if (sendLocationOnStartup && (GetTelescopeCapability() & TELESCOPE_HAS_LOCATION))
        sendScopeLocation();
    if (sendTimeOnStartup && (GetTelescopeCapability() & TELESCOPE_HAS_TIME))
        sendScopeTime();
}

void LX200Generic::slewError(int slewCode)
{
    if (slewCode == 1)
        DEBUG(INDI::Logger::DBG_ERROR, "Object below horizon.");
    else if (slewCode == 2)
        DEBUG(INDI::Logger::DBG_ERROR, "Object below the minimum elevation limit.");
    else
        DEBUG(INDI::Logger::DBG_ERROR, "Slew failed.");

    EqNP.s = IPS_ALERT;
    IDSetNumber(&EqNP, nullptr);
}

void LX200Generic::getAlignment()
{
    signed char align = ACK(PortFD);
    if (align < 0)
    {
        IDSetSwitch(&AlignmentSP, "Failed to get telescope alignment.");
        return;
    }

    AlignmentS[0].s = ISS_OFF;
    AlignmentS[1].s = ISS_OFF;
    AlignmentS[2].s = ISS_OFF;

    switch (align)
    {
    case 'P':
        AlignmentS[0].s = ISS_ON;
        break;
    case 'A':
        AlignmentS[1].s = ISS_ON;
        break;
    case 'L':
        AlignmentS[2].s = ISS_ON;
        break;
    }

    AlignmentSP.s = IPS_OK;
    IDSetSwitch(&AlignmentSP, nullptr);
}

bool LX200Generic::getLocalTime(char *timeString)
{
    if (isSimulation())
    {
        time_t now = time (NULL);
        strftime(timeString, 32, "%T", localtime(&now));
    }
    else
    {
        double ctime=0;
        int h, m, s;
        getLocalTime24(PortFD, &ctime);
        getSexComponents(ctime, &h, &m, &s);
        snprintf(timeString, 32, "%02d:%02d:%02d", h, m, s);
    }

    return true;
}

bool LX200Generic::getLocalDate(char *dateString)
{
    if (isSimulation())
    {
        time_t now = time (NULL);
        strftime(dateString, 32, "%F", localtime(&now));
    }
    else
    {
        getCalendarDate(PortFD, dateString);
    }

    return true;
}

bool LX200Generic::getUTFOffset(double *offset)
{
    if (isSimulation())
    {
        *offset = 3;
        return true;
    }

    int lx200_utc_offset = 0;
    getUTCOffset(PortFD, &lx200_utc_offset);
    // LX200 TimeT Offset is defined at the number of hours added to LOCAL TIME to get TimeT. This is contrary to the normal definition.
    *offset = lx200_utc_offset * -1;
    return true;
}

bool LX200Generic::sendScopeTime()
{
    char cdate[32]={0};
    char ctime[32]={0};
    struct tm ltm;
    struct tm utm;
    time_t time_epoch;

    double offset=0;
    if (getUTFOffset(&offset))
    {
        char utcStr[8]={0};
        snprintf(utcStr, 8, "%.2f", offset);
        IUSaveText(&TimeT[1], utcStr);
    }
    else
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Could not obtain UTC offset from mount!");
        return false;
    }

    if (getLocalTime(ctime) == false)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Could not obtain local time from mount!");
        return false;
    }

    if (getLocalDate(cdate) == false)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Could not obtain local date from mount!");
        return false;
    }

    // To ISO 8601 format in LOCAL TIME!
    char datetime[64];
    snprintf(datetime, 64, "%sT%s", cdate, ctime);

    // Now that date+time are combined, let's get tm representation of it.
    if (strptime(datetime, "%FT%T", &ltm) == NULL)
    {
        DEBUGF(INDI::Logger::DBG_WARNING, "Could not process mount date and time: %s", datetime);
        return false;
    }

    // Get local time epoch in UNIX seconds
    time_epoch = mktime(&ltm);

    // LOCAL to UTC by subtracting offset.
    time_epoch -= static_cast<int>(offset * 3600.0);

    // Get UTC (we're using localtime_r, but since we shifted time_epoch above by UTCOffset, we should be getting the real UTC time)
    localtime_r(&time_epoch, &utm);

    // Format it into the final UTC ISO 8601
    strftime(cdate, 32, "%Y-%m-%dT%H:%M:%S", &utm);
    IUSaveText(&TimeT[0], cdate);

    DEBUGF(INDI::Logger::DBG_DEBUG, "Mount controller UTC Time: %s", TimeT[0].text);
    DEBUGF(INDI::Logger::DBG_DEBUG, "Mount controller UTC Offset: %s", TimeT[1].text);

    // Let's send everything to the client
    IDSetText(&TimeTP, nullptr);

    return true;
}

bool LX200Generic::sendScopeLocation()
{
    int dd = 0, mm = 0;

    if (isSimulation())
    {
        LocationNP.np[LOCATION_LATITUDE].value = 29.5;
        LocationNP.np[LOCATION_LONGITUDE].value = 48.0;
        LocationNP.np[LOCATION_ELEVATION].value = 10;
        LocationNP.s           = IPS_OK;
        IDSetNumber(&LocationNP, nullptr);
        return true;
    }

    if (getSiteLatitude(PortFD, &dd, &mm) < 0)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Failed to get site latitude from device.");
        return false;
    }
    else
    {
        if (dd > 0)
            LocationNP.np[0].value = dd + mm / 60.0;
        else
            LocationNP.np[0].value = dd - mm / 60.0;
    }

    if (getSiteLongitude(PortFD, &dd, &mm) < 0)
    {
        DEBUG(INDI::Logger::DBG_WARNING, "Failed to get site longitude from device.");
        return false;
    }
    else
    {
        if (dd > 0)
            LocationNP.np[1].value = 360.0 - (dd + mm / 60.0);
        else
            LocationNP.np[1].value = (dd - mm / 60.0) * -1.0;

    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Mount Controller Latitude: %g Longitude: %g", LocationN[LOCATION_LATITUDE].value, LocationN[LOCATION_LONGITUDE].value);

    IDSetNumber(&LocationNP, nullptr);

    return true;
}

IPState LX200Generic::GuideNorth(float ms)
{
    int use_pulse_cmd;

    use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);

    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }

    // If already moving (no pulse command), then stop movement
    if (MovementNSSP.s == IPS_BUSY)
    {
        int dir = IUFindOnSwitchIndex(&MovementNSSP);

        MoveNS(dir == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);
    }

    if (GuideNSTID)
    {
        IERmTimer(GuideNSTID);
        GuideNSTID = 0;
    }

    if (use_pulse_cmd)
    {
        SendPulseCmd(LX200_NORTH, ms);
    }
    else
    {
        if (setSlewMode(PortFD, LX200_SLEW_GUIDE) < 0)
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }

        MovementNSS[0].s = ISS_ON;
        MoveNS(DIRECTION_NORTH, MOTION_START);
    }

    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_NORTH;
    GuideNSTID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Generic::GuideSouth(float ms)
{
    int use_pulse_cmd;

    use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);

    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }

    // If already moving (no pulse command), then stop movement
    if (MovementNSSP.s == IPS_BUSY)
    {
        int dir = IUFindOnSwitchIndex(&MovementNSSP);

        MoveNS(dir == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);
    }

    if (GuideNSTID)
    {
        IERmTimer(GuideNSTID);
        GuideNSTID = 0;
    }

    if (use_pulse_cmd)
    {
        SendPulseCmd(LX200_SOUTH, ms);
    }
    else
    {
        if (setSlewMode(PortFD, LX200_SLEW_GUIDE) < 0)
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }

        MovementNSS[1].s = ISS_ON;
        MoveNS(DIRECTION_SOUTH, MOTION_START);
    }

    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_SOUTH;
    GuideNSTID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Generic::GuideEast(float ms)
{
    int use_pulse_cmd;

    use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);

    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }

    // If already moving (no pulse command), then stop movement
    if (MovementWESP.s == IPS_BUSY)
    {
        int dir = IUFindOnSwitchIndex(&MovementWESP);

        MoveWE(dir == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
    }

    if (GuideWETID)
    {
        IERmTimer(GuideWETID);
        GuideWETID = 0;
    }

    if (use_pulse_cmd)
    {
        SendPulseCmd(LX200_EAST, ms);
    }
    else
    {
        if (setSlewMode(PortFD, LX200_SLEW_GUIDE) < 0)
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }

        MovementWES[1].s = ISS_ON;
        MoveWE(DIRECTION_EAST, MOTION_START);
    }

    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_EAST;
    GuideWETID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

IPState LX200Generic::GuideWest(float ms)
{
    int use_pulse_cmd;

    use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);

    if (!use_pulse_cmd && (MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY))
    {
        DEBUG(INDI::Logger::DBG_ERROR, "Cannot guide while moving.");
        return IPS_ALERT;
    }

    // If already moving (no pulse command), then stop movement
    if (MovementWESP.s == IPS_BUSY)
    {
        int dir = IUFindOnSwitchIndex(&MovementWESP);

        MoveWE(dir == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
    }

    if (GuideWETID)
    {
        IERmTimer(GuideWETID);
        GuideWETID = 0;
    }

    if (use_pulse_cmd)
    {
        SendPulseCmd(LX200_WEST, ms);
    }
    else
    {
        if (setSlewMode(PortFD, LX200_SLEW_GUIDE) < 0)
        {
            SlewRateSP.s = IPS_ALERT;
            IDSetSwitch(&SlewRateSP, "Error setting slew mode.");
            return IPS_ALERT;
        }

        MovementWES[0].s = ISS_ON;
        MoveWE(DIRECTION_WEST, MOTION_START);
    }

    // Set slew to guiding
    IUResetSwitch(&SlewRateSP);
    SlewRateS[SLEW_GUIDE].s = ISS_ON;
    IDSetSwitch(&SlewRateSP, nullptr);
    guide_direction = LX200_WEST;
    GuideWETID      = IEAddTimer(ms, guideTimeoutHelper, this);
    return IPS_BUSY;
}

int LX200Generic::SendPulseCmd(int direction, int duration_msec)
{
    return ::SendPulseCmd(PortFD, direction, duration_msec);
}

void LX200Generic::guideTimeoutHelper(void * p)
{
    ((LX200Generic *)p)->guideTimeout();
}

void LX200Generic::guideTimeout()
{
    int use_pulse_cmd;

    use_pulse_cmd = IUFindOnSwitchIndex(&UsePulseCmdSP);
    if (guide_direction == -1)
    {
        HaltMovement(PortFD, LX200_NORTH);
        HaltMovement(PortFD, LX200_SOUTH);
        HaltMovement(PortFD, LX200_EAST);
        HaltMovement(PortFD, LX200_WEST);

        MovementNSSP.s = IPS_IDLE;
        MovementWESP.s = IPS_IDLE;
        IUResetSwitch(&MovementNSSP);
        IUResetSwitch(&MovementWESP);
        IDSetSwitch(&MovementNSSP, nullptr);
        IDSetSwitch(&MovementWESP, nullptr);
        IERmTimer(GuideNSTID);
        IERmTimer(GuideWETID);
    }
    else if (!use_pulse_cmd)
    {
        if (guide_direction == LX200_NORTH || guide_direction == LX200_SOUTH)
        {
            MoveNS(guide_direction == LX200_NORTH ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP);

            if (guide_direction == LX200_NORTH)
                GuideNSNP.np[0].value = 0;
            else
                GuideNSNP.np[1].value = 0;

            GuideNSNP.s = IPS_IDLE;
            IDSetNumber(&GuideNSNP, nullptr);
            MovementNSSP.s = IPS_IDLE;
            IUResetSwitch(&MovementNSSP);
            IDSetSwitch(&MovementNSSP, nullptr);
        }
        if (guide_direction == LX200_WEST || guide_direction == LX200_EAST)
        {
            MoveWE(guide_direction == LX200_WEST ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP);
            if (guide_direction == LX200_WEST)
                GuideWENP.np[0].value = 0;
            else
                GuideWENP.np[1].value = 0;

            GuideWENP.s = IPS_IDLE;
            IDSetNumber(&GuideWENP, nullptr);
            MovementWESP.s = IPS_IDLE;
            IUResetSwitch(&MovementWESP);
            IDSetSwitch(&MovementWESP, nullptr);
        }
    }
    if (guide_direction == LX200_NORTH || guide_direction == LX200_SOUTH || guide_direction == -1)
    {
        GuideNSNP.np[0].value = 0;
        GuideNSNP.np[1].value = 0;
        GuideNSNP.s           = IPS_IDLE;
        GuideNSTID            = 0;
        IDSetNumber(&GuideNSNP, nullptr);
    }
    if (guide_direction == LX200_WEST || guide_direction == LX200_EAST || guide_direction == -1)
    {
        GuideWENP.np[0].value = 0;
        GuideWENP.np[1].value = 0;
        GuideWENP.s           = IPS_IDLE;
        GuideWETID            = 0;
        IDSetNumber(&GuideWENP, nullptr);
    }
}

bool LX200Generic::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    if (genericCapability & LX200_HAS_PULSE_GUIDING)
        IUSaveConfigSwitch(fp, &UsePulseCmdSP);

    return true;
}
