/*******************************************************************************
 Copyright(c) 2017 Jasem Mutlaq. All rights reserved.

 It just gets the encoder positoin and outputs current coordinates.
 Calibratoin and syncing not supported yet.

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

#include "dsc.h"

#include "indicom.h"

#include <cstring>
#include <memory>
#include <regex>
#include <termios.h>
#include <unistd.h>

#define DSC_TIMEOUT 2
#define AXIS_TAB    "Axis Settings"

#include <alignment/DriverCommon.h> // For DBG_ALIGNMENT
using namespace INDI::AlignmentSubsystem;

// We declare an auto pointer to DSC.
std::unique_ptr<DSC> dsc(new DSC());

void ISGetProperties(const char *dev)
{
    dsc->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    dsc->ISNewSwitch(dev, name, states, names, n);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    dsc->ISNewText(dev, name, texts, names, n);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    dsc->ISNewNumber(dev, name, values, names, n);
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
    dsc->ISSnoopDevice(root);
}

DSC::DSC()
{
    SetTelescopeCapability(TELESCOPE_CAN_SYNC | TELESCOPE_HAS_LOCATION, 0);
}

const char *DSC::getDefaultName()
{
    return (const char *)"Digital Setting Circle";
}

bool DSC::initProperties()
{
    INDI::Telescope::initProperties();

    // Raw encoder values
    IUFillNumber(&EncoderN[AXIS1_ENCODER], "AXIS1_ENCODER", "Axis 1", "%0.f", 0, 1e6, 0, 0);
    IUFillNumber(&EncoderN[AXIS2_ENCODER], "AXIS2_ENCODER", "Axis 2", "%0.f", 0, 1e6, 0, 0);
    IUFillNumber(&EncoderN[AXIS1_RAW_ENCODER], "AXIS1_RAW_ENCODER", "RAW Axis 1", "%0.f", -1e6, 1e6, 0, 0);
    IUFillNumber(&EncoderN[AXIS2_RAW_ENCODER], "AXIS2_RAW_ENCODER", "RAW Axis 2", "%0.f", -1e6, 1e6, 0, 0);
    IUFillNumberVector(&EncoderNP, EncoderN, 4, getDeviceName(), "DCS_ENCODER", "Encoders", MAIN_CONTROL_TAB, IP_RO, 0,
                       IPS_IDLE);

    // Encoder Settings
    IUFillNumber(&AxisSettingsN[AXIS1_TICKS], "AXIS1_TICKS", "#1 ticks/rev", "%g", 256, 1e6, 0, 4096);
    IUFillNumber(&AxisSettingsN[AXIS1_DEGREE_OFFSET], "AXIS1_DEGREE_OFFSET", "#1 Degrees Offset", "%g", -180, 180, 30,
                 0);
    IUFillNumber(&AxisSettingsN[AXIS2_TICKS], "AXIS2_TICKS", "#2 ticks/rev", "%g", 256, 1e6, 0, 4096);
    IUFillNumber(&AxisSettingsN[AXIS2_DEGREE_OFFSET], "AXIS2_DEGREE_OFFSET", "#2 Degrees Offset", "%g", -180, 180, 30,
                 0);
    IUFillNumberVector(&AxisSettingsNP, AxisSettingsN, 4, getDeviceName(), "AXIS_SETTINGS", "Axis Resolution", AXIS_TAB,
                       IP_RW, 0, IPS_IDLE);

    // Axis Range
    IUFillSwitch(&AxisRangeS[AXIS_FULL_STEP], "AXIS_FULL_STEP", "Full Step", ISS_ON);
    IUFillSwitch(&AxisRangeS[AXIS_HALF_STEP], "AXIS_HALF_STEP", "Half Step", ISS_OFF);
    IUFillSwitchVector(&AxisRangeSP, AxisRangeS, 2, getDeviceName(), "AXIS_RANGE", "Axis Range", AXIS_TAB, IP_RW,
                       ISR_1OFMANY, 0, IPS_IDLE);

    // Reverse Encoder Direction
    IUFillSwitch(&ReverseS[AXIS1_ENCODER], "AXIS1_REVERSE", "Axis 1", ISS_OFF);
    IUFillSwitch(&ReverseS[AXIS2_ENCODER], "AXIS2_REVERSE", "Axis 2", ISS_OFF);
    IUFillSwitchVector(&ReverseSP, ReverseS, 2, getDeviceName(), "AXIS_REVERSE", "Reverse", AXIS_TAB, IP_RW,
                       ISR_NOFMANY, 0, IPS_IDLE);

// Offsets applied to raw encoder values to adjust them as necessary
#if 0
    IUFillNumber(&EncoderOffsetN[OFFSET_AXIS1_SCALE], "OFFSET_AXIS1_SCALE", "#1 Ticks Scale", "%g", 0, 1e6, 0, 1);
    IUFillNumber(&EncoderOffsetN[OFFSET_AXIS1_OFFSET], "OFFSET_AXIS1_OFFSET", "#1 Ticks Offset", "%g", -1e6, 1e6, 0, 0);
    IUFillNumber(&EncoderOffsetN[AXIS1_DEGREE_OFFSET], "AXIS1_DEGREE_OFFSET", "#1 Degrees Offset", "%g", -180, 180, 30, 0);
    IUFillNumber(&EncoderOffsetN[OFFSET_AXIS2_SCALE], "OFFSET_AIXS2_SCALE", "#2 Ticks Scale", "%g", 0, 1e6, 0, 1);
    IUFillNumber(&EncoderOffsetN[OFFSET_AXIS2_OFFSET], "OFFSET_AXIS2_OFFSET", "#2 Ticks Offset", "%g", -1e6, 1e6, 0, 0);
    IUFillNumber(&EncoderOffsetN[AXIS2_DEGREE_OFFSET], "AXIS2_DEGREE_OFFSET", "#2 Degrees Offset", "%g", -180, 180, 30, 0);
    IUFillNumberVector(&EncoderOffsetNP, EncoderOffsetN, 6, getDeviceName(), "AXIS_OFFSET", "Offsets", AXIS_TAB, IP_RW, 0, IPS_IDLE);
#endif

    // Mount Type
    IUFillSwitch(&MountTypeS[MOUNT_EQUATORIAL], "MOUNT_EQUATORIAL", "Equatorial", ISS_ON);
    IUFillSwitch(&MountTypeS[MOUNT_ALTAZ], "MOUNT_ALTAZ", "AltAz", ISS_OFF);
    IUFillSwitchVector(&MountTypeSP, MountTypeS, 2, getDeviceName(), "MOUNT_TYPE", "Mount Type", MAIN_CONTROL_TAB,
                       IP_RW, ISR_1OFMANY, 0, IPS_IDLE);

    // Simulation encoder values
    IUFillNumber(&SimEncoderN[AXIS1_ENCODER], "AXIS1_ENCODER", "Axis 1", "%0.f", -1e6, 1e6, 0, 0);
    IUFillNumber(&SimEncoderN[AXIS2_ENCODER], "AXIS2_ENCODER", "Axis 2", "%0.f", -1e6, 1e6, 0, 0);
    IUFillNumberVector(&SimEncoderNP, SimEncoderN, 2, getDeviceName(), "SIM_ENCODER", "Sim Encoders", MAIN_CONTROL_TAB,
                       IP_RW, 0, IPS_IDLE);

    addAuxControls();

    InitAlignmentProperties(this);

    return true;
}

bool DSC::updateProperties()
{
    INDI::Telescope::updateProperties();

    if (isConnected())
    {
        defineNumber(&EncoderNP);
        defineNumber(&AxisSettingsNP);
        defineSwitch(&AxisRangeSP);
        defineSwitch(&ReverseSP);
        //defineNumber(&EncoderOffsetNP);
        defineSwitch(&MountTypeSP);

        if (isSimulation())
            defineNumber(&SimEncoderNP);

        SetAlignmentSubsystemActive(true);
    }
    else
    {
        deleteProperty(EncoderNP.name);
        deleteProperty(AxisSettingsNP.name);
        deleteProperty(AxisRangeSP.name);
        deleteProperty(ReverseSP.name);
        //deleteProperty(EncoderOffsetNP.name);
        deleteProperty(MountTypeSP.name);

        if (isSimulation())
            deleteProperty(SimEncoderNP.name);
    }

    return true;
}

bool DSC::saveConfigItems(FILE *fp)
{
    INDI::Telescope::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &AxisSettingsNP);
    //IUSaveConfigNumber(fp, &EncoderOffsetNP);
    IUSaveConfigSwitch(fp, &AxisRangeSP);
    IUSaveConfigSwitch(fp, &ReverseSP);
    IUSaveConfigSwitch(fp, &MountTypeSP);

    return true;
}

bool DSC::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    ProcessAlignmentTextProperties(this, name, texts, names, n);

    return INDI::Telescope::ISNewText(dev, name, texts, names, n);
}

bool DSC::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, AxisSettingsNP.name) == 0)
        {
            IUUpdateNumber(&AxisSettingsNP, values, names, n);
            AxisSettingsNP.s = IPS_OK;
            IDSetNumber(&AxisSettingsNP, nullptr);
            return true;
        }

        /*if(strcmp(name,EncoderOffsetNP.name) == 0)
        {
            IUUpdateNumber(&EncoderOffsetNP, values, names, n);
            EncoderOffsetNP.s = IPS_OK;
            IDSetNumber(&EncoderOffsetNP, nullptr);
            return true;
        }*/

        if (strcmp(name, SimEncoderNP.name) == 0)
        {
            IUUpdateNumber(&SimEncoderNP, values, names, n);
            SimEncoderNP.s = IPS_OK;
            IDSetNumber(&SimEncoderNP, nullptr);
            return true;
        }

        ProcessAlignmentNumberProperties(this, name, values, names, n);
    }

    return INDI::Telescope::ISNewNumber(dev, name, values, names, n);
}

