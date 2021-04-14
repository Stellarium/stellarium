/*******************************************************************************
  Copyright(c) 2011 Gerry Rozema, Jasem Mutlaq. All rights reserved.

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/

#include "inditelescope.h"

#include "indicom.h"
#include "indicontroller.h"
#include "connectionplugins/connectionserial.h"
#include "connectionplugins/connectiontcp.h"

/*
#include <libnova/sidereal_time.h>
#include <libnova/transform.h>
*/

#include <cmath>
#include <cerrno>
#include <pwd.h>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <wordexp.h>
#include <limits>

namespace INDI
{

Telescope::Telescope()
    : DefaultDevice(), ScopeConfigFileName(GetHomeDirectory() + "/.indi/ScopeConfig.xml"),
      ParkDataFileName(GetHomeDirectory() + "/.indi/ParkData.xml")
{
    controller = new Controller(this);
    controller->setJoystickCallback(joystickHelper);
    controller->setAxisCallback(axisHelper);
    controller->setButtonCallback(buttonHelper);

    currentPierSide = PIER_EAST;
    lastPierSide    = PIER_UNKNOWN;

    currentPECState = PEC_OFF;
    lastPECState    = PEC_UNKNOWN;
}

Telescope::~Telescope()
{
    if (ParkdataXmlRoot)
        delXMLEle(ParkdataXmlRoot);

    delete (controller);
}

bool Telescope::initProperties()
{
    DefaultDevice::initProperties();

    // Active Devices
    IUFillText(&ActiveDeviceT[0], "ACTIVE_GPS", "GPS", "GPS Simulator");
    IUFillText(&ActiveDeviceT[1], "ACTIVE_DOME", "DOME", "Dome Simulator");
    IUFillTextVector(&ActiveDeviceTP, ActiveDeviceT, 2, getDeviceName(), "ACTIVE_DEVICES", "Snoop devices", OPTIONS_TAB,
                     IP_RW, 60, IPS_IDLE);

    // Use locking if dome is closed (and or) park scope if dome is closing
    IUFillSwitch(&DomeClosedLockT[0], "NO_ACTION", "Ignore dome", ISS_ON);
    IUFillSwitch(&DomeClosedLockT[1], "LOCK_PARKING", "Dome locks", ISS_OFF);
    IUFillSwitch(&DomeClosedLockT[2], "FORCE_CLOSE", "Dome parks", ISS_OFF);
    IUFillSwitch(&DomeClosedLockT[3], "LOCK_AND_FORCE", "Both", ISS_OFF);
    IUFillSwitchVector(&DomeClosedLockTP, DomeClosedLockT, 4, getDeviceName(), "DOME_POLICY", "Dome parking policy",
                       OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillNumber(&EqN[AXIS_RA], "RA", "RA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumber(&EqN[AXIS_DE], "DEC", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 0);
    IUFillNumberVector(&EqNP, EqN, 2, getDeviceName(), "EQUATORIAL_EOD_COORD", "Eq. Coordinates", MAIN_CONTROL_TAB,
                       IP_RW, 60, IPS_IDLE);
    lastEqState = IPS_IDLE;

    IUFillNumber(&TargetN[AXIS_RA], "RA", "RA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
    IUFillNumber(&TargetN[AXIS_DE], "DEC", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 0);
    IUFillNumberVector(&TargetNP, TargetN, 2, getDeviceName(), "TARGET_EOD_COORD", "Slew Target", MOTION_TAB, IP_RO, 60,
                       IPS_IDLE);

    IUFillSwitch(&ParkOptionS[PARK_CURRENT], "PARK_CURRENT", "Current", ISS_OFF);
    IUFillSwitch(&ParkOptionS[PARK_DEFAULT], "PARK_DEFAULT", "Default", ISS_OFF);
    IUFillSwitch(&ParkOptionS[PARK_WRITE_DATA], "PARK_WRITE_DATA", "Write Data", ISS_OFF);
    IUFillSwitch(&ParkOptionS[PARK_PURGE_DATA], "PARK_PURGE_DATA", "Purge Data", ISS_OFF);
    IUFillSwitchVector(&ParkOptionSP, ParkOptionS, 4, getDeviceName(), "TELESCOPE_PARK_OPTION", "Park Options",
                       SITE_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillText(&TimeT[0], "UTC", "UTC Time", nullptr);
    IUFillText(&TimeT[1], "OFFSET", "UTC Offset", nullptr);
    IUFillTextVector(&TimeTP, TimeT, 2, getDeviceName(), "TIME_UTC", "UTC", SITE_TAB, IP_RW, 60, IPS_IDLE);

    IUFillNumber(&LocationN[LOCATION_LATITUDE], "LAT", "Lat (dd:mm:ss)", "%010.6m", -90, 90, 0, 0.0);
    IUFillNumber(&LocationN[LOCATION_LONGITUDE], "LONG", "Lon (dd:mm:ss)", "%010.6m", 0, 360, 0, 0.0);
    IUFillNumber(&LocationN[LOCATION_ELEVATION], "ELEV", "Elevation (m)", "%g", -200, 10000, 0, 0);
    IUFillNumberVector(&LocationNP, LocationN, 3, getDeviceName(), "GEOGRAPHIC_COORD", "Scope Location", SITE_TAB,
                       IP_RW, 60, IPS_IDLE);

    // Pier Side
    IUFillSwitch(&PierSideS[PIER_WEST], "PIER_WEST", "West (pointing east)", ISS_OFF);
    IUFillSwitch(&PierSideS[PIER_EAST], "PIER_EAST", "East (pointing west)", ISS_OFF);
    IUFillSwitchVector(&PierSideSP, PierSideS, 2, getDeviceName(), "TELESCOPE_PIER_SIDE", "Pier Side", MAIN_CONTROL_TAB,
                       IP_RO, ISR_ATMOST1, 60, IPS_IDLE);
    // Pier Side Simulation
    IUFillSwitch(&SimulatePierSideS[0], "SIMULATE_YES", "Yes", ISS_OFF);
    IUFillSwitch(&SimulatePierSideS[1], "SIMULATE_NO", "No", ISS_ON);
    IUFillSwitchVector(&SimulatePierSideSP, SimulatePierSideS, 2, getDeviceName(), "SIMULATE_PIER_SIDE", "Simulate Pier Side", MAIN_CONTROL_TAB,
                       IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    // PEC State
    IUFillSwitch(&PECStateS[PEC_OFF], "PEC OFF", "PEC OFF", ISS_OFF);
    IUFillSwitch(&PECStateS[PEC_ON], "PEC ON", "PEC ON", ISS_ON);
    IUFillSwitchVector(&PECStateSP, PECStateS, 2, getDeviceName(), "PEC", "PEC Playback", MOTION_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Track Mode. Child class must call AddTrackMode to add members
    IUFillSwitchVector(&TrackModeSP, TrackModeS, 0, getDeviceName(), "TELESCOPE_TRACK_MODE", "Track Mode", MAIN_CONTROL_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Track State
    IUFillSwitch(&TrackStateS[TRACK_ON], "TRACK_ON", "On", ISS_OFF);
    IUFillSwitch(&TrackStateS[TRACK_OFF], "TRACK_OFF", "Off", ISS_ON);
    IUFillSwitchVector(&TrackStateSP, TrackStateS, 2, getDeviceName(), "TELESCOPE_TRACK_STATE", "Tracking", MAIN_CONTROL_TAB, IP_RW, ISR_1OFMANY, 0,
                       IPS_IDLE);

    // Track Rate
    IUFillNumber(&TrackRateN[AXIS_RA], "TRACK_RATE_RA", "RA (arcsecs/s)", "%.6f", -16384.0, 16384.0, 0.000001, TRACKRATE_SIDEREAL);
    IUFillNumber(&TrackRateN[AXIS_DE], "TRACK_RATE_DE", "DE (arcsecs/s)", "%.6f", -16384.0, 16384.0, 0.000001, 0.0);
    IUFillNumberVector(&TrackRateNP, TrackRateN, 2, getDeviceName(), "TELESCOPE_TRACK_RATE", "Track Rates", MAIN_CONTROL_TAB,
                       IP_RW, 60, IPS_IDLE);

    // On Coord Set actions
    IUFillSwitch(&CoordS[0], "TRACK", "Track", ISS_ON);
    IUFillSwitch(&CoordS[1], "SLEW", "Slew", ISS_OFF);
    IUFillSwitch(&CoordS[2], "SYNC", "Sync", ISS_OFF);

    // If both GOTO and SYNC are supported
    if (CanGOTO() && CanSync())
        IUFillSwitchVector(&CoordSP, CoordS, 3, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    // If ONLY GOTO is supported
    else if (CanGOTO())
        IUFillSwitchVector(&CoordSP, CoordS, 2, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    // If ONLY SYNC is supported
    else if (CanSync())
    {
        IUFillSwitch(&CoordS[0], "SYNC", "Sync", ISS_ON);
        IUFillSwitchVector(&CoordSP, CoordS, 1, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    }

    if (nSlewRate >= 4)
        IUFillSwitchVector(&SlewRateSP, SlewRateS, nSlewRate, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate",
                           MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    IUFillSwitch(&ParkS[0], "PARK", "Park(ed)", ISS_OFF);
    IUFillSwitch(&ParkS[1], "UNPARK", "UnPark(ed)", ISS_OFF);
    IUFillSwitchVector(&ParkSP, ParkS, 2, getDeviceName(), "TELESCOPE_PARK", "Parking", MAIN_CONTROL_TAB, IP_RW,
                       ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&AbortS[0], "ABORT", "Abort", ISS_OFF);
    IUFillSwitchVector(&AbortSP, AbortS, 1, getDeviceName(), "TELESCOPE_ABORT_MOTION", "Abort Motion", MAIN_CONTROL_TAB,
                       IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillSwitch(&MovementNSS[DIRECTION_NORTH], "MOTION_NORTH", "North", ISS_OFF);
    IUFillSwitch(&MovementNSS[DIRECTION_SOUTH], "MOTION_SOUTH", "South", ISS_OFF);
    IUFillSwitchVector(&MovementNSSP, MovementNSS, 2, getDeviceName(), "TELESCOPE_MOTION_NS", "Motion N/S", MOTION_TAB,
                       IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillSwitch(&MovementWES[DIRECTION_WEST], "MOTION_WEST", "West", ISS_OFF);
    IUFillSwitch(&MovementWES[DIRECTION_EAST], "MOTION_EAST", "East", ISS_OFF);
    IUFillSwitchVector(&MovementWESP, MovementWES, 2, getDeviceName(), "TELESCOPE_MOTION_WE", "Motion W/E", MOTION_TAB,
                       IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillNumber(&ScopeParametersN[0], "TELESCOPE_APERTURE", "Aperture (mm)", "%g", 10, 5000, 0, 0.0);
    IUFillNumber(&ScopeParametersN[1], "TELESCOPE_FOCAL_LENGTH", "Focal Length (mm)", "%g", 10, 10000, 0, 0.0);
    IUFillNumber(&ScopeParametersN[2], "GUIDER_APERTURE", "Guider Aperture (mm)", "%g", 10, 5000, 0, 0.0);
    IUFillNumber(&ScopeParametersN[3], "GUIDER_FOCAL_LENGTH", "Guider Focal Length (mm)", "%g", 10, 10000, 0, 0.0);
    IUFillNumberVector(&ScopeParametersNP, ScopeParametersN, 4, getDeviceName(), "TELESCOPE_INFO", "Scope Properties",
                       OPTIONS_TAB, IP_RW, 60, IPS_OK);

    // Scope config name
    IUFillText(&ScopeConfigNameT[0], "SCOPE_CONFIG_NAME", "Config Name", "");
    IUFillTextVector(&ScopeConfigNameTP, ScopeConfigNameT, 1, getDeviceName(), "SCOPE_CONFIG_NAME", "Scope Name",
                     OPTIONS_TAB, IP_RW, 60, IPS_OK);

    // Switch for aperture/focal length configs
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG1], "SCOPE_CONFIG1", "Config #1", ISS_ON);
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG2], "SCOPE_CONFIG2", "Config #2", ISS_OFF);
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG3], "SCOPE_CONFIG3", "Config #3", ISS_OFF);
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG4], "SCOPE_CONFIG4", "Config #4", ISS_OFF);
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG5], "SCOPE_CONFIG5", "Config #5", ISS_OFF);
    IUFillSwitch(&ScopeConfigs[SCOPE_CONFIG6], "SCOPE_CONFIG6", "Config #6", ISS_OFF);
    IUFillSwitchVector(&ScopeConfigsSP, ScopeConfigs, 6, getDeviceName(), "APPLY_SCOPE_CONFIG", "Scope Configs",
                       OPTIONS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_OK);

    controller->initProperties();

    // Joystick motion control
    IUFillSwitch(&MotionControlModeT[0], "MOTION_CONTROL_MODE_JOYSTICK", "4-Way Joystick", ISS_ON);
    IUFillSwitch(&MotionControlModeT[1], "MOTION_CONTROL_MODE_AXES", "Two Separate Axes", ISS_OFF);
    IUFillSwitchVector(&MotionControlModeTP, MotionControlModeT, 2, getDeviceName(), "MOTION_CONTROL_MODE", "Motion Control",
                       "Joystick", IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    // Lock Axis
    IUFillSwitch(&LockAxisS[0], "LOCK_AXIS_1", "West/East", ISS_OFF);
    IUFillSwitch(&LockAxisS[1], "LOCK_AXIS_2", "North/South", ISS_OFF);
    IUFillSwitchVector(&LockAxisSP, LockAxisS, 2, getDeviceName(), "JOYSTICK_LOCK_AXIS", "Lock Axis", "Joystick", IP_RW,
                       ISR_ATMOST1, 60, IPS_IDLE);

    TrackState = SCOPE_IDLE;

    setDriverInterface(TELESCOPE_INTERFACE);

    if (telescopeConnection & CONNECTION_SERIAL)
    {
        serialConnection = new Connection::Serial(this);
        serialConnection->registerHandshake([&]()
        {
            return callHandshake();
        });
        registerConnection(serialConnection);
    }

    if (telescopeConnection & CONNECTION_TCP)
    {
        tcpConnection = new Connection::TCP(this);
        tcpConnection->registerHandshake([&]()
        {
            return callHandshake();
        });

        registerConnection(tcpConnection);
    }

    IDSnoopDevice(ActiveDeviceT[0].text, "GEOGRAPHIC_COORD");
    IDSnoopDevice(ActiveDeviceT[0].text, "TIME_UTC");

    IDSnoopDevice(ActiveDeviceT[1].text, "DOME_PARK");
    IDSnoopDevice(ActiveDeviceT[1].text, "DOME_SHUTTER");

    addPollPeriodControl();

    return true;
}

void Telescope::ISGetProperties(const char *dev)
{
    //  First we let our parent populate
    DefaultDevice::ISGetProperties(dev);

    if (CanGOTO())
    {
        defineText(&ActiveDeviceTP);
        loadConfig(true, "ACTIVE_DEVICES");

        defineSwitch(&DomeClosedLockTP);
        loadConfig(true, "DOME_POLICY");
    }

    defineNumber(&ScopeParametersNP);
    defineText(&ScopeConfigNameTP);

    if (HasDefaultScopeConfig())
    {
        LoadScopeConfig();
    }
    else
    {
        loadConfig(true, "TELESCOPE_INFO");
        loadConfig(true, "SCOPE_CONFIG_NAME");
    }

    /*
    if (isConnected())
    {
        //  Now we add our telescope specific stuff

        if (CanGOTO())
            defineSwitch(&CoordSP);
        defineNumber(&EqNP);
        if (CanAbort())
            defineSwitch(&AbortSP);
        if (HasTrackMode() && TrackModeS != nullptr)
            defineSwitch(&TrackModeSP);
        if (CanControlTrack())
            defineSwitch(&TrackStateSP);
        if (HasTrackRate())
            defineNumber(&TrackRateNP);


        if (HasTime())
            defineText(&TimeTP);
        if (HasLocation())
            defineNumber(&LocationNP);

        if (CanPark())
        {
            defineSwitch(&ParkSP);
            if (parkDataType != PARK_NONE)
            {
                defineNumber(&ParkPositionNP);
                defineSwitch(&ParkOptionSP);
            }
        }

        if (CanGOTO())
        {
            defineSwitch(&MovementNSSP);
            defineSwitch(&MovementWESP);

            if (nSlewRate >= 4)
                defineSwitch(&SlewRateSP);

            defineNumber(&TargetNP);
        }

        if (HasPierSide())
            defineSwitch(&PierSideSP);

        if (HasPECState())
            defineSwitch(&PECStateSP);

        defineSwitch(&ScopeConfigsSP);
    }
    */

    if (CanGOTO())
        controller->ISGetProperties(dev);
}

bool Telescope::updateProperties()
{
    if (isConnected())
    {
        controller->mapController("MOTIONDIR", "N/S/W/E Control", Controller::CONTROLLER_JOYSTICK, "JOYSTICK_1");
        controller->mapController("MOTIONDIRNS", "N/S Control", Controller::CONTROLLER_AXIS, "AXIS_8");
        controller->mapController("MOTIONDIRWE", "W/E Control", Controller::CONTROLLER_AXIS, "AXIS_7");

        if (nSlewRate >= 4)
        {
            controller->mapController("SLEWPRESET", "Slew Rate", Controller::CONTROLLER_JOYSTICK, "JOYSTICK_2");
            controller->mapController("SLEWPRESETUP", "Slew Rate Up", Controller::CONTROLLER_BUTTON, "BUTTON_5");
            controller->mapController("SLEWPRESETDOWN", "Slew Rate Down", Controller::CONTROLLER_BUTTON,
                                      "BUTTON_6");
        }
        if (CanAbort())
            controller->mapController("ABORTBUTTON", "Abort", Controller::CONTROLLER_BUTTON, "BUTTON_1");
        if (CanPark())
        {
            controller->mapController("PARKBUTTON", "Park", Controller::CONTROLLER_BUTTON, "BUTTON_2");
            controller->mapController("UNPARKBUTTON", "UnPark", Controller::CONTROLLER_BUTTON, "BUTTON_3");
        }

        //  Now we add our telescope specific stuff
        if (CanGOTO() || CanSync())
            defineSwitch(&CoordSP);
        defineNumber(&EqNP);
        if (CanAbort())
            defineSwitch(&AbortSP);

        if (HasTrackMode() && TrackModeS != nullptr)
            defineSwitch(&TrackModeSP);
        if (CanControlTrack())
            defineSwitch(&TrackStateSP);
        if (HasTrackRate())
            defineNumber(&TrackRateNP);


        if (CanGOTO())
        {
            defineSwitch(&MovementNSSP);
            defineSwitch(&MovementWESP);
            if (nSlewRate >= 4)
                defineSwitch(&SlewRateSP);
            defineNumber(&TargetNP);
        }

        if (HasTime())
            defineText(&TimeTP);
        if (HasLocation())
            defineNumber(&LocationNP);
        if (CanPark())
        {
            defineSwitch(&ParkSP);
            if (parkDataType != PARK_NONE)
            {
                defineNumber(&ParkPositionNP);
                defineSwitch(&ParkOptionSP);
            }
        }

        if (HasPierSide())
            defineSwitch(&PierSideSP);

        if (HasPierSideSimulation())
        {
            defineSwitch(&SimulatePierSideSP);
            ISState value;
            if (IUGetConfigSwitch(getDefaultName(), "SIMULATE_PIER_SIDE", "SIMULATE_YES", &value) )
                setSimulatePierSide(value == ISS_ON);
        }

        if (HasPECState())
            defineSwitch(&PECStateSP);

        defineText(&ScopeConfigNameTP);
        defineSwitch(&ScopeConfigsSP);
    }
    else
    {
        if (CanGOTO() || CanSync())
            deleteProperty(CoordSP.name);
        deleteProperty(EqNP.name);
        if (CanAbort())
            deleteProperty(AbortSP.name);
        if (HasTrackMode() && TrackModeS != nullptr)
            deleteProperty(TrackModeSP.name);
        if (HasTrackRate())
            deleteProperty(TrackRateNP.name);
        if (CanControlTrack())
            deleteProperty(TrackStateSP.name);

        if (CanGOTO())
        {
            deleteProperty(MovementNSSP.name);
            deleteProperty(MovementWESP.name);
            if (nSlewRate >= 4)
                deleteProperty(SlewRateSP.name);
            deleteProperty(TargetNP.name);
        }

        if (HasTime())
            deleteProperty(TimeTP.name);
        if (HasLocation())
            deleteProperty(LocationNP.name);

        if (CanPark())
        {
            deleteProperty(ParkSP.name);
            if (parkDataType != PARK_NONE)
            {
                deleteProperty(ParkPositionNP.name);
                deleteProperty(ParkOptionSP.name);
            }
        }

        if (HasPierSide())
            deleteProperty(PierSideSP.name);

        if (HasPierSideSimulation())
        {
            deleteProperty(SimulatePierSideSP.name);
            if (getSimulatePierSide() == true)
                deleteProperty(PierSideSP.name);
        }

        if (HasPECState())
            deleteProperty(PECStateSP.name);

        deleteProperty(ScopeConfigNameTP.name);
        deleteProperty(ScopeConfigsSP.name);
    }

    if (CanGOTO())
    {
        controller->updateProperties();

        ISwitchVectorProperty *useJoystick = getSwitch("USEJOYSTICK");
        if (useJoystick)
        {
            if (isConnected())
            {
                if (useJoystick->sp[0].s == ISS_ON)
                {
                    defineSwitch(&MotionControlModeTP);
                    loadConfig(true, "MOTION_CONTROL_MODE");
                    defineSwitch(&LockAxisSP);
                    loadConfig(true, "LOCK_AXIS");
                }
                else
                {
                    deleteProperty(MotionControlModeTP.name);
                    deleteProperty(LockAxisSP.name);
                }
            }
            else
            {
                deleteProperty(MotionControlModeTP.name);
                deleteProperty(LockAxisSP.name);
            }
        }
    }

    return true;
}

bool Telescope::ISSnoopDevice(XMLEle *root)
{
    controller->ISSnoopDevice(root);

    XMLEle *ep           = nullptr;
    const char *propName = findXMLAttValu(root, "name");

    if (isConnected())
    {
        if (HasLocation() && !strcmp(propName, "GEOGRAPHIC_COORD"))
        {
            // Only accept IPS_OK state
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
                return false;

            double longitude = -1, latitude = -1, elevation = -1;

            for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
            {
                const char *elemName = findXMLAttValu(ep, "name");

                if (!strcmp(elemName, "LAT"))
                    latitude = atof(pcdataXMLEle(ep));
                else if (!strcmp(elemName, "LONG"))
                    longitude = atof(pcdataXMLEle(ep));
                else if (!strcmp(elemName, "ELEV"))
                    elevation = atof(pcdataXMLEle(ep));
            }

            return processLocationInfo(latitude, longitude, elevation);
        }
        else if (HasTime() && !strcmp(propName, "TIME_UTC"))
        {
            // Only accept IPS_OK state
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
                return false;

            char utc[MAXINDITSTAMP], offset[MAXINDITSTAMP];

            for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
            {
                const char *elemName = findXMLAttValu(ep, "name");

                if (!strcmp(elemName, "UTC"))
                    strncpy(utc, pcdataXMLEle(ep), MAXINDITSTAMP);
                else if (!strcmp(elemName, "OFFSET"))
                    strncpy(offset, pcdataXMLEle(ep), MAXINDITSTAMP);
            }

            return processTimeInfo(utc, offset);
        }
        else if (!strcmp(propName, "DOME_PARK") || !strcmp(propName, "DOME_SHUTTER"))
        {
            if (strcmp(findXMLAttValu(root, "state"), "Ok"))
            {
                // Dome options is dome parks or both and dome is parking.
                if ((DomeClosedLockT[2].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON) && !IsLocked && !IsParked)
                {
                    for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
                    {
                        const char * elemName = findXMLAttValu(ep, "name");
                        if (( (!strcmp(elemName, "SHUTTER_CLOSE") || !strcmp(elemName, "PARK"))
                                && !strcmp(pcdataXMLEle(ep), "On")))
                        {
                            RememberTrackState = TrackState;
                            Park();
                            LOG_INFO("Dome is closing, parking mount...");
                        }
                    }
                }
            } // Dome is changing state and Dome options is lock or both. d
            else if (!strcmp(findXMLAttValu(root, "state"), "Ok"))
            {
                bool prevState = IsLocked;
                for (ep = nextXMLEle(root, 1); ep != nullptr; ep = nextXMLEle(root, 0))
                {
                    const char *elemName = findXMLAttValu(ep, "name");

                    if (!IsLocked && (!strcmp(elemName, "PARK")) && !strcmp(pcdataXMLEle(ep), "On"))
                        IsLocked = true;
                    else if (IsLocked && (!strcmp(elemName, "UNPARK")) && !strcmp(pcdataXMLEle(ep), "On"))
                        IsLocked = false;
                }
                if (prevState != IsLocked && (DomeClosedLockT[1].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON))
                    LOGF_INFO("Dome status changed. Lock is set to: %s",
                              IsLocked ? "locked" : "unlock");
            }
            return true;
        }
    }

    return DefaultDevice::ISSnoopDevice(root);
}

void Telescope::triggerSnoop(const char *driverName, const char *snoopedProp)
{
    LOGF_DEBUG("Active Snoop, driver: %s, property: %s", driverName, snoopedProp);
    IDSnoopDevice(driverName, snoopedProp);
}

uint8_t Telescope::getTelescopeConnection() const
{
    return telescopeConnection;
}

void Telescope::setTelescopeConnection(const uint8_t &value)
{
    uint8_t mask = CONNECTION_SERIAL | CONNECTION_TCP | CONNECTION_NONE;

    if (value == 0 || (mask & value) == 0)
    {
        DEBUGF(Logger::DBG_ERROR, "Invalid connection mode %d", value);
        return;
    }

    telescopeConnection = value;
}

bool Telescope::saveConfigItems(FILE *fp)
{
    DefaultDevice::saveConfigItems(fp);

    IUSaveConfigText(fp, &ActiveDeviceTP);
    IUSaveConfigSwitch(fp, &DomeClosedLockTP);

    if (HasLocation())
        IUSaveConfigNumber(fp, &LocationNP);

    if (!HasDefaultScopeConfig())
    {
        if (ScopeParametersNP.s == IPS_OK)
            IUSaveConfigNumber(fp, &ScopeParametersNP);
        if (ScopeConfigNameTP.s == IPS_OK)
            IUSaveConfigText(fp, &ScopeConfigNameTP);
    }
    if (SlewRateS != nullptr)
        IUSaveConfigSwitch(fp, &SlewRateSP);
    if (HasPECState())
        IUSaveConfigSwitch(fp, &PECStateSP);
    if (HasTrackMode())
        IUSaveConfigSwitch(fp, &TrackModeSP);
    if (HasTrackRate())
        IUSaveConfigNumber(fp, &TrackRateNP);

    controller->saveConfigItems(fp);
    IUSaveConfigSwitch(fp, &MotionControlModeTP);
    IUSaveConfigSwitch(fp, &LockAxisSP);
    IUSaveConfigSwitch(fp, &SimulatePierSideSP);

    return true;
}

void Telescope::NewRaDec(double ra, double dec)
{
    switch (TrackState)
    {
        case SCOPE_PARKED:
        case SCOPE_IDLE:
            EqNP.s = IPS_IDLE;
            break;

        case SCOPE_SLEWING:
        case SCOPE_PARKING:
            EqNP.s = IPS_BUSY;
            break;

        case SCOPE_TRACKING:
            EqNP.s = IPS_OK;
            break;
    }

    if (TrackState != SCOPE_TRACKING && CanControlTrack() && TrackStateS[TRACK_ON].s == ISS_ON)
    {
        TrackStateSP.s = IPS_IDLE;
        TrackStateS[TRACK_ON].s = ISS_OFF;
        TrackStateS[TRACK_OFF].s = ISS_ON;
        IDSetSwitch(&TrackStateSP, nullptr);
    }
    else if (TrackState == SCOPE_TRACKING && CanControlTrack() && TrackStateS[TRACK_OFF].s == ISS_ON)
    {
        TrackStateSP.s = IPS_BUSY;
        TrackStateS[TRACK_ON].s = ISS_ON;
        TrackStateS[TRACK_OFF].s = ISS_OFF;
        IDSetSwitch(&TrackStateSP, nullptr);
    }

    if (EqN[AXIS_RA].value != ra || EqN[AXIS_DE].value != dec || EqNP.s != lastEqState)
    {
        EqN[AXIS_RA].value = ra;
        EqN[AXIS_DE].value = dec;
        lastEqState        = EqNP.s;
        IDSetNumber(&EqNP, nullptr);
    }
}

bool Telescope::Sync(double ra, double dec)
{
    INDI_UNUSED(ra);
    INDI_UNUSED(dec);
    //  if we get here, our mount doesn't support sync
    DEBUG(Logger::DBG_ERROR, "Telescope does not support Sync.");
    return false;
}

bool Telescope::MoveNS(INDI_DIR_NS dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    DEBUG(Logger::DBG_ERROR, "Telescope does not support North/South motion.");
    IUResetSwitch(&MovementNSSP);
    MovementNSSP.s = IPS_IDLE;
    IDSetSwitch(&MovementNSSP, nullptr);
    return false;
}

bool Telescope::MoveWE(INDI_DIR_WE dir, TelescopeMotionCommand command)
{
    INDI_UNUSED(dir);
    INDI_UNUSED(command);
    DEBUG(Logger::DBG_ERROR, "Telescope does not support West/East motion.");
    IUResetSwitch(&MovementWESP);
    MovementWESP.s = IPS_IDLE;
    IDSetSwitch(&MovementWESP, nullptr);
    return false;
}

/**************************************************************************************
** Process Text properties
***************************************************************************************/
bool Telescope::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    //  first check if it's for our device
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (!strcmp(name, TimeTP.name))
        {
            int utcindex    = IUFindIndex("UTC", names, n);
            int offsetindex = IUFindIndex("OFFSET", names, n);

            return processTimeInfo(texts[utcindex], texts[offsetindex]);
        }

        if (!strcmp(name, ActiveDeviceTP.name))
        {
            ActiveDeviceTP.s = IPS_OK;
            IUUpdateText(&ActiveDeviceTP, texts, names, n);
            //  Update client display
            IDSetText(&ActiveDeviceTP, nullptr);

            IDSnoopDevice(ActiveDeviceT[0].text, "GEOGRAPHIC_COORD");
            IDSnoopDevice(ActiveDeviceT[0].text, "TIME_UTC");

            IDSnoopDevice(ActiveDeviceT[1].text, "DOME_PARK");
            IDSnoopDevice(ActiveDeviceT[1].text, "DOME_SHUTTER");
            return true;
        }

        if (name && std::string(name) == "SCOPE_CONFIG_NAME")
        {
            ScopeConfigNameTP.s = IPS_OK;
            IUUpdateText(&ScopeConfigNameTP, texts, names, n);
            IDSetText(&ScopeConfigNameTP, nullptr);
            UpdateScopeConfig();
            return true;
        }
    }

    controller->ISNewText(dev, name, texts, names, n);

    return DefaultDevice::ISNewText(dev, name, texts, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
bool Telescope::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    //  first check if it's for our device
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        ///////////////////////////////////
        // Goto & Sync for Equatorial Coords
        ///////////////////////////////////
        if (strcmp(name, "EQUATORIAL_EOD_COORD") == 0)
        {
            //  this is for us, and it is a goto
            bool rc    = false;
            double ra  = -1;
            double dec = -100;

            for (int x = 0; x < n; x++)
            {
                INumber *eqp = IUFindNumber(&EqNP, names[x]);
                if (eqp == &EqN[AXIS_RA])
                {
                    ra = values[x];
                }
                else if (eqp == &EqN[AXIS_DE])
                {
                    dec = values[x];
                }
            }
            if ((ra >= 0) && (ra <= 24) && (dec >= -90) && (dec <= 90))
            {
                // Check if it is already parked.
                if (CanPark())
                {
                    if (isParked())
                    {
                        DEBUG(Logger::DBG_WARNING,
                              "Please unpark the mount before issuing any motion/sync commands.");
                        EqNP.s = lastEqState = IPS_IDLE;
                        IDSetNumber(&EqNP, nullptr);
                        return false;
                    }
                }

                // Check if it can sync
                if (CanSync())
                {
                    ISwitch *sw;
                    sw = IUFindSwitch(&CoordSP, "SYNC");
                    if ((sw != nullptr) && (sw->s == ISS_ON))
                    {
                        rc = Sync(ra, dec);
                        if (rc)
                            EqNP.s = lastEqState = IPS_OK;
                        else
                            EqNP.s = lastEqState = IPS_ALERT;
                        IDSetNumber(&EqNP, nullptr);
                        return rc;
                    }
                }

                // Remember Track State
                RememberTrackState = TrackState;
                // Issue GOTO
                rc = Goto(ra, dec);
                if (rc)
                {
                    EqNP.s = lastEqState = IPS_BUSY;
                    //  Now fill in target co-ords, so domes can start turning
                    TargetN[AXIS_RA].value = ra;
                    TargetN[AXIS_DE].value = dec;
                    IDSetNumber(&TargetNP, nullptr);
                }
                else
                {
                    EqNP.s = lastEqState = IPS_ALERT;
                }
                IDSetNumber(&EqNP, nullptr);
            }
            return rc;
        }

        ///////////////////////////////////
        // Geographic Coords
        ///////////////////////////////////
        if (strcmp(name, "GEOGRAPHIC_COORD") == 0)
        {
            int latindex       = IUFindIndex("LAT", names, n);
            int longindex      = IUFindIndex("LONG", names, n);
            int elevationindex = IUFindIndex("ELEV", names, n);

            if (latindex == -1 || longindex == -1 || elevationindex == -1)
            {
                LocationNP.s = IPS_ALERT;
                IDSetNumber(&LocationNP, "Location data missing or corrupted.");
            }

            double targetLat  = values[latindex];
            double targetLong = values[longindex];
            double targetElev = values[elevationindex];

            return processLocationInfo(targetLat, targetLong, targetElev);
        }

        ///////////////////////////////////
        // Telescope Info
        ///////////////////////////////////
        if (strcmp(name, "TELESCOPE_INFO") == 0)
        {
            ScopeParametersNP.s = IPS_OK;

            IUUpdateNumber(&ScopeParametersNP, values, names, n);
            IDSetNumber(&ScopeParametersNP, nullptr);
            UpdateScopeConfig();
            return true;
        }

        ///////////////////////////////////
        // Park Position
        ///////////////////////////////////
        if (strcmp(name, ParkPositionNP.name) == 0)
        {
            double axis1 = std::numeric_limits<double>::quiet_NaN(), axis2 = std::numeric_limits<double>::quiet_NaN();
            for (int x = 0; x < n; x++)
            {
                INumber *parkPosAxis = IUFindNumber(&ParkPositionNP, names[x]);
                if (parkPosAxis == &ParkPositionN[AXIS_RA])
                {
                    axis1 = values[x];
                }
                else if (parkPosAxis == &ParkPositionN[AXIS_DE])
                {
                    axis2 = values[x];
                }
            }

            if (std::isnan(axis1) == false && std::isnan(axis2) == false)
            {
                bool rc = false;

                rc = SetParkPosition(axis1, axis2);

                if (rc)
                {
                    IUUpdateNumber(&ParkPositionNP, values, names, n);
                    Axis1ParkPosition = ParkPositionN[AXIS_RA].value;
                    Axis2ParkPosition = ParkPositionN[AXIS_DE].value;
                }

                ParkPositionNP.s = rc ? IPS_OK : IPS_ALERT;
            }
            else
                ParkPositionNP.s = IPS_ALERT;

            IDSetNumber(&ParkPositionNP, nullptr);
            return true;
        }

        ///////////////////////////////////
        // Track Rate
        ///////////////////////////////////
        if (strcmp(name, TrackRateNP.name) == 0)
        {
            double preAxis1 = TrackRateN[AXIS_RA].value, preAxis2 = TrackRateN[AXIS_DE].value;
            bool rc = (IUUpdateNumber(&TrackRateNP, values, names, n) == 0);

            if (!rc)
            {
                TrackRateNP.s = IPS_ALERT;
                IDSetNumber(&TrackRateNP, nullptr);
                return false;
            }

            if (TrackState == SCOPE_TRACKING && !strcmp(IUFindOnSwitch(&TrackModeSP)->name, "TRACK_CUSTOM"))
            {
                // Check that we do not abruplty change positive tracking rates to negative ones.
                // tracking must be stopped first.
                // Give warning is tracking sign would cause a reverse in direction
                if ( (preAxis1 * TrackRateN[AXIS_RA].value < 0) || (preAxis2 * TrackRateN[AXIS_DE].value < 0) )
                {
                    DEBUG(Logger::DBG_ERROR, "Cannot reverse tracking while tracking is engaged. Disengage tracking then try again.");
                    return false;
                }

                // All is fine, ask mount to change tracking rate
                rc = SetTrackRate(TrackRateN[AXIS_RA].value, TrackRateN[AXIS_DE].value);

                if (!rc)
                {
                    TrackRateN[AXIS_RA].value = preAxis1;
                    TrackRateN[AXIS_DE].value = preAxis2;
                }
            }

            // If we are already tracking but tracking mode is NOT custom
            // We just inform the user that it must be set to custom for these values to take
            // effect.
            if (TrackState == SCOPE_TRACKING && strcmp(IUFindOnSwitch(&TrackModeSP)->name, "TRACK_CUSTOM"))
            {
                LOG_INFO("Custom tracking rates set. Tracking mode must be set to Custom for these rates to take effect.");
            }

            // If mount is NOT tracking, we simply accept whatever valid values for use when mount tracking is engaged.
            TrackRateNP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetNumber(&TrackRateNP, nullptr);
            return true;
        }
    }

    return DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

/**************************************************************************************
**
***************************************************************************************/
bool Telescope::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        //  This one is for us
        if (!strcmp(name, CoordSP.name))
        {
            //  client is telling us what to do with co-ordinate requests
            CoordSP.s = IPS_OK;
            IUUpdateSwitch(&CoordSP, states, names, n);
            //  Update client display
            IDSetSwitch(&CoordSP, nullptr);
            return true;
        }

        ///////////////////////////////////
        // Slew Rate
        ///////////////////////////////////
        if (!strcmp(name, SlewRateSP.name))
        {
            int preIndex = IUFindOnSwitchIndex(&SlewRateSP);
            IUUpdateSwitch(&SlewRateSP, states, names, n);
            int nowIndex = IUFindOnSwitchIndex(&SlewRateSP);
            if (SetSlewRate(nowIndex) == false)
            {
                IUResetSwitch(&SlewRateSP);
                SlewRateS[preIndex].s = ISS_ON;
                SlewRateSP.s          = IPS_ALERT;
            }
            else
                SlewRateSP.s = IPS_OK;
            IDSetSwitch(&SlewRateSP, nullptr);
            return true;
        }

        ///////////////////////////////////
        // Parking
        ///////////////////////////////////
        if (!strcmp(name, ParkSP.name))
        {
            if (TrackState == SCOPE_PARKING)
            {
                IUResetSwitch(&ParkSP);
                ParkSP.s = IPS_ALERT;
                Abort();
                LOG_INFO("Parking/Unparking aborted.");
                IDSetSwitch(&ParkSP, nullptr);
                return true;
            }

            int preIndex = IUFindOnSwitchIndex(&ParkSP);
            IUUpdateSwitch(&ParkSP, states, names, n);

            bool toPark = (ParkS[0].s == ISS_ON);

            if (toPark == false && TrackState != SCOPE_PARKED)
            {
                IUResetSwitch(&ParkSP);
                ParkS[1].s = ISS_ON;
                ParkSP.s   = IPS_IDLE;
                LOG_INFO("Telescope already unparked.");
                IsParked = false;
                IDSetSwitch(&ParkSP, nullptr);
                return true;
            }

            if (toPark == false && isLocked())
            {
                IUResetSwitch(&ParkSP);
                ParkS[0].s = ISS_ON;
                ParkSP.s   = IPS_IDLE;
                LOG_WARN("Cannot unpark mount when dome is locking. See: Dome parking policy, in options tab.");
                IsParked = true;
                IDSetSwitch(&ParkSP, nullptr);
                return true;
            }

            if (toPark && TrackState == SCOPE_PARKED)
            {
                IUResetSwitch(&ParkSP);
                ParkS[0].s = ISS_ON;
                ParkSP.s   = IPS_IDLE;
                LOG_INFO("Telescope already parked.");
                IDSetSwitch(&ParkSP, nullptr);
                return true;
            }

            RememberTrackState = TrackState;

            IUResetSwitch(&ParkSP);
            bool rc = toPark ? Park() : UnPark();
            if (rc)
            {
                if (TrackState == SCOPE_PARKING)
                {
                    ParkS[0].s = toPark ? ISS_ON : ISS_OFF;
                    ParkS[1].s = toPark ? ISS_OFF : ISS_ON;
                    ParkSP.s   = IPS_BUSY;
                }
                else
                {
                    ParkS[0].s = toPark ? ISS_ON : ISS_OFF;
                    ParkS[1].s = toPark ? ISS_OFF : ISS_ON;
                    ParkSP.s   = IPS_OK;
                }
            }
            else
            {
                ParkS[preIndex].s = ISS_ON;
                ParkSP.s          = IPS_ALERT;
            }

            IDSetSwitch(&ParkSP, nullptr);
            return true;
        }

        ///////////////////////////////////
        // NS Motion
        ///////////////////////////////////
        if (!strcmp(name, MovementNSSP.name))
        {
            // Check if it is already parked.
            if (CanPark())
            {
                if (isParked())
                {
                    DEBUG(Logger::DBG_WARNING,
                          "Please unpark the mount before issuing any motion/sync commands.");
                    MovementNSSP.s = IPS_IDLE;
                    IDSetSwitch(&MovementNSSP, nullptr);
                    return false;
                }
            }

            IUUpdateSwitch(&MovementNSSP, states, names, n);

            int current_motion = IUFindOnSwitchIndex(&MovementNSSP);

            // if same move requested, return
            if (MovementNSSP.s == IPS_BUSY && current_motion == last_ns_motion)
                return true;

            // Time to stop motion
            if (current_motion == -1 || (last_ns_motion != -1 && current_motion != last_ns_motion))
            {
                if (MoveNS(last_ns_motion == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP))
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_IDLE;
                    last_ns_motion = -1;
                }
                else
                    MovementNSSP.s = IPS_ALERT;
            }
            else
            {
                if (TrackState != SCOPE_SLEWING && TrackState != SCOPE_PARKING)
                    RememberTrackState = TrackState;

                if (MoveNS(current_motion == 0 ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_START))
                {
                    MovementNSSP.s = IPS_BUSY;
                    last_ns_motion = current_motion;
                }
                else
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_ALERT;
                    last_ns_motion = -1;
                }
            }

            IDSetSwitch(&MovementNSSP, nullptr);

            return true;
        }

        ///////////////////////////////////
        // WE Motion
        ///////////////////////////////////
        if (!strcmp(name, MovementWESP.name))
        {
            // Check if it is already parked.
            if (CanPark())
            {
                if (isParked())
                {
                    DEBUG(Logger::DBG_WARNING,
                          "Please unpark the mount before issuing any motion/sync commands.");
                    MovementWESP.s = IPS_IDLE;
                    IDSetSwitch(&MovementWESP, nullptr);
                    return false;
                }
            }

            IUUpdateSwitch(&MovementWESP, states, names, n);

            int current_motion = IUFindOnSwitchIndex(&MovementWESP);

            // if same move requested, return
            if (MovementWESP.s == IPS_BUSY && current_motion == last_we_motion)
                return true;

            // Time to stop motion
            if (current_motion == -1 || (last_we_motion != -1 && current_motion != last_we_motion))
            {
                if (MoveWE(last_we_motion == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP))
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_IDLE;
                    last_we_motion = -1;
                }
                else
                    MovementWESP.s = IPS_ALERT;
            }
            else
            {
                if (TrackState != SCOPE_SLEWING && TrackState != SCOPE_PARKING)
                    RememberTrackState = TrackState;

                if (MoveWE(current_motion == 0 ? DIRECTION_WEST : DIRECTION_EAST, MOTION_START))
                {
                    MovementWESP.s = IPS_BUSY;
                    last_we_motion = current_motion;
                }
                else
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_ALERT;
                    last_we_motion = -1;
                }
            }

            IDSetSwitch(&MovementWESP, nullptr);

            return true;
        }