bool DSC::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev != nullptr && strcmp(dev, getDeviceName()) == 0)
    {
        if (strcmp(name, ReverseSP.name) == 0)
        {
            IUUpdateSwitch(&ReverseSP, states, names, n);
            ReverseSP.s = IPS_OK;
            IDSetSwitch(&ReverseSP, nullptr);
            return true;
        }

        if (strcmp(name, MountTypeSP.name) == 0)
        {
            IUUpdateSwitch(&MountTypeSP, states, names, n);
            MountTypeSP.s = IPS_OK;
            IDSetSwitch(&MountTypeSP, nullptr);
            return true;
        }

        if (strcmp(name, AxisRangeSP.name) == 0)
        {
            IUUpdateSwitch(&AxisRangeSP, states, names, n);
            AxisRangeSP.s = IPS_OK;

            if (AxisRangeS[AXIS_FULL_STEP].s == ISS_ON)
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Axis range is from 0 to %.f", AxisSettingsN[AXIS1_TICKS].value);
            }
            else
            {
                DEBUGF(INDI::Logger::DBG_SESSION, "Axis range is from -%.f to %.f",
                       AxisSettingsN[AXIS1_TICKS].value / 2, AxisSettingsN[AXIS1_TICKS].value / 2);
            }
            IDSetSwitch(&AxisRangeSP, nullptr);
            return true;
        }

        ProcessAlignmentSwitchProperties(this, name, states, names, n);
    }

    return INDI::Telescope::ISNewSwitch(dev, name, states, names, n);
}