        ///////////////////////////////////
        // Abort Motion
        ///////////////////////////////////
        if (!strcmp(name, AbortSP.name))
        {
            IUResetSwitch(&AbortSP);

            if (Abort())
            {
                AbortSP.s = IPS_OK;

                if (ParkSP.s == IPS_BUSY)
                {
                    IUResetSwitch(&ParkSP);
                    ParkSP.s = IPS_ALERT;
                    IDSetSwitch(&ParkSP, nullptr);

                    LOG_INFO("Parking aborted.");
                }
                if (EqNP.s == IPS_BUSY)
                {
                    EqNP.s = lastEqState = IPS_IDLE;
                    IDSetNumber(&EqNP, nullptr);
                    LOG_INFO("Slew/Track aborted.");
                }
                if (MovementWESP.s == IPS_BUSY)
                {
                    IUResetSwitch(&MovementWESP);
                    MovementWESP.s = IPS_IDLE;
                    IDSetSwitch(&MovementWESP, nullptr);
                }
                if (MovementNSSP.s == IPS_BUSY)
                {
                    IUResetSwitch(&MovementNSSP);
                    MovementNSSP.s = IPS_IDLE;
                    IDSetSwitch(&MovementNSSP, nullptr);
                }

                last_ns_motion = last_we_motion = -1;

                // JM 2017-07-28: Abort shouldn't affect tracking state. It should affect motion and that's it.
                //if (TrackState != SCOPE_PARKED)
                //TrackState = SCOPE_IDLE;
                // For Idle, Tracking, Parked state, we do not change its status, it should remain as is.
                // For Slewing & Parking, state should go back to last rememberd state.
                if (TrackState == SCOPE_SLEWING || TrackState == SCOPE_PARKING)
                {
                    TrackState = RememberTrackState;
                }
            }
            else
                AbortSP.s = IPS_ALERT;

            IDSetSwitch(&AbortSP, nullptr);

            return true;
        }