bool DSC::Handshake()
{
    return true;
}

bool DSC::ReadScopeStatus()
{
    // Send 'Q'
    char CR[1] = { 0x51 };
    // Response
    char response[16] = { 0 };
    int rc = 0, nbytes_read = 0, nbytes_written = 0;

    DEBUGF(INDI::Logger::DBG_DEBUG, "CMD: %#02X", CR[0]);

    if (isSimulation())
    {
        snprintf(response, 16, "%06.f\t%06.f", SimEncoderN[AXIS1_ENCODER].value, SimEncoderN[AXIS2_ENCODER].value);
    }
    else
    {
        tcflush(PortFD, TCIFLUSH);

        if ((rc = tty_write(PortFD, CR, 1, &nbytes_written)) != TTY_OK)
        {
            char errmsg[256];
            tty_error_msg(rc, errmsg, 256);
            DEBUGF(INDI::Logger::DBG_ERROR, "Error writing to device %s (%d)", errmsg, rc);
            return false;
        }

        // Read until we encounter a CR
        if ((rc = tty_read_section(PortFD, response, 0x0D, DSC_TIMEOUT, &nbytes_read)) != TTY_OK)
        {
            // if we read enough, let's try to process, otherwise throw error
            // 6 characters for each number
            if (nbytes_read < 12)
            {
                char errmsg[256];
                tty_error_msg(rc, errmsg, 256);
                DEBUGF(INDI::Logger::DBG_ERROR, "Error reading from device %s (%d)", errmsg, rc);
                return false;
            }
        }
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "RES: %s", response);

    double Axis1Encoder = 0, Axis2Encoder = 0;
    std::regex rgx(R"((\+?\-?\d+)\s(\+?\-?\d+))");
    std::smatch match;
    std::string input(response);

    if (std::regex_search(input, match, rgx))
    {
        Axis1Encoder = atof(match.str(1).c_str());
        Axis2Encoder = atof(match.str(2).c_str());
    }
    else
    {
        DEBUGF(INDI::Logger::DBG_ERROR, "Error processing response: %s", response);
        EncoderNP.s = IPS_ALERT;
        IDSetNumber(&EncoderNP, nullptr);
        return false;
    }

    DEBUGF(INDI::Logger::DBG_DEBUG, "Raw Axis encoders. Axis1: %g Axis2: %g", Axis1Encoder, Axis2Encoder);

    EncoderN[AXIS1_RAW_ENCODER].value = Axis1Encoder;
    EncoderN[AXIS2_RAW_ENCODER].value = Axis2Encoder;

    // Convert Half Step to Full Step
    if (AxisRangeS[AXIS_HALF_STEP].s == ISS_ON)
    {
        if (Axis1Encoder < 0)
            Axis1Encoder += AxisSettingsN[AXIS1_TICKS].value;

        if (Axis2Encoder < 0)
            Axis2Encoder += AxisSettingsN[AXIS2_TICKS].value;
    }

    // Calculate reverse values
    double Axis1 = Axis1Encoder;
    if (ReverseS[AXIS1_ENCODER].s == ISS_ON)
        Axis1 = AxisSettingsN[AXIS1_TICKS].value - Axis1;

#if 0
    if (Axis1Diff < 0)
        Axis1Diff += AxisSettingsN[AXIS1_TICKS].value;
    else if (Axis1Diff > AxisSettingsN[AXIS1_TICKS].value)
        Axis1Diff -= AxisSettingsN[AXIS1_TICKS].value;
#endif

    double Axis2 = Axis2Encoder;
    if (ReverseS[AXIS2_ENCODER].s == ISS_ON)
        Axis2 = AxisSettingsN[AXIS2_TICKS].value - Axis2;

    DEBUGF(INDI::Logger::DBG_DEBUG, "Axis encoders after reverse. Axis1: %g Axis2: %g", Axis1, Axis2);

    // Apply raw offsets

    // It seems having encoder offsets like this is confusing for users
    //Axis1 = (Axis1 * EncoderOffsetN[OFFSET_AXIS1_SCALE].value + EncoderOffsetN[OFFSET_AXIS1_OFFSET].value);
    //Axis2 = (Axis2 * EncoderOffsetN[OFFSET_AXIS2_SCALE].value + EncoderOffsetN[OFFSET_AXIS2_OFFSET].value);

    //DEBUGF(INDI::Logger::DBG_DEBUG, "Axis encoders after raw offsets. Axis1: %g Axis2: %g", Axis1, Axis2);

    EncoderN[AXIS1_ENCODER].value = Axis1;
    EncoderN[AXIS2_ENCODER].value = Axis2;
    EncoderNP.s                   = IPS_OK;
    IDSetNumber(&EncoderNP, nullptr);

    double Axis1Degrees = (Axis1 / AxisSettingsN[AXIS1_TICKS].value * 360.0) + AxisSettingsN[AXIS1_DEGREE_OFFSET].value;
    double Axis2Degrees = (Axis2 / AxisSettingsN[AXIS2_TICKS].value * 360.0) + AxisSettingsN[AXIS2_DEGREE_OFFSET].value;

    Axis1Degrees = range360(Axis1Degrees);
    Axis2Degrees = range360(Axis2Degrees);

    // Adjust for LST
    double LST = get_local_sidereal_time(observer.lng);

    // Final aligned equatorial position
    ln_equ_posn eq { 0, 0 };

    // Now we proceed depending on mount type
    if (MountTypeS[MOUNT_EQUATORIAL].s == ISS_ON)
    {
        encoderEquatorialCoordinates.ra = Axis1Degrees / 15.0;

        encoderEquatorialCoordinates.ra += LST;
        encoderEquatorialCoordinates.ra = range24(encoderEquatorialCoordinates.ra);

        encoderEquatorialCoordinates.dec = rangeDec(Axis2Degrees);

        // Do alignment
        eq = TelescopeEquatorialToSky();
    }
    else
    {
        encoderHorizontalCoordinates.az = Axis1Degrees;
        encoderHorizontalCoordinates.az += 180;
        encoderHorizontalCoordinates.az = range360(encoderHorizontalCoordinates.az);

        encoderHorizontalCoordinates.alt = Axis2Degrees;

        // Do alignment
        eq = TelescopeHorizontalToSky();

        char AzStr[64], AltStr[64];
        fs_sexa(AzStr, Axis1Degrees, 2, 3600);
        fs_sexa(AltStr, Axis2Degrees, 2, 3600);
        DEBUGF(INDI::Logger::DBG_DEBUG, "Current Az: %s Current Alt: %s", AzStr, AltStr);

        //ln_get_equ_from_hrz(&encoderHorizontalCoordinates, &observer, ln_get_julian_from_sys(), &encoderEquatorialCoordinates);
        //equatorialPos.ra /= 15.0;

        //encoderEquatorialCoordinates.ra  = range24(encoderEquatorialCoordinates.ra);
        //encoderEquatorialCoordinates.dec = rangeDec(encoderEquatorialCoordinates.dec);
    }

    //  Now feed the rest of the system with corrected data
    NewRaDec(eq.ra, eq.dec);
    return true;
}

bool DSC::Sync(double ra, double dec)
{
    AlignmentDatabaseEntry NewEntry;
    struct ln_equ_posn RaDec { 0, 0 };
    struct ln_hrz_posn AltAz { 0, 0 };

    if (MountTypeS[MOUNT_EQUATORIAL].s == ISS_ON)
    {
        double LST = get_local_sidereal_time(observer.lng);
        RaDec.ra   = ((LST - encoderEquatorialCoordinates.ra) * 360.0) / 24.0;
        RaDec.dec  = encoderEquatorialCoordinates.dec;
    }
    else
    {
        AltAz.az  = encoderHorizontalCoordinates.az;
        AltAz.alt = encoderHorizontalCoordinates.alt;
    }

    NewEntry.ObservationJulianDate = ln_get_julian_from_sys();
    NewEntry.RightAscension        = ra;
    NewEntry.Declination           = dec;

    if (MountTypeS[MOUNT_EQUATORIAL].s == ISS_ON)
        NewEntry.TelescopeDirection = TelescopeDirectionVectorFromLocalHourAngleDeclination(RaDec);
    else
        NewEntry.TelescopeDirection = TelescopeDirectionVectorFromAltitudeAzimuth(AltAz);

    NewEntry.PrivateDataSize = 0;
    DEBUGF(INDI::AlignmentSubsystem::DBG_ALIGNMENT, "New sync point Date %lf RA %lf DEC %lf TDV(x %lf y %lf z %lf)",
           NewEntry.ObservationJulianDate, NewEntry.RightAscension, NewEntry.Declination, NewEntry.TelescopeDirection.x,
           NewEntry.TelescopeDirection.y, NewEntry.TelescopeDirection.z);

    if (!CheckForDuplicateSyncPoint(NewEntry))
    {
        GetAlignmentDatabase().push_back(NewEntry);

        // Tell the client about size change
        UpdateSize();

        // Tell the math plugin to reinitialise
        Initialise(this);

        return true;
    }

    return false;
}