        ///////////////////////////////////
        // Track Mode
        ///////////////////////////////////
        if (!strcmp(name, TrackModeSP.name))
        {
            int prevIndex = IUFindOnSwitchIndex(&TrackModeSP);
            IUUpdateSwitch(&TrackModeSP, states, names, n);
            int currIndex = IUFindOnSwitchIndex(&TrackModeSP);
            // If same as previous index, or if scope is already idle, then just update switch and return. No commands are sent to the mount.
            if (prevIndex == currIndex || TrackState == SCOPE_IDLE)
            {
                TrackModeSP.s = IPS_OK;
                IDSetSwitch(&TrackModeSP, nullptr);
                return true;
            }

            if (TrackState == SCOPE_PARKED)
            {
                DEBUG(Logger::DBG_WARNING, "Telescope is Parked, Unpark before changing track mode.");
                return false;
            }

            bool rc = SetTrackMode(currIndex);
            if (rc)
                TrackModeSP.s = IPS_OK;
            else
            {
                IUResetSwitch(&TrackModeSP);
                TrackModeS[prevIndex].s = ISS_ON;
                TrackModeSP.s = IPS_ALERT;
            }
            IDSetSwitch(&TrackModeSP, nullptr);
            return false;
        }

        ///////////////////////////////////
        // Track State
        ///////////////////////////////////
        if (!strcmp(name, TrackStateSP.name))
        {
            int previousState = IUFindOnSwitchIndex(&TrackStateSP);
            IUUpdateSwitch(&TrackStateSP, states, names, n);
            int targetState = IUFindOnSwitchIndex(&TrackStateSP);

            if (previousState == targetState)
            {
                IDSetSwitch(&TrackStateSP, nullptr);
                return true;
            }

            if (TrackState == SCOPE_PARKED)
            {
                DEBUG(Logger::DBG_WARNING, "Telescope is Parked, Unpark before tracking.");
                return false;
            }

            bool rc = SetTrackEnabled((targetState == TRACK_ON) ? true : false);

            if (rc)
            {
                TrackState = (targetState == TRACK_ON) ? SCOPE_TRACKING : SCOPE_IDLE;

                TrackStateSP.s = (targetState == TRACK_ON) ? IPS_BUSY : IPS_IDLE;

                TrackStateS[TRACK_ON].s = (targetState == TRACK_ON) ? ISS_ON : ISS_OFF;
                TrackStateS[TRACK_OFF].s = (targetState == TRACK_ON) ? ISS_OFF : ISS_ON;
            }
            else
            {
                TrackStateSP.s = IPS_ALERT;
                IUResetSwitch(&TrackStateSP);
                TrackStateS[previousState].s = ISS_ON;
            }

            IDSetSwitch(&TrackStateSP, nullptr);
            return true;
        }