ln_equ_posn DSC::TelescopeEquatorialToSky()
{
    double RightAscension, Declination;
    ln_equ_posn eq { 0, 0 };

    if (GetAlignmentDatabase().size() > 1)
    {
        TelescopeDirectionVector TDV;

        /*  and here we convert from ra/dec to hour angle / dec before calling alignment stuff */
        double lha, lst;
        lst = get_local_sidereal_time(LocationN[LOCATION_LONGITUDE].value);
        lha = get_local_hour_angle(lst, encoderEquatorialCoordinates.ra);
        //  convert lha to degrees
        lha    = lha * 360 / 24;
        eq.ra  = lha;
        eq.dec = encoderEquatorialCoordinates.dec;
        TDV    = TelescopeDirectionVectorFromLocalHourAngleDeclination(eq);

        if (!TransformTelescopeToCelestial(TDV, RightAscension, Declination))
        {
            RightAscension = encoderEquatorialCoordinates.ra;
            Declination    = encoderEquatorialCoordinates.dec;
        }
    }
    else
    {
        //  With less than 2 align points
        // Just return raw data
        RightAscension = encoderEquatorialCoordinates.ra;
        Declination    = encoderEquatorialCoordinates.dec;
    }

    eq.ra  = RightAscension;
    eq.dec = Declination;
    return eq;
}