        ///////////////////////////////////
        // Park Options
        ///////////////////////////////////
        if (!strcmp(name, ParkOptionSP.name))
        {
            IUUpdateSwitch(&ParkOptionSP, states, names, n);
            int index = IUFindOnSwitchIndex(&ParkOptionSP);
            if (index == -1)
                return false;

            IUResetSwitch(&ParkOptionSP);

            bool rc = false;

            if ((TrackState != SCOPE_IDLE && TrackState != SCOPE_TRACKING) || MovementNSSP.s == IPS_BUSY ||
                    MovementWESP.s == IPS_BUSY)
            {
                LOG_INFO("Can not change park position while slewing or already parked...");
                ParkOptionSP.s = IPS_ALERT;
                IDSetSwitch(&ParkOptionSP, nullptr);
                return false;
            }

            switch (index)
            {
                case PARK_CURRENT:
                    rc = SetCurrentPark();
                    break;
                case PARK_DEFAULT:
                    rc = SetDefaultPark();
                    break;
                case PARK_WRITE_DATA:
                    rc = WriteParkData();
                    if (rc)
                        LOG_INFO("Saved Park Status/Position.");
                    else
                        DEBUG(Logger::DBG_WARNING, "Can not save Park Status/Position.");
                    break;
                case PARK_PURGE_DATA:
                    rc = PurgeParkData();
                    if (rc)
                        LOG_INFO("Park data purged.");
                    else
                        DEBUG(Logger::DBG_WARNING, "Can not purge Park Status/Position.");
                    break;
            }

            ParkOptionSP.s = rc ? IPS_OK : IPS_ALERT;
            IDSetSwitch(&ParkOptionSP, nullptr);

            return true;
        }

        ///////////////////////////////////
        // Parking Dome Policy
        ///////////////////////////////////
        if (!strcmp(name, DomeClosedLockTP.name))
        {
            if (n == 1)
            {
                if (!strcmp(names[0], DomeClosedLockT[0].name))
                    LOG_INFO("Dome parking policy set to: Ignore dome");
                else if (!strcmp(names[0], DomeClosedLockT[1].name))
                    LOG_INFO("Warning: Dome parking policy set to: Dome locks. This disallows "
                             "the scope from unparking when dome is parked");
                else if (!strcmp(names[0], DomeClosedLockT[2].name))
                    LOG_INFO("Warning: Dome parking policy set to: Dome parks. This tells "
                             "scope to park if dome is parking. This will disable the locking "
                             "for dome parking, EVEN IF MOUNT PARKING FAILS");
                else if (!strcmp(names[0], DomeClosedLockT[3].name))
                    LOG_INFO("Warning: Dome parking policy set to: Both. This disallows the "
                             "scope from unparking when dome is parked, and tells scope to "
                             "park if dome is parking. This will disable the locking for dome "
                             "parking, EVEN IF MOUNT PARKING FAILS.");
            }
            IUUpdateSwitch(&DomeClosedLockTP, states, names, n);
            DomeClosedLockTP.s = IPS_OK;
            IDSetSwitch(&DomeClosedLockTP, nullptr);

            triggerSnoop(ActiveDeviceT[1].text, "DOME_PARK");
            return true;
        }

        ///////////////////////////////////
        // Simulate Pier Side
        // This ia a major change to the design of the simulated scope, it might not handle changes on the fly
        ///////////////////////////////////
        if (!strcmp(name, SimulatePierSideSP.name))
        {
            IUUpdateSwitch(&SimulatePierSideSP, states, names, n);
            int index = IUFindOnSwitchIndex(&SimulatePierSideSP);
            if (index == -1)
            {
                SimulatePierSideSP.s = IPS_ALERT;
                LOG_INFO("Cannot determine whether pier side simulation should be switched on or off.");
                IDSetSwitch(&SimulatePierSideSP, nullptr);
                return false;
            }

            bool pierSideEnabled = index == 0;

            LOGF_INFO("Simulating Pier Side %s.", (pierSideEnabled ? "enabled" : "disabled"));

            setSimulatePierSide(pierSideEnabled);
            if (pierSideEnabled)
            {
                // set the pier side from the current Ra
                // assumes we haven't tracked across the meridian
                setPierSide(expectedPierSide(EqN[AXIS_RA].value));
            }
            return true;
        }

        ///////////////////////////////////
        // Joystick Motion Control Mode
        ///////////////////////////////////
        if (!strcmp(name, MotionControlModeTP.name))
        {
            IUUpdateSwitch(&MotionControlModeTP, states, names, n);
            MotionControlModeTP.s = IPS_OK;
            IDSetSwitch(&MotionControlModeTP, nullptr);
            if (MotionControlModeT[MOTION_CONTROL_JOYSTICK].s == ISS_ON)
                LOG_INFO("Motion control is set to 4-way joystick.");
            else if (MotionControlModeT[MOTION_CONTROL_AXES].s == ISS_ON)
                LOG_INFO("Motion control is set to 2 separate axes.");
            else
                DEBUGF(Logger::DBG_WARNING, "Motion control is set to unknown value %d!", n);
            return true;
        }

        ///////////////////////////////////
        // Joystick Lock Axis
        ///////////////////////////////////
        if (!strcmp(name, LockAxisSP.name))
        {
            IUUpdateSwitch(&LockAxisSP, states, names, n);
            LockAxisSP.s = IPS_OK;
            IDSetSwitch(&LockAxisSP, nullptr);
            if (LockAxisS[AXIS_RA].s == ISS_ON)
                LOG_INFO("Joystick motion is locked to West/East axis only.");
            else if (LockAxisS[AXIS_DE].s == ISS_ON)
                LOG_INFO("Joystick motion is locked to North/South axis only.");
            else
                LOG_INFO("Joystick motion is unlocked.");
            return true;
        }

        ///////////////////////////////////
        // Scope Apply Config
        ///////////////////////////////////
        if (name && std::string(name) == "APPLY_SCOPE_CONFIG")
        {
            IUUpdateSwitch(&ScopeConfigsSP, states, names, n);
            bool rc          = LoadScopeConfig();
            ScopeConfigsSP.s = (rc ? IPS_OK : IPS_ALERT);
            IDSetSwitch(&ScopeConfigsSP, nullptr);
            return true;
        }
    }

    bool rc = controller->ISNewSwitch(dev, name, states, names, n);
    if (rc)
    {
        ISwitchVectorProperty *useJoystick = getSwitch("USEJOYSTICK");
        if (useJoystick && useJoystick->sp[0].s == ISS_ON)
        {
            defineSwitch(&MotionControlModeTP);
            defineSwitch(&LockAxisSP);
        }
        else
        {
            deleteProperty(MotionControlModeTP.name);
            deleteProperty(LockAxisSP.name);
        }

    }

    //  Nobody has claimed this, so, ignore it
    return DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool Telescope::callHandshake()
{
    if (telescopeConnection > 0)
    {
        if (getActiveConnection() == serialConnection)
            PortFD = serialConnection->getPortFD();
        else if (getActiveConnection() == tcpConnection)
            PortFD = tcpConnection->getPortFD();
    }

    return Handshake();
}

bool Telescope::Handshake()
{
    /* Test connection */
    return ReadScopeStatus();
}

void Telescope::TimerHit()
{
    if (isConnected())
    {
        bool rc;

        rc = ReadScopeStatus();

        if (!rc)
        {
            //  read was not good
            EqNP.s = lastEqState = IPS_ALERT;
            IDSetNumber(&EqNP, nullptr);
        }

        SetTimer(POLLMS);
    }
}

bool Telescope::Goto(double ra, double dec)
{
    INDI_UNUSED(ra);
    INDI_UNUSED(dec);

    DEBUG(Logger::DBG_WARNING, "GOTO is not supported.");
    return false;
}

bool Telescope::Abort()
{
    DEBUG(Logger::DBG_WARNING, "Abort is not supported.");
    return false;
}

bool Telescope::Park()
{
    DEBUG(Logger::DBG_WARNING, "Parking is not supported.");
    return false;
}

bool Telescope::UnPark()
{
    DEBUG(Logger::DBG_WARNING, "UnParking is not supported.");
    return false;
}

bool Telescope::SetTrackMode(uint8_t mode)
{
    INDI_UNUSED(mode);
    DEBUG(Logger::DBG_WARNING, "Tracking mode is not supported.");
    return false;
}

bool Telescope::SetTrackRate(double raRate, double deRate)
{
    INDI_UNUSED(raRate);
    INDI_UNUSED(deRate);
    DEBUG(Logger::DBG_WARNING, "Custom tracking rates is not supported.");
    return false;
}

bool Telescope::SetTrackEnabled(bool enabled)
{
    INDI_UNUSED(enabled);
    DEBUG(Logger::DBG_WARNING, "Tracking state is not supported.");
    return false;
}

int Telescope::AddTrackMode(const char *name, const char *label, bool isDefault)
{
    TrackModeS = (TrackModeS == nullptr) ? static_cast<ISwitch *>(malloc(sizeof(ISwitch))) :
                 static_cast<ISwitch *>(realloc(TrackModeS, (TrackModeSP.nsp + 1) * sizeof(ISwitch)));

    IUFillSwitch(&TrackModeS[TrackModeSP.nsp], name, label, isDefault ? ISS_ON : ISS_OFF);

    TrackModeSP.sp = TrackModeS;
    TrackModeSP.nsp++;

    return (TrackModeSP.nsp - 1);
}

bool Telescope::SetCurrentPark()
{
    DEBUG(Logger::DBG_WARNING, "Parking is not supported.");
    return false;
}

bool Telescope::SetDefaultPark()
{
    DEBUG(Logger::DBG_WARNING, "Parking is not supported.");
    return false;
}

/*
bool Telescope::processTimeInfo(const char *utc, const char *offset)
{
    struct ln_date utc_date;
    double utc_offset = 0;

    if (extractISOTime(utc, &utc_date) == -1)
    {
        TimeTP.s = IPS_ALERT;
        IDSetText(&TimeTP, "Date/Time is invalid: %s.", utc);
        return false;
    }

    utc_offset = atof(offset);

    if (updateTime(&utc_date, utc_offset))
    {
        IUSaveText(&TimeT[0], utc);
        IUSaveText(&TimeT[1], offset);
        TimeTP.s = IPS_OK;
        IDSetText(&TimeTP, nullptr);
        return true;
    }
    else
    {
        TimeTP.s = IPS_ALERT;
        IDSetText(&TimeTP, nullptr);
        return false;
    }
}
*/

bool Telescope::processLocationInfo(double latitude, double longitude, double elevation)
{
    // Do not update if not necessary
    if (latitude == LocationN[LOCATION_LATITUDE].value && longitude == LocationN[LOCATION_LONGITUDE].value &&
            elevation == LocationN[LOCATION_ELEVATION].value)
    {
        LocationNP.s = IPS_OK;
        IDSetNumber(&LocationNP, nullptr);
    }
    else if (latitude == 0 && longitude == 0)
    {
        LOG_DEBUG("Silently ignoring invalid latitude and longitude.");
        LocationNP.s = IPS_IDLE;
        IDSetNumber(&LocationNP, nullptr);
        return false;
    }

    if (updateLocation(latitude, longitude, elevation))
    {
        LocationNP.s                        = IPS_OK;
        LocationN[LOCATION_LATITUDE].value  = latitude;
        LocationN[LOCATION_LONGITUDE].value = longitude;
        LocationN[LOCATION_ELEVATION].value = elevation;
        //  Update client display
        IDSetNumber(&LocationNP, nullptr);

        // Always save geographic coord config immediately.
        saveConfig(true, "GEOGRAPHIC_COORD");

        updateObserverLocation(latitude, longitude, elevation);

        return true;
    }
    else
    {
        LocationNP.s = IPS_ALERT;
        //  Update client display
        IDSetNumber(&LocationNP, nullptr);

        return false;
    }
}

/*
bool Telescope::updateTime(ln_date *utc, double utc_offset)
{
    INDI_UNUSED(utc);
    INDI_UNUSED(utc_offset);
    return true;
}
*/

bool Telescope::updateLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(latitude);
    INDI_UNUSED(longitude);
    INDI_UNUSED(elevation);
    return true;
}

void Telescope::updateObserverLocation(double latitude, double longitude, double elevation)
{
    INDI_UNUSED(elevation);
    // JM: INDI Longitude is 0 to 360 increasing EAST. libnova East is Positive, West is negative
    lnobserver.lng = longitude;

    if (lnobserver.lng > 180)
        lnobserver.lng -= 360;
    lnobserver.lat = latitude;

    LOGF_INFO("Observer location updated: Longitude (%g) Latitude (%g)", lnobserver.lng, lnobserver.lat);
}

bool Telescope::SetParkPosition(double Axis1Value, double Axis2Value)
{
    INDI_UNUSED(Axis1Value);
    INDI_UNUSED(Axis2Value);
    return true;
}