ln_equ_posn DSC::TelescopeHorizontalToSky()
{
    ln_equ_posn eq { 0, 0 };
    TelescopeDirectionVector TDV = TelescopeDirectionVectorFromAltitudeAzimuth(encoderHorizontalCoordinates);
    double RightAscension, Declination;

    if (!TransformTelescopeToCelestial(TDV, RightAscension, Declination))
    {
        struct ln_equ_posn EquatorialCoordinates { 0, 0 };
        TelescopeDirectionVector RotatedTDV(TDV);

        switch (GetApproximateMountAlignment())
        {
            case ZENITH:
                break;

            case NORTH_CELESTIAL_POLE:
                // Rotate the TDV coordinate system anticlockwise (positive) around the y axis by 90 minus
                // the (positive)observatory latitude. The vector itself is rotated clockwise
                RotatedTDV.RotateAroundY(90.0 - observer.lat);
                AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, encoderHorizontalCoordinates);
                break;

            case SOUTH_CELESTIAL_POLE:
                // Rotate the TDV coordinate system clockwise (negative) around the y axis by 90 plus
                // the (negative)observatory latitude. The vector itself is rotated anticlockwise
                RotatedTDV.RotateAroundY(-90.0 - observer.lat);
                AltitudeAzimuthFromTelescopeDirectionVector(RotatedTDV, encoderHorizontalCoordinates);
                break;
        }

        ln_get_equ_from_hrz(&encoderHorizontalCoordinates, &observer, ln_get_julian_from_sys(), &EquatorialCoordinates);

        // libnova works in decimal degrees
        RightAscension = EquatorialCoordinates.ra * 24.0 / 360.0;
        Declination    = EquatorialCoordinates.dec;
    }

    eq.ra  = RightAscension;
    eq.dec = Declination;
    return eq;
}

bool DSC::updateLocation(double latitude, double longitude, double elevation)
{
    UpdateLocation(latitude, longitude, elevation);

    INDI_UNUSED(elevation);
    // JM: INDI Longitude is 0 to 360 increasing EAST. libnova East is Positive, West is negative
    observer.lng = longitude;

    if (observer.lng > 180)
        observer.lng -= 360;
    observer.lat = latitude;

    DEBUGF(INDI::Logger::DBG_SESSION, "Location updated: Longitude (%g) Latitude (%g)", observer.lng, observer.lat);
    return true;
}

void DSC::simulationTriggered(bool enable)
{
    if (!isConnected())
        return;

    if (enable)
    {
        defineNumber(&SimEncoderNP);
    }
    else
    {
        deleteProperty(SimEncoderNP.name);
    }
}