void Telescope::SetTelescopeCapability(uint32_t cap, uint8_t slewRateCount)
{
    capability = cap;
    nSlewRate  = slewRateCount;

    // If both GOTO and SYNC are supported
    if (CanGOTO() && CanSync())
        IUFillSwitchVector(&CoordSP, CoordS, 3, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    // If ONLY GOTO is supported
    else if (CanGOTO())
        IUFillSwitchVector(&CoordSP, CoordS, 2, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    // If ONLY SYNC is supported
    else if (CanSync())
    {
        IUFillSwitch(&CoordS[0], "SYNC", "Sync", ISS_ON);
        IUFillSwitchVector(&CoordSP, CoordS, 1, getDeviceName(), "ON_COORD_SET", "On Set", MAIN_CONTROL_TAB, IP_RW,
                           ISR_1OFMANY, 60, IPS_IDLE);
    }

    if (nSlewRate >= 4)
    {
        free(SlewRateS);
        SlewRateS = static_cast<ISwitch *>(malloc(sizeof(ISwitch) * nSlewRate));
        //int step  = nSlewRate / 4;
        for (int i = 0; i < nSlewRate; i++)
        {
            char name[4];
            snprintf(name, 4, "%dx", i + 1);
            IUFillSwitch(SlewRateS + i, name, name, ISS_OFF);
        }

        //        strncpy((SlewRateS + (step * 0))->name, "SLEW_GUIDE", MAXINDINAME);
        //        strncpy((SlewRateS + (step * 1))->name, "SLEW_CENTERING", MAXINDINAME);
        //        strncpy((SlewRateS + (step * 2))->name, "SLEW_FIND", MAXINDINAME);
        //        strncpy((SlewRateS + (nSlewRate - 1))->name, "SLEW_MAX", MAXINDINAME);

        // If number of slew rate is EXACTLY 4, then let's use common labels
        if (nSlewRate == 4)
        {
            strncpy((SlewRateS + (0))->label, "Guide", MAXINDILABEL);
            strncpy((SlewRateS + (1))->label, "Centering", MAXINDILABEL);
            strncpy((SlewRateS + (2))->label, "Find", MAXINDILABEL);
            strncpy((SlewRateS + (3))->label, "Max", MAXINDILABEL);
        }

        // By Default we set current Slew Rate to 0.5 of max
        (SlewRateS + (nSlewRate / 2))->s = ISS_ON;

        IUFillSwitchVector(&SlewRateSP, SlewRateS, nSlewRate, getDeviceName(), "TELESCOPE_SLEW_RATE", "Slew Rate",
                           MOTION_TAB, IP_RW, ISR_1OFMANY, 0, IPS_IDLE);
    }
}

void Telescope::SetParkDataType(TelescopeParkData type)
{
    parkDataType = type;

    if (parkDataType != PARK_NONE)
    {
        switch (parkDataType)
        {
            case PARK_RA_DEC:
                IUFillNumber(&ParkPositionN[AXIS_RA], "PARK_RA", "RA (hh:mm:ss)", "%010.6m", 0, 24, 0, 0);
                IUFillNumber(&ParkPositionN[AXIS_DE], "PARK_DEC", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 0);
                break;

            case PARK_HA_DEC:
                IUFillNumber(&ParkPositionN[AXIS_RA], "PARK_HA", "HA (hh:mm:ss)", "%010.6m", -12, 12, 0, 0);
                IUFillNumber(&ParkPositionN[AXIS_DE], "PARK_DEC", "DEC (dd:mm:ss)", "%010.6m", -90, 90, 0, 0);
                break;

            case PARK_AZ_ALT:
                IUFillNumber(&ParkPositionN[AXIS_AZ], "PARK_AZ", "AZ D:M:S", "%10.6m", 0.0, 360.0, 0.0, 0);
                IUFillNumber(&ParkPositionN[AXIS_ALT], "PARK_ALT", "Alt  D:M:S", "%10.6m", -90., 90.0, 0.0, 0);
                break;

            case PARK_RA_DEC_ENCODER:
                IUFillNumber(&ParkPositionN[AXIS_RA], "PARK_RA", "RA Encoder", "%.0f", 0, 16777215, 1, 0);
                IUFillNumber(&ParkPositionN[AXIS_DE], "PARK_DEC", "DEC Encoder", "%.0f", 0, 16777215, 1, 0);
                break;

            case PARK_AZ_ALT_ENCODER:
                IUFillNumber(&ParkPositionN[AXIS_RA], "PARK_AZ", "AZ Encoder", "%.0f", 0, 16777215, 1, 0);
                IUFillNumber(&ParkPositionN[AXIS_DE], "PARK_ALT", "ALT Encoder", "%.0f", 0, 16777215, 1, 0);
                break;

            default:
                break;
        }

        IUFillNumberVector(&ParkPositionNP, ParkPositionN, 2, getDeviceName(), "TELESCOPE_PARK_POSITION",
                           "Park Position", SITE_TAB, IP_RW, 60, IPS_IDLE);
    }
}

void Telescope::SyncParkStatus(bool isparked)
{
    IsParked = isparked;
    IUResetSwitch(&ParkSP);

    if (IsParked)
    {
        ParkS[0].s = ISS_ON;
        TrackState = SCOPE_PARKED;
        ParkSP.s = IPS_OK;
        LOG_INFO("Mount is parked.");
    }
    else
    {
        ParkS[1].s = ISS_ON;
        TrackState = SCOPE_IDLE;
        ParkSP.s = IPS_IDLE;
        LOG_INFO("Mount is unparked.");
    }

    IDSetSwitch(&ParkSP, nullptr);
}

void Telescope::SetParked(bool isparked)
{
    SyncParkStatus(isparked);

    if (parkDataType != PARK_NONE)
        WriteParkData();
}

bool Telescope::isParked()
{
    return IsParked;
}

bool Telescope::InitPark()
{
    const char *loadres = LoadParkData();
    if (loadres)
    {
        LOGF_INFO("InitPark: No Park data in file %s: %s", ParkDataFileName.c_str(), loadres);
        SyncParkStatus(false);
        return false;
    }

    SyncParkStatus(isParked());

    LOGF_DEBUG("InitPark Axis1 %.2f Axis2 %.2f", Axis1ParkPosition, Axis2ParkPosition);
    ParkPositionN[AXIS_RA].value = Axis1ParkPosition;
    ParkPositionN[AXIS_DE].value = Axis2ParkPosition;
    IDSetNumber(&ParkPositionNP, nullptr);

    return true;
}

const char *Telescope::LoadParkXML()
{
    wordexp_t wexp;
    FILE *fp = nullptr;
    LilXML *lp = nullptr;
    static char errmsg[512];

    XMLEle *parkxml = nullptr;
    XMLAtt *ap = nullptr;
    bool devicefound = false;

    ParkDeviceName       = getDeviceName();
    ParkstatusXml        = nullptr;
    ParkdeviceXml        = nullptr;
    ParkpositionXml      = nullptr;
    ParkpositionAxis1Xml = nullptr;
    ParkpositionAxis2Xml = nullptr;

    if (wordexp(ParkDataFileName.c_str(), &wexp, 0))
    {
        wordfree(&wexp);
        return "Badly formed filename.";
    }

    if (!(fp = fopen(wexp.we_wordv[0], "r")))
    {
        wordfree(&wexp);
        return strerror(errno);
    }
    wordfree(&wexp);

    lp = newLilXML();

    if (ParkdataXmlRoot)
        delXMLEle(ParkdataXmlRoot);

    ParkdataXmlRoot = readXMLFile(fp, lp, errmsg);
    fclose(fp);

    delLilXML(lp);
    if (!ParkdataXmlRoot)
        return errmsg;

    parkxml = nextXMLEle(ParkdataXmlRoot, 1);

    if (!parkxml)
        return "Empty park file.";

    if (!strcmp(tagXMLEle(parkxml), "parkdata"))
    {
        delXMLEle(parkxml);
        return "Not a park data file";
    }

    while (parkxml)
    {
        if (strcmp(tagXMLEle(parkxml), "device"))
        {
            parkxml = nextXMLEle(ParkdataXmlRoot, 0);
            continue;
        }
        ap = findXMLAtt(parkxml, "name");
        if (ap && (!strcmp(valuXMLAtt(ap), ParkDeviceName)))
        {
            devicefound = true;
            break;
        }
        parkxml = nextXMLEle(ParkdataXmlRoot, 0);
    }

    if (!devicefound)
    {
        delXMLEle(parkxml);
        return "No park data found for this device";
    }

    ParkdeviceXml        = parkxml;
    ParkstatusXml        = findXMLEle(parkxml, "parkstatus");
    ParkpositionXml      = findXMLEle(parkxml, "parkposition");
    ParkpositionAxis1Xml = findXMLEle(ParkpositionXml, "axis1position");
    ParkpositionAxis2Xml = findXMLEle(ParkpositionXml, "axis2position");


    if (ParkstatusXml == nullptr || ParkpositionAxis1Xml == nullptr || ParkpositionAxis2Xml == nullptr)
    {
        return "Park data invalid or missing.";
    }

    return nullptr;
}

const char *Telescope::LoadParkData()
{
    IsParked = false;

    const char *result = LoadParkXML();
    if (result != nullptr)
        return result;

    if (!strcmp(pcdataXMLEle(ParkstatusXml), "true"))
        IsParked = true;

    double axis1Pos = std::numeric_limits<double>::quiet_NaN();
    double axis2Pos = std::numeric_limits<double>::quiet_NaN();

    int rc = sscanf(pcdataXMLEle(ParkpositionAxis1Xml), "%lf", &axis1Pos);
    if (rc != 1)
    {
        return "Unable to parse Park Position Axis 1.";
    }
    rc = sscanf(pcdataXMLEle(ParkpositionAxis2Xml), "%lf", &axis2Pos);
    if (rc != 1)
    {
        return "Unable to parse Park Position Axis 2.";
    }

    if (std::isnan(axis1Pos) == false && std::isnan(axis2Pos) == false)
    {
        Axis1ParkPosition = axis1Pos;
        Axis2ParkPosition = axis2Pos;
        return nullptr;
    }

    return "Failed to parse Park Position.";
}

bool Telescope::PurgeParkData()
{
    // We need to refresh parking data in case other devices parking states were updated since we
    // read the data the first time.
    if (LoadParkXML() != nullptr)
        LOG_DEBUG("Failed to refresh parking data.");

    wordexp_t wexp;
    FILE *fp = nullptr;
    LilXML *lp = nullptr;
    static char errmsg[512];

    XMLEle *parkxml = nullptr;
    XMLAtt *ap = nullptr;
    bool devicefound = false;

    ParkDeviceName = getDeviceName();

    if (wordexp(ParkDataFileName.c_str(), &wexp, 0))
    {
        wordfree(&wexp);
        return false;
    }

    if (!(fp = fopen(wexp.we_wordv[0], "r")))
    {
        wordfree(&wexp);
        LOGF_ERROR("Failed to purge park data: %s", strerror(errno));
        return false;
    }
    wordfree(&wexp);

    lp = newLilXML();

    if (ParkdataXmlRoot)
        delXMLEle(ParkdataXmlRoot);

    ParkdataXmlRoot = readXMLFile(fp, lp, errmsg);
    fclose(fp);

    delLilXML(lp);
    if (!ParkdataXmlRoot)
        return false;

    parkxml = nextXMLEle(ParkdataXmlRoot, 1);

    if (!parkxml)
        return false;

    if (!strcmp(tagXMLEle(parkxml), "parkdata"))
    {
        delXMLEle(parkxml);
        return false;
    }

    while (parkxml)
    {
        if (strcmp(tagXMLEle(parkxml), "device"))
        {
            parkxml = nextXMLEle(ParkdataXmlRoot, 0);
            continue;
        }
        ap = findXMLAtt(parkxml, "name");
        if (ap && (!strcmp(valuXMLAtt(ap), ParkDeviceName)))
        {
            devicefound = true;
            break;
        }
        parkxml = nextXMLEle(ParkdataXmlRoot, 0);
    }

    if (!devicefound)
        return false;

    delXMLEle(parkxml);

    ParkstatusXml        = nullptr;
    ParkdeviceXml        = nullptr;
    ParkpositionXml      = nullptr;
    ParkpositionAxis1Xml = nullptr;
    ParkpositionAxis2Xml = nullptr;

    wordexp(ParkDataFileName.c_str(), &wexp, 0);
    if (!(fp = fopen(wexp.we_wordv[0], "w")))
    {
        wordfree(&wexp);
        LOGF_INFO("WriteParkData: can not write file %s: %s", ParkDataFileName.c_str(), strerror(errno));
        return false;
    }
    prXMLEle(fp, ParkdataXmlRoot, 0);
    fclose(fp);
    wordfree(&wexp);

    return true;
}

bool Telescope::WriteParkData()
{
    // We need to refresh parking data in case other devices parking states were updated since we
    // read the data the first time.
    if (LoadParkXML() != nullptr)
        LOG_DEBUG("Failed to refresh parking data.");

    wordexp_t wexp;
    FILE *fp;
    char pcdata[30];
    ParkDeviceName = getDeviceName();

    if (wordexp(ParkDataFileName.c_str(), &wexp, 0))
    {
        wordfree(&wexp);
        LOGF_INFO("WriteParkData: can not write file %s: Badly formed filename.",
                  ParkDataFileName.c_str());
        return false;
    }

    if (!(fp = fopen(wexp.we_wordv[0], "w")))
    {
        wordfree(&wexp);
        LOGF_INFO("WriteParkData: can not write file %s: %s", ParkDataFileName.c_str(),
                  strerror(errno));
        return false;
    }

    if (!ParkdataXmlRoot)
        ParkdataXmlRoot = addXMLEle(nullptr, "parkdata");

    if (!ParkdeviceXml)
    {
        ParkdeviceXml = addXMLEle(ParkdataXmlRoot, "device");
        addXMLAtt(ParkdeviceXml, "name", ParkDeviceName);
    }

    if (!ParkstatusXml)
        ParkstatusXml = addXMLEle(ParkdeviceXml, "parkstatus");
    if (!ParkpositionXml)
        ParkpositionXml = addXMLEle(ParkdeviceXml, "parkposition");
    if (!ParkpositionAxis1Xml)
        ParkpositionAxis1Xml = addXMLEle(ParkpositionXml, "axis1position");
    if (!ParkpositionAxis2Xml)
        ParkpositionAxis2Xml = addXMLEle(ParkpositionXml, "axis2position");

    editXMLEle(ParkstatusXml, (IsParked ? "true" : "false"));

    snprintf(pcdata, sizeof(pcdata), "%lf", Axis1ParkPosition);
    editXMLEle(ParkpositionAxis1Xml, pcdata);
    snprintf(pcdata, sizeof(pcdata), "%lf", Axis2ParkPosition);
    editXMLEle(ParkpositionAxis2Xml, pcdata);

    prXMLEle(fp, ParkdataXmlRoot, 0);
    fclose(fp);
    wordfree(&wexp);

    return true;
}

double Telescope::GetAxis1Park() const
{
    return Axis1ParkPosition;
}
double Telescope::GetAxis1ParkDefault() const
{
    return Axis1DefaultParkPosition;
}
double Telescope::GetAxis2Park() const
{
    return Axis2ParkPosition;
}
double Telescope::GetAxis2ParkDefault() const
{
    return Axis2DefaultParkPosition;
}

void Telescope::SetAxis1Park(double value)
{
    LOGF_DEBUG("Setting Park Axis1 to %.2f", value);
    Axis1ParkPosition            = value;
    ParkPositionN[AXIS_RA].value = value;
    IDSetNumber(&ParkPositionNP, nullptr);
}

void Telescope::SetAxis1ParkDefault(double value)
{
    LOGF_DEBUG("Setting Default Park Axis1 to %.2f", value);
    Axis1DefaultParkPosition = value;
}

void Telescope::SetAxis2Park(double value)
{
    LOGF_DEBUG("Setting Park Axis2 to %.2f", value);
    Axis2ParkPosition            = value;
    ParkPositionN[AXIS_DE].value = value;
    IDSetNumber(&ParkPositionNP, nullptr);
}

void Telescope::SetAxis2ParkDefault(double value)
{
    LOGF_DEBUG("Setting Default Park Axis2 to %.2f", value);
    Axis2DefaultParkPosition = value;
}

bool Telescope::isLocked() const
{
    return (DomeClosedLockT[1].s == ISS_ON || DomeClosedLockT[3].s == ISS_ON) && IsLocked;
}

bool Telescope::SetSlewRate(int index)
{
    INDI_UNUSED(index);
    return true;
}

void Telescope::processButton(const char *button_n, ISState state)
{
    //ignore OFF
    if (state == ISS_OFF)
        return;

    if (!strcmp(button_n, "ABORTBUTTON"))
    {
        ISwitchVectorProperty *trackSW = getSwitch("TELESCOPE_TRACK_MODE");
        // Only abort if we have some sort of motion going on
        if (ParkSP.s == IPS_BUSY || MovementNSSP.s == IPS_BUSY || MovementWESP.s == IPS_BUSY || EqNP.s == IPS_BUSY ||
                (trackSW && trackSW->s == IPS_BUSY))
        {
            // Invoke parent processing so that Telescope takes care of abort cross-check
            ISState states[1] = { ISS_ON };
            char *names[1]    = { AbortS[0].name };
            ISNewSwitch(getDeviceName(), AbortSP.name, states, names, 1);
        }
    }
    else if (!strcmp(button_n, "PARKBUTTON"))
    {
        ISState states[2] = { ISS_ON, ISS_OFF };
        char *names[2]    = { ParkS[0].name, ParkS[1].name };
        ISNewSwitch(getDeviceName(), ParkSP.name, states, names, 2);
    }
    else if (!strcmp(button_n, "UNPARKBUTTON"))
    {
        ISState states[2] = { ISS_OFF, ISS_ON };
        char *names[2]    = { ParkS[0].name, ParkS[1].name };
        ISNewSwitch(getDeviceName(), ParkSP.name, states, names, 2);
    }
    else if (!strcmp(button_n, "SLEWPRESETUP"))
    {
        processSlewPresets(1, 270);
    }
    else if (!strcmp(button_n, "SLEWPRESETDOWN"))
    {
        processSlewPresets(1, 90);
    }
}

void Telescope::processJoystick(const char *joystick_n, double mag, double angle)
{
    if (MotionControlModeTP.sp[MOTION_CONTROL_JOYSTICK].s == ISS_ON && !strcmp(joystick_n, "MOTIONDIR"))
    {
        if ((TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
        {
            DEBUG(Logger::DBG_WARNING, "Can not slew while mount is parking/parked.");
            return;
        }

        processNSWE(mag, angle);
    }
    else if (!strcmp(joystick_n, "SLEWPRESET"))
        processSlewPresets(mag, angle);
}

void Telescope::processAxis(const char *axis_n, double value)
{
    if (MotionControlModeTP.sp[MOTION_CONTROL_AXES].s == ISS_ON)
    {
        if (!strcmp(axis_n, "MOTIONDIRNS") || !strcmp(axis_n, "MOTIONDIRWE"))
        {
            if ((TrackState == SCOPE_PARKING) || (TrackState == SCOPE_PARKED))
            {
                DEBUG(Logger::DBG_WARNING, "Cannot slew while mount is parking/parked.");
                return;
            }

            if (!strcmp(axis_n, "MOTIONDIRNS"))
            {
                // South
                if (value > 0)
                {
                    motionDirNSValue = -1;
                }
                // North
                else if (value < 0)
                {
                    motionDirNSValue = 1;
                }
                else
                {
                    motionDirNSValue = 0;
                }
            }
            else if (!strcmp(axis_n, "MOTIONDIRWE"))
            {
                // East
                if (value > 0)
                {
                    motionDirWEValue = 1;
                }
                // West
                else if (value < 0)
                {
                    motionDirWEValue = -1;
                }
                else
                {
                    motionDirWEValue = 0;
                }
            }

            float x = motionDirWEValue * sqrt(1 - pow(motionDirNSValue, 2) / 2.0f);
            float y = motionDirNSValue * sqrt(1 - pow(motionDirWEValue, 2) / 2.0f);
            float angle = atan2(y, x) * (180.0 / 3.141592653589);
            float mag = sqrt(pow(y, 2) + pow(x, 2));
            while (angle < 0)
            {
                angle += 360;
            }
            if (mag == 0)
            {
                angle = 0;
            }

            processNSWE(mag, angle);
        }
    }
}

void Telescope::processNSWE(double mag, double angle)
{
    if (mag < 0.5)
    {
        // Moving in the same direction will make it stop
        if (MovementNSSP.s == IPS_BUSY)
        {
            if (MoveNS(MovementNSSP.sp[0].s == ISS_ON ? DIRECTION_NORTH : DIRECTION_SOUTH, MOTION_STOP))
            {
                IUResetSwitch(&MovementNSSP);
                MovementNSSP.s = IPS_IDLE;
                IDSetSwitch(&MovementNSSP, nullptr);
            }
            else
            {
                MovementNSSP.s = IPS_ALERT;
                IDSetSwitch(&MovementNSSP, nullptr);
            }
        }

        if (MovementWESP.s == IPS_BUSY)
        {
            if (MoveWE(MovementWESP.sp[0].s == ISS_ON ? DIRECTION_WEST : DIRECTION_EAST, MOTION_STOP))
            {
                IUResetSwitch(&MovementWESP);
                MovementWESP.s = IPS_IDLE;
                IDSetSwitch(&MovementWESP, nullptr);
            }
            else
            {
                MovementWESP.s = IPS_ALERT;
                IDSetSwitch(&MovementWESP, nullptr);
            }
        }
    }
    // Put high threshold
    else if (mag > 0.9)
    {
        // Only one axis can move at a time
        if (LockAxisS[AXIS_RA].s == ISS_ON)
        {
            // West
            if (angle >= 90 && angle <= 270)
                angle = 180;
            // East
            else
                angle = 0;
        }
        else if (LockAxisS[AXIS_DE].s == ISS_ON)
        {
            // North
            if (angle >= 0 && angle <= 180)
                angle = 90;
            // South
            else
                angle = 270;
        }

        // Snap angle to x or y direction if close to corresponding axis (i.e. deviation < 15°)
        if (angle > 75 && angle < 105)
        {
            angle = 90;
        }
        if (angle > 165 && angle < 195)
        {
            angle = 180;
        }
        if (angle > 255 && angle < 285)
        {
            angle = 270;
        }
        if (angle > 345 || angle < 15)
        {
            angle = 0;
        }

        // North
        if (angle > 0 && angle < 180)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementNSSP.s != IPS_BUSY || MovementNSS[0].s != ISS_ON)
                MoveNS(DIRECTION_NORTH, MOTION_START);

            MovementNSSP.s                     = IPS_BUSY;
            MovementNSSP.sp[DIRECTION_NORTH].s = ISS_ON;
            MovementNSSP.sp[DIRECTION_SOUTH].s = ISS_OFF;
            IDSetSwitch(&MovementNSSP, nullptr);
        }
        // South
        if (angle > 180 && angle < 360)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementNSSP.s != IPS_BUSY || MovementNSS[1].s != ISS_ON)
                MoveNS(DIRECTION_SOUTH, MOTION_START);

            MovementNSSP.s                     = IPS_BUSY;
            MovementNSSP.sp[DIRECTION_NORTH].s = ISS_OFF;
            MovementNSSP.sp[DIRECTION_SOUTH].s = ISS_ON;
            IDSetSwitch(&MovementNSSP, nullptr);
        }
        // East
        if (angle < 90 || angle > 270)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementWESP.s != IPS_BUSY || MovementWES[1].s != ISS_ON)
                MoveWE(DIRECTION_EAST, MOTION_START);

            MovementWESP.s                    = IPS_BUSY;
            MovementWESP.sp[DIRECTION_WEST].s = ISS_OFF;
            MovementWESP.sp[DIRECTION_EAST].s = ISS_ON;
            IDSetSwitch(&MovementWESP, nullptr);
        }

        // West
        if (angle > 90 && angle < 270)
        {
            // Don't try to move if you're busy and moving in the same direction
            if (MovementWESP.s != IPS_BUSY || MovementWES[0].s != ISS_ON)
                MoveWE(DIRECTION_WEST, MOTION_START);

            MovementWESP.s                    = IPS_BUSY;
            MovementWESP.sp[DIRECTION_WEST].s = ISS_ON;
            MovementWESP.sp[DIRECTION_EAST].s = ISS_OFF;
            IDSetSwitch(&MovementWESP, nullptr);
        }
    }
}

void Telescope::processSlewPresets(double mag, double angle)
{
    // high threshold, only 1 is accepted
    if (mag != 1)
        return;

    int currentIndex = IUFindOnSwitchIndex(&SlewRateSP);

    // Up
    if (angle > 0 && angle < 180)
    {
        if (currentIndex <= 0)
            return;

        IUResetSwitch(&SlewRateSP);
        SlewRateS[currentIndex - 1].s = ISS_ON;
        SetSlewRate(currentIndex - 1);
    }
    // Down
    else
    {
        if (currentIndex >= SlewRateSP.nsp - 1)
            return;

        IUResetSwitch(&SlewRateSP);
        SlewRateS[currentIndex + 1].s = ISS_ON;
        SetSlewRate(currentIndex - 1);
    }

    IDSetSwitch(&SlewRateSP, nullptr);
}

void Telescope::joystickHelper(const char *joystick_n, double mag, double angle, void *context)
{
    static_cast<Telescope *>(context)->processJoystick(joystick_n, mag, angle);
}

void Telescope::axisHelper(const char *axis_n, double value, void *context)
{
    static_cast<Telescope *>(context)->processAxis(axis_n, value);
}

void Telescope::buttonHelper(const char *button_n, ISState state, void *context)
{
    static_cast<Telescope *>(context)->processButton(button_n, state);
}

void Telescope::setPierSide(TelescopePierSide side)
{
    // ensure that the scope knows it's pier side or the pier side is simulated
    if (HasPierSide() == false && getSimulatePierSide() == false)
        return;

    currentPierSide = side;

    if (currentPierSide != lastPierSide)
    {
        PierSideS[PIER_WEST].s = (side == PIER_WEST) ? ISS_ON : ISS_OFF;
        PierSideS[PIER_EAST].s = (side == PIER_EAST) ? ISS_ON : ISS_OFF;
        PierSideSP.s           = IPS_OK;
        IDSetSwitch(&PierSideSP, nullptr);

        lastPierSide = currentPierSide;
    }
}

/// Simulates pier side using the hour angle.
/// A correct implementation uses the declination axis value, this is only for where this isn't available,
/// such as in the telescope simulator or a GEM which does not provide any pier side or axis information.
/// This last is deeply unsatisfactory, it will not be able to reflect the true pointing state
/// reliably for positions close to the meridian.
Telescope::TelescopePierSide Telescope::expectedPierSide(double ra)
{
    // return unknown if the mount does not have pier side, this will be the case for a fork mount
    // where a pier flip is not required.
    if (!HasPierSide() && !HasPierSideSimulation())
        return INDI::Telescope::PIER_UNKNOWN;

    // calculate the hour angle and derive the pier side
    double lst = get_local_sidereal_time(lnobserver.lng);
    double hourAngle = get_local_hour_angle(lst, ra);

    return hourAngle <= 0 ? INDI::Telescope::PIER_WEST : INDI::Telescope::PIER_EAST;
}

void Telescope::setPECState(TelescopePECState state)
{
    currentPECState = state;

    if (currentPECState != lastPECState)
    {

        PECStateS[PEC_OFF].s = (state == PEC_ON) ? ISS_OFF : ISS_ON;
        PECStateS[PEC_ON].s  = (state == PEC_ON) ? ISS_ON  : ISS_OFF;
        PECStateSP.s         = IPS_OK;
        IDSetSwitch(&PECStateSP, nullptr);

        lastPECState = currentPECState;
    }
}

bool Telescope::LoadScopeConfig()
{
    if (!CheckFile(ScopeConfigFileName, false))
    {
        LOGF_INFO("Can't open XML file (%s) for read", ScopeConfigFileName.c_str());
        return false;
    }
    LilXML *XmlHandle      = newLilXML();
    FILE *FilePtr          = fopen(ScopeConfigFileName.c_str(), "r");
    XMLEle *RootXmlNode    = nullptr;
    XMLEle *CurrentXmlNode = nullptr;
    XMLAtt *Ap             = nullptr;
    bool DeviceFound       = false;
    char ErrMsg[512];

    RootXmlNode = readXMLFile(FilePtr, XmlHandle, ErrMsg);
    fclose(FilePtr);
    delLilXML(XmlHandle);
    XmlHandle = nullptr;
    if (!RootXmlNode)
    {
        LOGF_INFO("Failed to parse XML file (%s): %s", ScopeConfigFileName.c_str(), ErrMsg);
        return false;
    }
    if (std::string(tagXMLEle(RootXmlNode)) != ScopeConfigRootXmlNode)
    {
        LOGF_INFO("Not a scope config XML file (%s)", ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    CurrentXmlNode = nextXMLEle(RootXmlNode, 1);
    // Find the current telescope in the config file
    while (CurrentXmlNode)
    {
        if (std::string(tagXMLEle(CurrentXmlNode)) != ScopeConfigDeviceXmlNode)
        {
            CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
            continue;
        }
        Ap = findXMLAtt(CurrentXmlNode, ScopeConfigNameXmlNode.c_str());
        if (Ap && !strcmp(valuXMLAtt(Ap), getDeviceName()))
        {
            DeviceFound = true;
            break;
        }
        CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
    }
    if (!DeviceFound)
    {
        LOGF_INFO("No a scope config found for %s in the XML file (%s)", getDeviceName(),
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    // Read the values
    XMLEle *XmlNode       = nullptr;
    const int ConfigIndex = GetScopeConfigIndex();
    double ScopeFoc = 0, ScopeAp = 0;
    double GScopeFoc = 0, GScopeAp = 0;
    std::string ConfigName;

    CurrentXmlNode = findXMLEle(CurrentXmlNode, ("config" + std::to_string(ConfigIndex)).c_str());
    if (!CurrentXmlNode)
    {
        DEBUGF(Logger::DBG_SESSION,
               "Config %d is not found in the XML file (%s). To save a new config, update and set scope properties and "
               "config name.",
               ConfigIndex, ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigScopeFocXmlNode.c_str());
    if (!XmlNode || sscanf(pcdataXMLEle(XmlNode), "%lf", &ScopeFoc) != 1)
    {
        LOGF_INFO("Can't read the telescope focal length from the XML file (%s)",
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigScopeApXmlNode.c_str());
    if (!XmlNode || sscanf(pcdataXMLEle(XmlNode), "%lf", &ScopeAp) != 1)
    {
        LOGF_INFO("Can't read the telescope aperture from the XML file (%s)",
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigGScopeFocXmlNode.c_str());
    if (!XmlNode || sscanf(pcdataXMLEle(XmlNode), "%lf", &GScopeFoc) != 1)
    {
        LOGF_INFO("Can't read the guide scope focal length from the XML file (%s)",
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigGScopeApXmlNode.c_str());
    if (!XmlNode || sscanf(pcdataXMLEle(XmlNode), "%lf", &GScopeAp) != 1)
    {
        LOGF_INFO("Can't read the guide scope aperture from the XML file (%s)",
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigLabelApXmlNode.c_str());
    if (!XmlNode)
    {
        LOGF_INFO("Can't read the telescope config name from the XML file (%s)",
                  ScopeConfigFileName.c_str());
        delXMLEle(RootXmlNode);
        return false;
    }
    ConfigName = pcdataXMLEle(XmlNode);
    // Store the loaded values
    if (IUFindNumber(&ScopeParametersNP, "TELESCOPE_FOCAL_LENGTH"))
    {
        IUFindNumber(&ScopeParametersNP, "TELESCOPE_FOCAL_LENGTH")->value = ScopeFoc;
    }
    if (IUFindNumber(&ScopeParametersNP, "TELESCOPE_APERTURE"))
    {
        IUFindNumber(&ScopeParametersNP, "TELESCOPE_APERTURE")->value = ScopeAp;
    }
    if (IUFindNumber(&ScopeParametersNP, "GUIDER_FOCAL_LENGTH"))
    {
        IUFindNumber(&ScopeParametersNP, "GUIDER_FOCAL_LENGTH")->value = GScopeFoc;
    }
    if (IUFindNumber(&ScopeParametersNP, "GUIDER_APERTURE"))
    {
        IUFindNumber(&ScopeParametersNP, "GUIDER_APERTURE")->value = GScopeAp;
    }
    if (IUFindText(&ScopeConfigNameTP, "SCOPE_CONFIG_NAME"))
    {
        IUSaveText(IUFindText(&ScopeConfigNameTP, "SCOPE_CONFIG_NAME"), ConfigName.c_str());
    }
    ScopeParametersNP.s = IPS_OK;
    IDSetNumber(&ScopeParametersNP, nullptr);
    ScopeConfigNameTP.s = IPS_OK;
    IDSetText(&ScopeConfigNameTP, nullptr);
    delXMLEle(RootXmlNode);
    return true;
}

bool Telescope::HasDefaultScopeConfig()
{
    if (!CheckFile(ScopeConfigFileName, false))
    {
        return false;
    }
    LilXML *XmlHandle      = newLilXML();
    FILE *FilePtr          = fopen(ScopeConfigFileName.c_str(), "r");
    XMLEle *RootXmlNode    = nullptr;
    XMLEle *CurrentXmlNode = nullptr;
    XMLAtt *Ap             = nullptr;
    bool DeviceFound       = false;
    char ErrMsg[512];

    RootXmlNode = readXMLFile(FilePtr, XmlHandle, ErrMsg);
    fclose(FilePtr);
    delLilXML(XmlHandle);
    XmlHandle = nullptr;
    if (!RootXmlNode)
    {
        return false;
    }
    if (std::string(tagXMLEle(RootXmlNode)) != ScopeConfigRootXmlNode)
    {
        delXMLEle(RootXmlNode);
        return false;
    }
    CurrentXmlNode = nextXMLEle(RootXmlNode, 1);
    // Find the current telescope in the config file
    while (CurrentXmlNode)
    {
        if (std::string(tagXMLEle(CurrentXmlNode)) != ScopeConfigDeviceXmlNode)
        {
            CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
            continue;
        }
        Ap = findXMLAtt(CurrentXmlNode, ScopeConfigNameXmlNode.c_str());
        if (Ap && !strcmp(valuXMLAtt(Ap), getDeviceName()))
        {
            DeviceFound = true;
            break;
        }
        CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
    }
    if (!DeviceFound)
    {
        delXMLEle(RootXmlNode);
        return false;
    }
    // Check the existence of Config #1 node
    CurrentXmlNode = findXMLEle(CurrentXmlNode, "config1");
    if (!CurrentXmlNode)
    {
        delXMLEle(RootXmlNode);
        return false;
    }
    return true;
}

bool Telescope::UpdateScopeConfig()
{
    // Get the config values from the UI
    const int ConfigIndex = GetScopeConfigIndex();
    double ScopeFoc = 0, ScopeAp = 0;
    double GScopeFoc = 0, GScopeAp = 0;
    std::string ConfigName;

    if (IUFindNumber(&ScopeParametersNP, "TELESCOPE_FOCAL_LENGTH"))
    {
        ScopeFoc = IUFindNumber(&ScopeParametersNP, "TELESCOPE_FOCAL_LENGTH")->value;
    }
    if (IUFindNumber(&ScopeParametersNP, "TELESCOPE_APERTURE"))
    {
        ScopeAp = IUFindNumber(&ScopeParametersNP, "TELESCOPE_APERTURE")->value;
    }
    if (IUFindNumber(&ScopeParametersNP, "GUIDER_FOCAL_LENGTH"))
    {
        GScopeFoc = IUFindNumber(&ScopeParametersNP, "GUIDER_FOCAL_LENGTH")->value;
    }
    if (IUFindNumber(&ScopeParametersNP, "GUIDER_APERTURE"))
    {
        GScopeAp = IUFindNumber(&ScopeParametersNP, "GUIDER_APERTURE")->value;
    }
    if (IUFindText(&ScopeConfigNameTP, "SCOPE_CONFIG_NAME") &&
            IUFindText(&ScopeConfigNameTP, "SCOPE_CONFIG_NAME")->text)
    {
        ConfigName = IUFindText(&ScopeConfigNameTP, "SCOPE_CONFIG_NAME")->text;
    }
    // Save the values to the actual XML file
    if (!CheckFile(ScopeConfigFileName, true))
    {
        LOGF_INFO("Can't open XML file (%s) for write", ScopeConfigFileName.c_str());
        return false;
    }
    // Open the existing XML file for write
    LilXML *XmlHandle   = newLilXML();
    FILE *FilePtr       = fopen(ScopeConfigFileName.c_str(), "r");
    XMLEle *RootXmlNode = nullptr;
    XMLAtt *Ap          = nullptr;
    bool DeviceFound    = false;
    char ErrMsg[512];

    RootXmlNode = readXMLFile(FilePtr, XmlHandle, ErrMsg);
    delLilXML(XmlHandle);
    XmlHandle = nullptr;
    fclose(FilePtr);

    XMLEle *CurrentXmlNode = nullptr;
    XMLEle *XmlNode        = nullptr;

    if (!RootXmlNode || std::string(tagXMLEle(RootXmlNode)) != ScopeConfigRootXmlNode)
    {
        RootXmlNode = addXMLEle(nullptr, ScopeConfigRootXmlNode.c_str());
    }
    CurrentXmlNode = nextXMLEle(RootXmlNode, 1);
    // Find the current telescope in the config file
    while (CurrentXmlNode)
    {
        if (std::string(tagXMLEle(CurrentXmlNode)) != ScopeConfigDeviceXmlNode)
        {
            CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
            continue;
        }
        Ap = findXMLAtt(CurrentXmlNode, ScopeConfigNameXmlNode.c_str());
        if (Ap && !strcmp(valuXMLAtt(Ap), getDeviceName()))
        {
            DeviceFound = true;
            break;
        }
        CurrentXmlNode = nextXMLEle(RootXmlNode, 0);
    }
    if (!DeviceFound)
    {
        CurrentXmlNode = addXMLEle(RootXmlNode, ScopeConfigDeviceXmlNode.c_str());
        addXMLAtt(CurrentXmlNode, ScopeConfigNameXmlNode.c_str(), getDeviceName());
    }
    // Add or update the config node
    XmlNode = findXMLEle(CurrentXmlNode, ("config" + std::to_string(ConfigIndex)).c_str());
    if (!XmlNode)
    {
        CurrentXmlNode = addXMLEle(CurrentXmlNode, ("config" + std::to_string(ConfigIndex)).c_str());
    }
    else
    {
        CurrentXmlNode = XmlNode;
    }
    // Add or update the telescope focal length
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigScopeFocXmlNode.c_str());
    if (!XmlNode)
    {
        XmlNode = addXMLEle(CurrentXmlNode, ScopeConfigScopeFocXmlNode.c_str());
    }
    editXMLEle(XmlNode, std::to_string(ScopeFoc).c_str());
    // Add or update the telescope focal aperture
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigScopeApXmlNode.c_str());
    if (!XmlNode)
    {
        XmlNode = addXMLEle(CurrentXmlNode, ScopeConfigScopeApXmlNode.c_str());
    }
    editXMLEle(XmlNode, std::to_string(ScopeAp).c_str());
    // Add or update the guide scope focal length
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigGScopeFocXmlNode.c_str());
    if (!XmlNode)
    {
        XmlNode = addXMLEle(CurrentXmlNode, ScopeConfigGScopeFocXmlNode.c_str());
    }
    editXMLEle(XmlNode, std::to_string(GScopeFoc).c_str());
    // Add or update the guide scope focal aperture
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigGScopeApXmlNode.c_str());
    if (!XmlNode)
    {
        XmlNode = addXMLEle(CurrentXmlNode, ScopeConfigGScopeApXmlNode.c_str());
    }
    editXMLEle(XmlNode, std::to_string(GScopeAp).c_str());
    // Add or update the config name
    XmlNode = findXMLEle(CurrentXmlNode, ScopeConfigLabelApXmlNode.c_str());
    if (!XmlNode)
    {
        XmlNode = addXMLEle(CurrentXmlNode, ScopeConfigLabelApXmlNode.c_str());
    }
    editXMLEle(XmlNode, ConfigName.c_str());
    // Save the final content
    FilePtr = fopen(ScopeConfigFileName.c_str(), "w");
    prXMLEle(FilePtr, RootXmlNode, 0);
    fclose(FilePtr);
    delXMLEle(RootXmlNode);
    return true;
}

std::string Telescope::GetHomeDirectory() const
{
    // Check first the HOME environmental variable
    const char *HomeDir = getenv("HOME");

    // ...otherwise get the home directory of the current user.
    if (!HomeDir)
    {
        HomeDir = getpwuid(getuid())->pw_dir;
    }
    return (HomeDir ? std::string(HomeDir) : "");
}

int Telescope::GetScopeConfigIndex() const
{
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG1") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG1")->s == ISS_ON)
    {
        return 1;
    }
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG2") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG2")->s == ISS_ON)
    {
        return 2;
    }
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG3") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG3")->s == ISS_ON)
    {
        return 3;
    }
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG4") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG4")->s == ISS_ON)
    {
        return 4;
    }
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG5") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG5")->s == ISS_ON)
    {
        return 5;
    }
    if (IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG6") && IUFindSwitch(&ScopeConfigsSP, "SCOPE_CONFIG6")->s == ISS_ON)
    {
        return 6;
    }
    return 0;
}

bool Telescope::CheckFile(const std::string &file_name, bool writable) const
{
    FILE *FilePtr = fopen(file_name.c_str(), (writable ? "a" : "r"));

    if (FilePtr)
    {
        fclose(FilePtr);
        return true;
    }
    return false;
}

void Telescope::sendTimeFromSystem()
{
    char ts[32] = {0};

    std::time_t t = std::time(nullptr);
    struct std::tm *utctimeinfo = std::gmtime(&t);

    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%S", utctimeinfo);
    IUSaveText(&TimeT[0], ts);

    struct std::tm *localtimeinfo = std::localtime(&t);
    snprintf(ts, sizeof(ts), "%4.2f", (localtimeinfo->tm_gmtoff / 3600.0));
    IUSaveText(&TimeT[1], ts);

    TimeTP.s = IPS_OK;

    IDSetText(&TimeTP, nullptr);
}

bool Telescope::getSimulatePierSide() const
{
    return m_simulatePierSide;
}

const char * Telescope::getPierSideStr(TelescopePierSide ps)
{
    switch (ps)
    {
        case PIER_WEST:
            return "PIER_WEST";
        case PIER_EAST:
            return "PIER_EAST";
        default:
            return "PIER_UNKNOWN";
    }
}

void Telescope::setSimulatePierSide(bool simulate)
{
    IUResetSwitch(&SimulatePierSideSP);
    SimulatePierSideS[0].s = simulate ? ISS_ON : ISS_OFF;
    SimulatePierSideS[1].s = simulate ? ISS_OFF : ISS_ON;
    SimulatePierSideSP.s = IPS_OK;
    IDSetSwitch(&SimulatePierSideSP, nullptr);

    if (simulate)
    {
        capability |= TELESCOPE_HAS_PIER_SIDE;
        defineSwitch(&PierSideSP);
    }
    else
    {
        capability &= static_cast<uint32_t>(~TELESCOPE_HAS_PIER_SIDE);
        deleteProperty(PierSideSP.name);
    }

    m_simulatePierSide = simulate;
}

/*
double Telescope::getAzimuth(double r, double d)
{
    ln_equ_posn lnradec { 0, 0 };
    ln_hrz_posn altaz { 0, 0 };

    lnradec.ra  = (r * 360) / 24.0;
    lnradec.dec = d;

    ln_get_hrz_from_equ(&lnradec, &lnobserver, ln_get_julian_from_sys(), &altaz);

    /* libnova measures azimuth from south towards west */
    return (range360(altaz.az + 180));
}
*/

}
